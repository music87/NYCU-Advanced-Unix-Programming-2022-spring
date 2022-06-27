#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <regex>
#include <string>
#include <vector>
#include <unordered_map>
#include "argument_parser.hpp"
#include "process.hpp"
using namespace std;

unordered_map<uid_t, string> process::user_table;
void set_user_table(){
	FILE* fp;
	char buffer[1024];
	if((fp=fopen("/etc/passwd", "r")) == NULL){
		//perror("fopen");
	}
	while(fgets(buffer, sizeof(buffer), fp)){
		char deli[2] = ":";
		char* token = strtok(buffer, deli);
		string user_name(token);
		token = strtok(NULL, deli);
		token = strtok(NULL, deli);
		uid_t uid = atoi(token);
		process::user_table[uid] = user_name;
	}
	fclose(fp);
}

int main(int argc, char *argv[]){
	regex filter_command(".*"), filter_type("REG|CHR|DIR|FIFO|SOCK|unknown|"""), filter_file_name(".*");
	if(!argument_parser(argc, argv, filter_command, filter_type, filter_file_name))
		exit(EXIT_FAILURE);
	set_user_table();

	printf("COMMAND\t\tPID\t\tUSER\t\tFD\t\tTYPE\t\tNODE\t\tNAME\n");
	regex pid_regex("\\d+");
	struct dirent *dp;
	DIR *dirp = opendir("/proc");
	while((dp = readdir(dirp)) != NULL){
		if(!regex_match(dp->d_name, pid_regex))
			continue;
		process p(string(dp->d_name), filter_command, filter_type, filter_file_name);
	}
	closedir(dirp);
	return 0;
}
