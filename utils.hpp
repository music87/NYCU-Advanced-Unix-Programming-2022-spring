#include <dlfcn.h>
#include <ctype.h>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#define BUFFER_SIZE 1024

void *get_origin_func(std::string func_name){
	void *handle = dlopen(LIBM_SO, RTLD_LAZY);
	void *origin_func = NULL;
	if(!handle){
		fprintf(stderr, "%s\n", dlerror());
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
		if(bf_ptr[i]=='\0' || i>=len)
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
		perror("realpath");
	}
	return std::string(abs_path);
}

std::string get_abs_path(int fd){
	char file_name[BUFFER_SIZE];
	ssize_t len;
	if((len=readlink(("/proc/" + std::to_string(getpid()) + "/fd/" + std::to_string(fd)).c_str(), file_name, sizeof(file_name)))==-1){
		perror("readlink");
	}
	file_name[len] = '\0';

	std::string abs_path = get_abs_path(file_name);
	return abs_path;
}

std::string get_abs_path(FILE* fp){

	int fd = fileno(fp);
	if(fd==-1){
		perror("fileno");
	}
	std::string abs_path = get_abs_path(fd);
	return abs_path;
}

