#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char* argv[]){
	// parse argument
	int option;
	std::string path_libc = "./logger.so";
	std::string path_out = "stderr";
	while((option = getopt(argc, argv, "o:p:")) != -1){
		switch(option){
		case 'o':
			path_out = std::string(optarg);
			//printf("option o: %s\n", path_out.c_str());

			break;
		case 'p':
			path_libc = std::string(optarg);
			//printf("option p: %s\n", path_libc.c_str());
			break;
		case '?':
		case ':':
		default:
			printf("usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]\n"
					"\t-p: set the path to logger.so, default = ./logger.so\n"
					"\t-o: print output to file, print to \"stderr\" if no file specified\n"
					"\t--: separate the arguments for logger and for the command\n");
			return -1;
		}
	}

	argc -= optind;
	argv +=optind;

	if(argc == 0){
		printf("no command given.\n");
		return -1;
	}

	//for(int i=0; i<argc; i++){
	//	printf("argv[%d] = %s\n", optind +i, *(argv+i));
	//}

	int out_fd;
	if(path_out == "stderr"){
		out_fd = dup(STDERR_FILENO);
	} else{
		out_fd = open(path_out.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if(out_fd==-1)
			perror("open");
	}
	setenv("OUT_FD", std::to_string(out_fd).c_str(), 1);
	setenv("LD_PRELOAD", path_libc.c_str(), 1);
	if(execvp(argv[0], argv)==-1){
		perror("execvp");
		return -1;
	}

	return 0;
}
