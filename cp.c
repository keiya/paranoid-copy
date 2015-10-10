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
#include <sys/mman.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

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

void do_copy(struct file_s *src, struct file_s *dst)
{
	int fd;
	struct stat     fs;
	char   *file;
#ifdef DEBUG
	printf("%p->%p\n",src,dst);
#endif
	if (src == NULL||dst == NULL) return;
#ifdef DEBUG
	printf("'%s'->'%s'\n",src->path,dst->path);
#endif
	fd = open(src->path, O_RDONLY);
	if (fd < 0) {
		perror(src->path);
		exit(-1);
	}

	if (fstat(fd, &fs) < 0) {
		perror("fstat");
		exit(-1);
	}

	file = mmap(NULL, fs.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) {
		perror("mmap");
		exit(-1);
	}


	long long remain_size = fs.st_size;

	char *p = NULL;
	//memcpy
	close(fd);
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

	for (i=0; i<nsrc; ++i)
	{
		do_copy(src[i],dst);
	}
	return 0;
}

