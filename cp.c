/*
 * cp.c
 * A robust copy command
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 * */

#include <stdio.h>
#include <openssl/sha.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libgen.h>
#include "crc32c.c"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define OPEN_SRC 0
#define OPEN_DST 1
#define SIZE (262144*3)
#define CHUNK SIZE

struct file_s {
	int isdir;
	char *path;
};

struct option_s {
	int paranoid;
	int recursive;
};
struct option_s option;

void show_usage()
{
	printf("RTFM!!!!!!!!!!! read the f___ing manual!!!!!!!\n");
	exit(EXIT_FAILURE);
}

int file_open(char *filename, struct stat *fs, int is_dst)
{
	int fd;
	fd = open(filename,
			//writable ? O_CREAT|O_TRUNC|S_IWRITE : O_RDONLY,
			is_dst ? O_CREAT|O_TRUNC|O_RDWR : O_RDONLY,
			is_dst ? S_IRUSR|S_IWUSR : 0);
	if (fd < 0) {
		perror(filename);
		exit(-1);
	}

	if (fstat(fd, fs) < 0) {
		perror("fstat");
		exit(-1);
	}
	return fd;
}

uint32_t file_crc32c(int fd, size_t fs, void *map)
{
	size_t off, n;
	uint32_t crc=0;
	int flag_map_addr_given = 1;
	if (map == NULL)
	{
		map = mmap(NULL, fs, PROT_READ, MAP_SHARED, fd, 0);
		flag_map_addr_given = 0;
		if (map == MAP_FAILED) {
			perror("mmap crc");
			exit(-1);
		}
	}
	long long remain_size = fs;
	for (off=0;remain_size>0; remain_size -= CHUNK)
	{
		n = CHUNK;
		if (remain_size < CHUNK)
			n = remain_size;
//		printf("remain_size=%d mdst=%p off=%d n=%d\n",remain_size,mdst,off,n);
		crc = crc32c(crc,map + off,n);
		off += n;
	}
	if (flag_map_addr_given == 0)
		munmap(map,fs);
	return crc;
}

int do_copy(struct file_s *src, struct file_s *dst)
{
	int crc_match = 0;
	int fdsrc, fddst;
	struct stat fssrc,fsdst;
	char *msrc, *mdst;
	if (src == NULL||dst == NULL) return;
#ifdef DEBUG
	printf("'%s'(%d)->'%s'(%d)\n",src->path,src->isdir,dst->path,dst->isdir);
#endif
	fdsrc = file_open(src->path, &fssrc, OPEN_SRC);
	fddst = file_open(dst->path, &fsdst, OPEN_DST);

	msrc = mmap(NULL, fssrc.st_size, PROT_READ, MAP_SHARED, fdsrc, 0);
	if (msrc == MAP_FAILED) {
		perror("mmap src");
		exit(-1);
	}
	if (ftruncate(fddst,fssrc.st_size) != 0)
	{
		perror("ftruncate");
	}
	mdst = mmap(NULL, fssrc.st_size, PROT_WRITE, MAP_SHARED, fddst, 0);
	if (mdst == MAP_FAILED) {
		perror("mmap dst");
		exit(-1);
	}

	long long remain_size = fssrc.st_size;

	char *tmp_msrc, *tmp_mdst;
	tmp_msrc = msrc;
	tmp_mdst = mdst;
	while (remain_size--)
	{
		*tmp_mdst++ = *tmp_msrc++;
	}

	// destroy the data if ERRORTEST defined
	// for test a crc
#ifdef ERRORTEST
	mdst[0] = ~mdst[0];
#endif

	if (msync(mdst,fssrc.st_size,MS_SYNC) != 0)
	{
		perror("msync");
		exit(-1);
	}
	
	munmap(mdst, fssrc.st_size);

	if (option.paranoid)
	{
		uint32_t src_crc = file_crc32c(fdsrc,fssrc.st_size,msrc);
		uint32_t dst_crc = file_crc32c(fddst,fssrc.st_size,NULL);
		if (src_crc == dst_crc)
			crc_match = 1;
		else
			crc_match = -1;
	}

	munmap(msrc, fssrc.st_size);
	close(fdsrc);
	close(fddst);
	return crc_match;
}

