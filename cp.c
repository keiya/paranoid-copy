/*
 * cp.c
 * A robust copy command
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 * */

#include <stdio.h>
#include <openssl/sha.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libgen.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define OPEN_SRC 0
#define OPEN_DST 1

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

void file_munmap(int fd, void *mmap_addr, struct stat *fs)
{
	munmap(mmap_addr, fs->st_size);
	close(fd);
}

void do_copy(struct file_s *src, struct file_s *dst)
{
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

	while (remain_size--)
	{
		*mdst++ = *msrc++;
	}
	
	file_munmap(fdsrc,msrc,&fssrc);
	file_munmap(fddst,mdst,&fsdst);
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
		do_copy(src[0],dst);
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
	
				do_copy(src[i],dsttmp);
				//free(tmp); // realloc
				free(dsttmp); // realloc
			}
		}
	}
	return 0;
}

