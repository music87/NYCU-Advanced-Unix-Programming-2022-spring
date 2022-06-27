#pragma once
#include <regex>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
using namespace std;
class process;
class file{
public:
	file(){};
	file(string, string, string);
	file(string, string, string, string);
	file& operator=(file const &); // copy assignment operator
	void print_info();
	friend process;

private:
	void set_fd_field(string, mode_t);
	void set_type_field(mode_t);
	void set_inode_field(ino_t);
	void set_name_field(string);
	string fd; //cwd, rtd, txt, mem, DEL, or [0-9]+[rwu]
	string type; //DIR, REG, CHR, FIFO, SOCK, unknown
	string inode;
	string name; // file name
};

file::file(string fd_field, string path, string inode_field, string type_field){
	fd = fd_field;
	type = type_field;
	inode = inode_field;
	name = path;
	// print_info();
}

file::file(string fd_field, string path, string file_name){
	struct stat statbuf, lstatbuf;
	if(stat(path.c_str(), &statbuf) == -1){
		//perror("stat");
		throw runtime_error("race condition, file no longer exist");
	}
	if(lstat(path.c_str(), &lstatbuf) == -1){
		//perror("lstat");
		throw runtime_error("race condition, file no longer exist");
	}

	set_fd_field(fd_field, lstatbuf.st_mode);
	set_type_field(statbuf.st_mode);
	set_inode_field(statbuf.st_ino);
	set_name_field(file_name);
	// print_info();
}

file& file::operator=(file const &other){
	// copy assignment operator
	if(&other == this)
		return *this;
	fd = other.fd;
	type = other.type;
	inode = other.inode;
	name = other.name;
	// printf("after copy, "); print_info();
	return *this;
}

void file::set_fd_field(string fd_field, mode_t mode){
	regex file_descriptor_regex("\\d+");
	// cwd, rtd, txt, mem, DEL
	if(!regex_match(fd_field, file_descriptor_regex))
		fd = fd_field;
	// [0-9]+[rwu]
	else if((mode&S_IRUSR) && (mode&S_IWUSR))
		fd = fd_field + "u";
	else if(mode&S_IRUSR)
		fd = fd_field + "r";
	else if(mode&S_IWUSR)
		fd = fd_field + "w";
}

void file::set_type_field(mode_t mode){
	switch(mode & S_IFMT){
		case S_IFDIR:
			type = "DIR";
			break;
		case S_IFREG:
			type = "REG";
			break;
		case S_IFCHR:
			type = "CHR";
			break;
		case S_IFIFO:
			type = "FIFO";
			break;
		case S_IFSOCK:
			type = "SOCK";
			break;
		default:
			type = "unknown";
			break;
	}
}

void file::set_inode_field(ino_t inode_field){
	inode = to_string(inode_field);
}

void file::set_name_field(string path){
	name = path;
}
void file::print_info(){
	printf("fd=%s, type=%s, inode=%s, name=%s\n", fd.c_str(), type.c_str(), inode.c_str(), name.c_str());
}
