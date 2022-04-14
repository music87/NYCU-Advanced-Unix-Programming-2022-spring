#include <dlfcn.h>
#include <string>
#include <gnu/lib-names.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include "utils.hpp"

// spec: $ man 3 xxx
// extern C to convert cpp symbol into c symbol
#ifdef __cplusplus
extern "C"{
	int chmod(const char*, mode_t);
	int chown(const char*, uid_t, gid_t);
	int close(int);
	int creat(const char*, mode_t);
	int fclose(FILE*);
	FILE* fopen(const char*, const char*);
	size_t fread(void*, size_t, size_t, FILE*);
	size_t fwrite(const void*, size_t, size_t, FILE*);
	int open(const char*, int, ...);
	ssize_t read(int, void*, size_t);
	int remove(const char*);
	int rename(const char*, const char*);
	FILE *tmpfile(void);
	ssize_t write(int, const void*, size_t);
}
#endif


static int (*origin_chmod)(const char*, mode_t) = NULL;
int chmod(const char *path, mode_t mode){
	origin_chmod = (int (*)(const char*, mode_t)) get_origin_func("chmod");
	int ret_val = origin_chmod(path, mode);
	std::string abs_path = get_abs_path(path);

	fprintf(stderr, "[logger] chmod(%s, %o) = %d\n", abs_path.c_str(), mode, ret_val);
	return ret_val;
}

static int (*origin_chown)(const char*, uid_t, gid_t) = NULL;
int chown(const char* path, uid_t owner, gid_t group){
	origin_chown = (int (*)(const char*, uid_t, gid_t))get_origin_func("chown");
	int ret_val = origin_chown(path, owner, group);
	std::string abs_path = get_abs_path(path);
	fprintf(stderr, "[logger] chown(%s, %d , %d) = %d\n", abs_path.c_str(), owner, group, ret_val);
	return ret_val;
}

static int (*origin_close)(int) = NULL;
int close(int fd){
	origin_close = (int (*)(int)) get_origin_func("close");
	std::string abs_path = get_abs_path(fd); // order care
	int ret_val = origin_close(fd);
	fprintf(stderr, "[logger] close(%s) = %d\n", abs_path.c_str(), ret_val);
	return ret_val;
}

static int (*origin_open)(const char*, int) = NULL;
int open(const char *pathname, int flags, ...){
	// fetch optional argument, mode, accord. glibc source code open.c
	mode_t mode=0;
	if (__OPEN_NEEDS_MODE (flags)){
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);
	}

	// fetch original function
	origin_open = (int (*)(const char*, int)) get_origin_func("open");
	int ret_val = origin_open(pathname, flags);

	// resolve real absolute path
	std::string abs_path = get_abs_path(pathname);

	fprintf(stderr, "[logger] open(%s, %o, %o) = %d\n", abs_path.c_str(), flags, mode, ret_val);

	return ret_val;
}
