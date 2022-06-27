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

	dprintf(atoi(getenv("OUT_FD")), "[logger] chmod(\"%s\", %o) = %d\n", abs_path.c_str(), mode, ret_val);
	return ret_val;
}

static int (*origin_chown)(const char*, uid_t, gid_t) = NULL;
int chown(const char* path, uid_t owner, gid_t group){
	origin_chown = (int (*)(const char*, uid_t, gid_t))get_origin_func("chown");
	int ret_val = origin_chown(path, owner, group);
	std::string abs_path = get_abs_path(path);
	dprintf(atoi(getenv("OUT_FD")), "[logger] chown(\"%s\", %d, %d) = %d\n", abs_path.c_str(), owner, group, ret_val);
	return ret_val;
}

static int (*origin_close)(int) = NULL;
int close(int fd){
	origin_close = (int (*)(int)) get_origin_func("close");
	std::string abs_path = get_abs_path(fd); // order matter
	int ret_val = origin_close(fd);
	dprintf(atoi(getenv("OUT_FD")), "[logger] close(\"%s\") = %d\n", abs_path.c_str(), ret_val);
	return ret_val;
}

static int (*origin_creat)(const char*, mode_t) = NULL;
int creat(const char *path, mode_t mode){
	origin_creat = (int (*)(const char*, mode_t)) get_origin_func("creat");
	int ret_val = origin_creat(path, mode);
	std::string abs_path = get_abs_path(path); //order matter
	dprintf(atoi(getenv("OUT_FD")), "[logger] creat(\"%s\", %o) = %d\n", abs_path.c_str(), mode, ret_val);
	return ret_val;
}

static int (*origin_fclose)(FILE *) = NULL;
int fclose(FILE *stream){
	origin_fclose = (int (*)(FILE*)) get_origin_func("fclose");
	std::string abs_path = get_abs_path(stream); // order matter
	///int res_stderr_fd = dup(STDERR_FILENO);
	int ret_val = origin_fclose(stream);
	///dprintf(res_stderr_fd, "[logger] fclose(\"%s\") = %d\n", abs_path.c_str(), ret_val);
	///origin_fclose(fdopen(res_stderr_fd, "w+"));

	dprintf(atoi(getenv("OUT_FD")), "[logger] fclose(\"%s\") = %d\n", abs_path.c_str(), ret_val);
	return ret_val;
}

static FILE* (*origin_fopen)(const char*, const char*) = NULL;
FILE* fopen(const char *pathname, const char *mode){
	origin_fopen = (FILE* (*)(const char*, const char*)) get_origin_func("fopen");
	FILE *ret_fp = origin_fopen(pathname, mode);
	std::string abs_path = get_abs_path(pathname); // order matter
	dprintf(atoi(getenv("OUT_FD")), "[logger] fopen(\"%s\", \"%s\") = %p\n", abs_path.c_str(), mode, ret_fp);
	return ret_fp;
}

static size_t (*origin_fread)(void*, size_t, size_t, FILE*) = NULL;
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
	origin_fread = (size_t (*)(void*, size_t, size_t, FILE*)) get_origin_func("fread");
	size_t ret_val = origin_fread(ptr, size, nmemb, stream);
	std::string abs_path = get_abs_path(stream);
	std::string msg = get_buffer((char*)ptr, size*ret_val);
	dprintf(atoi(getenv("OUT_FD")), "[logger] fread(\"%s\", %ld, %ld, \"%s\") = %ld\n", msg.c_str(), size, nmemb, abs_path.c_str(), ret_val);
	return ret_val;
}

static size_t (*origin_fwrite)(const void*, size_t, size_t, FILE*) = NULL;
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE* stream){
	origin_fwrite = (size_t (*)(const void*, size_t, size_t, FILE*)) get_origin_func("fwrite");
	size_t ret_val = origin_fwrite(ptr, size, nmemb, stream);
	std::string abs_path = get_abs_path(stream);
	std::string msg = get_buffer((char *)ptr, size*ret_val);
	dprintf(atoi(getenv("OUT_FD")), "[logger] fwrite(\"%s\", %ld, %ld, \"%s\") = %ld\n", msg.c_str(), size, nmemb, abs_path.c_str(), ret_val);
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

	dprintf(atoi(getenv("OUT_FD")), "[logger] open(\"%s\", %o, %o) = %d\n", abs_path.c_str(), flags, mode, ret_val);

	return ret_val;
}

static ssize_t (*origin_read)(int, void*, size_t) = NULL;
ssize_t read(int fd, void* buf, size_t nbyte){
	origin_read = (ssize_t (*)(int, void*, size_t)) get_origin_func("read");
	ssize_t ret_val = origin_read(fd, buf, nbyte);
	std::string abs_path = get_abs_path(fd);
	std::string msg = get_buffer((char*) buf, ret_val);
	dprintf(atoi(getenv("OUT_FD")), "[logger] read(\"%s\", \"%s\", %ld) = %ld\n", abs_path.c_str(), msg.c_str(), nbyte, ret_val);
	return ret_val;
}

static ssize_t (*origin_write)(int, const void*, size_t) = NULL;
ssize_t write(int fd, const void* buf, size_t nbyte){
	origin_write = (ssize_t (*)(int, const void*, size_t)) get_origin_func("write");
	ssize_t ret_val = origin_write(fd, buf, nbyte);
	std::string abs_path = get_abs_path(fd);
	std::string msg = get_buffer((char*) buf, ret_val);
	dprintf(atoi(getenv("OUT_FD")), "[logger] write(\"%s\", \"%s\", %ld) = %ld\n", abs_path.c_str(), msg.c_str(), nbyte, ret_val);
	return ret_val;
}

static int (*origin_remove)(const char*) = NULL;
int remove(const char* pathname){
	origin_remove = (int (*)(const char*)) get_origin_func("remove");
	std::string abs_path = get_abs_path(pathname); //order mater
	int ret_val = origin_remove(pathname);
	dprintf(atoi(getenv("OUT_FD")), "[logger] remove(\"%s\") = %d\n", abs_path.c_str(), ret_val);
	return ret_val;
}

static int (*origin_rename)(const char*, const char*) = NULL;
int rename(const char* oldp, const char* newp){
	origin_rename = (int (*)(const char*, const char*)) get_origin_func("rename");
	std::string abs_oldp = get_abs_path(oldp);
	int ret_val = origin_rename(oldp, newp);
	std::string abs_newp = get_abs_path(newp); // order matter
	dprintf(atoi(getenv("OUT_FD")), "[logger] rename(\"%s\", \"%s\") = %d\n", abs_oldp.c_str(), abs_newp.c_str(), ret_val);
	return ret_val;
}

static FILE* (*origin_tmpfile)(void) = NULL;
FILE* tmpfile(void){
	origin_tmpfile = (FILE* (*)(void)) get_origin_func("tmpfile");
	FILE* ret_ptr = origin_tmpfile();
	dprintf(atoi(getenv("OUT_FD")), "[logger] tmpfile() = %p\n", ret_ptr);
	return ret_ptr;
}