int main(int argc, char *argv[])
{
	// allows multiple src
	//char **src_path;
	//char *dst_path;
	struct file_s **src;
	struct file_s *dst;
	int nsrc=0;

	int ch;
	int i;


	static struct option long_options[] = {
		{"paranoid", 1, 0, 'c'},
		{NULL, 0 , NULL, 0}
	};

	option.paranoid = 0;
	option.recursive = 0;
	while ((ch = getopt_long(argc, argv, "c", long_options, NULL)) != -1)
	{
		switch (ch) {
			// paranoid copy mode
			// compare original and copied file by using sha-512
			case 'c':
				option.paranoid = 1;
				break;
			default:
				show_usage();
		}
	}

	struct stat stat_buf;
	if (argc-optind >= 2)
	{
		nsrc = argc-optind-1;
		//src_path = malloc(sizeof(char*) * argc-optind-1);
		src = malloc(sizeof(struct file_s*) * argc-optind-1);
		for (i=optind; i<argc; ++i)
		{
			int size_path = MIN(strlen(argv[i]),PATH_MAX);
			if (i==argc-1)
			{
				dst = malloc(sizeof(struct file_s));
				dst->path = malloc(size_path);
				if (stat(argv[i],&stat_buf) == 0)
				{
				
					if (S_ISDIR(stat_buf.st_mode))
					{
						dst->isdir = 1;
					}
					else
					{
						// コピー元が複数にもかかわらず、コピー先がディレクトリでない場合はエラー
						if (nsrc >= 2)
						{
							perror("target is not a directory.");
							exit(EXIT_FAILURE);
						}
					}

					//dst = malloc(size_dst);
					strncpy(dst->path,argv[i],size_path);
				}
				else
				{
					// コピー元が複数にもかかわらず、コピー先ディレクトリがない場合はエラー
					if (nsrc >= 2)
					{
						perror(argv[i]);
						exit(EXIT_FAILURE);
					}
					// コピー元がひとつの場合、コピー先がなければ新規作成となるのでエラーにはしない
					strncpy(dst->path,argv[i],size_path);
				}
			}
			else
			{
				if (stat(argv[i],&stat_buf) == 0)
				{
					int idx = i-optind;
					src[idx] = malloc(sizeof(struct file_s));
					if (S_ISDIR(stat_buf.st_mode))
					{
						if (option.recursive == 0)
						{
							fprintf(stderr,"ommiting directory: %s\n",argv[i]);
							break;
						}
						src[idx]->isdir = 1;
					}
					//src_path[idx] = malloc(size_dst);

					src[idx]->path = malloc(size_path);
					strncpy(src[idx]->path,argv[i],size_path);
				}
			}
		}
	}
	else {
		show_usage();
	}

	// dstがファイルかつsrcが複数でないとき
	if (nsrc == 1)
	{
#ifdef DEBUG
		printf("src is single file\n");
#endif
		if (do_copy(src[0],dst) == -1)
		{
			fprintf(stderr,"'%s'->'%s' CRC doesn't match\n",src[0]->path,dst->path);
		}
	}
	else {
		for (i=0; i<nsrc; ++i)
		{
			// dstがディレクトリなら、dst_filename=dst_dir/basename(src_path)
			if (dst->isdir == 1)
			{
#ifdef DEBUG
				printf("dst is directory %s\n",dst->path);
#endif
				//char *dirc,  *dname;
				char *basec, *bname;
				//char *tmp;
				//dirc = strdup(src[i]->path);
				basec = strdup(src[i]->path);
				//dname = dirname(dirc);
				bname = basename(basec);
				int dst_filename_size = MIN(strlen(dst->path)+1+strlen(bname)+1,PATH_MAX);

				// copy struct files*
				struct file_s *dsttmp;
				dsttmp = malloc(sizeof(struct file_s));
				char dst_filename[PATH_MAX];
				dsttmp->path = dst_filename;
				strncpy(dsttmp->path,dst->path,PATH_MAX);
				dsttmp->isdir = dst->isdir;

				char *pathdup = strdup(dsttmp->path);
				snprintf(dsttmp->path,dst_filename_size,"%s/%s",pathdup,bname);
				char dst_realpath[PATH_MAX];
				realpath(dsttmp->path,dst_realpath);
				dsttmp->path = dst_realpath;
				free(pathdup); // strdup
				free(basec); // strdup
	
				if (do_copy(src[i],dsttmp) == -1)
				{
					fprintf(stderr,"'%s'->'%s' CRC doesn't match\n",src[i]->path,dsttmp->path);
				}
				//free(tmp); // realloc
				free(dsttmp); // realloc
			}
		}
	}
	return 0;
}

