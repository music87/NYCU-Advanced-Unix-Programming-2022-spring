#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include "file.hpp"

using namespace std;
class process{
public:
	process(string dir_name);
	static unordered_map<uid_t, string> user_table;
private:
	void set_command();
	void set_pid(string dir_name);
	void set_user_name();
	void handle_special_files();
	void handle_memory_maps();
	void handle_file_descriptors();
	bool alive = true;
	void print_opening_files();
	string path; // "/proc/[pid]"
	string command;
	string pid;
	string uid;
	string user_name;
	unordered_set<string> hash;
	vector<file> file_list;
};

process::process(string dir_name){
	path = "/proc/" + dir_name;
	set_command();
	set_pid(dir_name);
	set_user_name();
	//printf("cmd=%s, pid=%s, uid=%s, user_name=%s\n", command.c_str(), pid.c_str(), uid.c_str() ,user_name.c_str());

	handle_special_files();
	handle_memory_maps();
	handle_file_descriptors();
	print_opening_files();
}

void process::set_command(){
	if(!alive)
		return;
	char buffer[1024];
	ssize_t len;
	int fd;
	if((fd=open((path + "/comm").c_str(), O_RDONLY)) == -1){
		perror("fopen");
		alive = false;
		return;
	}
	if((len=read(fd, buffer, sizeof(buffer))) == -1){
		perror("read");
		alive = false;
		return;
	}
	buffer[len-1]='\0';
	command = buffer;
	close(fd);
}

void process::set_pid(string dir_name){
	if(!alive)
		return;
	pid = dir_name;
}

void process::set_user_name(){
	if(!alive)
		return;

	struct stat statbuf;
	if(stat(path.c_str(), &statbuf) == -1){
		perror("stat");
		alive = false;
		return;
	}
	uid = to_string(statbuf.st_uid);
	user_name = user_table[statbuf.st_uid];
}

void process::handle_special_files(){
	if(!alive)
		return;
	// handle /proc/[pid]/cwd, /proc/[pid]/root, /proc/[pid]/txt
	char buffer[1024];
	ssize_t len;
	vector<pair<string, string>> target;
	target.push_back(make_pair<string, string>("cwd", path + "/cwd"));
	target.push_back(make_pair<string, string>("rtd", path + "/root"));
	target.push_back(make_pair<string, string>("txt", path + "/exe"));

	for(auto tar : target){
		if((len=readlink(tar.second.c_str(), buffer, sizeof(buffer))) == -1){
			int errsv = errno; // according to man 3 errno
			if(errsv == EACCES){
				file_list.push_back(file(tar.first, tar.second + " (Permission denied)", "", "unknown"));
				continue;
			} else{
				perror("readlink");
				alive = false;
				return;
			}
		}
		buffer[len] = '\0';
		string file_name(buffer);
		try{
			file_list.push_back(file(tar.first, tar.second, file_name));
			hash.insert(file_name);
		} catch(runtime_error &e){
			printf("%s\n", e.what());
			alive = false;
			return;
		}
	}
}

void process::handle_memory_maps(){
	if(!alive)
		return;
	// handle /proc/[pid]/maps
	FILE *fp;
	char buffer[1024];
	if((fp=fopen((path + "/maps").c_str(), "r")) == NULL){
		int errsv = errno;
		if(errsv == EACCES){
			return;
		} else{
			perror("fopen");
			alive=false;
			return;
		}
	}
	char* re;
	char deli[] = " \t\n";
	while((re=fgets(buffer, sizeof(buffer), fp))){
		if(re == NULL){
			perror("fgets");
			alive=false;
			return;
		}
		char *token = strtok(buffer, deli);
		token = strtok(NULL, deli);
		token = strtok(NULL, deli);
		token = strtok(NULL, deli);
		token = strtok(NULL, deli);
		string inode(token);
		if(inode == "0")
			continue;
		token = strtok(NULL, deli);
		string file_name(token);
		if(hash.count(file_name))
			continue;
		hash.insert(file_name);
		string fd_field = "mem";
		if(file_name.find("deleted") != string::npos)
			fd_field = "DEL";
		file_list.push_back(file(fd_field, file_name, inode, "REG"));
	}
	fclose(fp);
}

void process::handle_file_descriptors(){
	if(!alive)
		return;
	// handle /proc/[pid]/fd
	DIR *dirp;
	struct dirent *dp;
	if((dirp=opendir((path + "/fd").c_str())) == NULL){
		int errsv = errno;
		if(errsv == EACCES){
			file_list.push_back(file("NOFD", path + "/fd" + " (Permission denied)", "", ""));
			return;
		} else{
			perror("opendir");
			alive = false;
			return;
		}
	}

	char buffer[1024];
	ssize_t len;
	while((dp=readdir(dirp))){
		if(dp==NULL && errno==0)
			break;
		if(dp==NULL && errno!=0){
			perror("readdir");
			errno=0;
			continue;
		}
		string file_descriptor(dp->d_name);
		if(file_descriptor=="." || file_descriptor=="..")
			continue;
		if((len=readlink((path + "/fd/" + file_descriptor).c_str(), buffer, sizeof(buffer))) == -1){
			perror("readlink");
			errno=0;
			continue;
		}
		buffer[len]='\0';
		string file_name(buffer);
		try{
			file_list.push_back(file(file_descriptor, path + "/fd/" + file_descriptor, file_name));
			hash.insert(file_name);
		} catch(runtime_error &e){
			printf("%s\n", e.what());
			continue;
		}
	}
}

void process::print_opening_files(){
	if(!alive)
		return;
	for(auto it=file_list.begin(); it!= file_list.end(); it++){
		printf("%s\t\t%s\t\t%s\t\t", command.c_str(), pid.c_str(), user_name.c_str());
		printf("%s\t\t%s\t\t%s\t\t%s\n", it->fd.c_str(), it->type.c_str(), it->inode.c_str(), it->name.c_str());
	}


}
