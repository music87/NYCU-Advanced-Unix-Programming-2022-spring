#include <dlfcn.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define BUFFER_SIZE 1024

void *get_origin_func(std::string func_name){
	void *handle = dlopen(LIBM_SO, RTLD_LAZY);
	void *origin_func = NULL;
	if(!handle){
		//fprintf(stderr, "%s\n", dlerror());
	} else{
		origin_func = dlsym(handle, func_name.c_str());
	}
	dlerror(); // clear error
	dlclose(handle);
	return origin_func;
}


std::string get_buffer(const char *bf_ptr, size_t len){
	// If a passed argument is a regular character buffer, print it out up to 32 bytes.
	std::string msg = "";
	for(int i=0; i<32; i++){
		if(i>=len)
			break;
		if(!isprint(bf_ptr[i]))
			msg += ".";
		else
			msg += bf_ptr[i];
	}
	return msg;
}

std::string get_abs_path(const char* file_name){
	char abs_path[BUFFER_SIZE];
	if(realpath(file_name, abs_path) == NULL){
		return "(null)";
		//perror("realpath");
	}
	return std::string(abs_path);
}

std::string get_abs_path(int fd){
	char file_name[BUFFER_SIZE];
	std::string abs_path;
	ssize_t len;
	std::string link = "/proc/" + std::to_string(getpid()) + "/fd/" + std::to_string(fd);
	if((len=readlink(link.c_str(), file_name, sizeof(file_name)))==-1){
		//perror("readlink");
		return "";
	}
	file_name[len] = '\0';
	struct stat statbuf;
	if(stat(link.c_str(), &statbuf) == -1){
		//perror("stat");
		return "";
	}
	mode_t type = statbuf.st_mode & S_IFMT;
	if(type == S_IFDIR || type == S_IFREG)
		abs_path = get_abs_path(file_name);
	else
		abs_path = std::string(file_name);
	return abs_path;
}

std::string get_abs_path(FILE* fp){

	int fd = fileno(fp);
	if(fd==-1){
		//perror("fileno");
		return "";
	}
	std::string abs_path = get_abs_path(fd);
	return abs_path;
}

