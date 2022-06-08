#pragma once
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <elf.h>
#include <libgen.h>
#include <map>
#include "utils.hpp"

using namespace std;
// maps_parser
// elf64_parser
// dbgcmd_parser

typedef struct range_s {
	unsigned long begin, end;
}	range_t;

typedef struct map_entry_s {
	range_t range;
	int perm;
	long offset;
	std::string name;
}	map_entry_t;

bool operator<(range_t r1, range_t r2) {
	if(r1.begin < r2.begin && r1.end < r2.end) return true;
	return false;
}
// refer to unix_prog/ptrace/ptools.cpp
int maps_parser(pid_t pid, map<range_t, map_entry_t>& loaded) {
	char fn[128];
	char buf[256];
	FILE *fp;
	snprintf(fn, sizeof(fn), "/proc/%u/maps", pid);
	if((fp = fopen(fn, "rt")) == NULL) return -1;
	while(fgets(buf, sizeof(buf), fp) != NULL) {
		int nargs = 0;
		char *token, *saveptr, *args[8], *ptr = buf;
		map_entry_t m;
		while(nargs < 8 && (token = strtok_r(ptr, " \t\n\r", &saveptr)) != NULL) {
			args[nargs++] = token;
			ptr = NULL;
		}
		if(nargs < 6) continue;
		if((ptr = strchr(args[0], '-')) != NULL) {
			*ptr = '\0';
			m.range.begin = strtol(args[0], NULL, 16);
			m.range.end = strtol(ptr+1, NULL, 16);
		}
		m.name = basename(args[5]);
		m.perm = 0;
		if(args[1][0] == 'r') m.perm |= 0x04;
		if(args[1][1] == 'w') m.perm |= 0x02;
		if(args[1][2] == 'x') m.perm |= 0x01;
		m.offset = strtol(args[2], NULL, 16);
		//printf("XXX: %lx-%lx %04o %s\n", m.range.begin, m.range.end, m.perm, m.name.c_str());
		loaded[m.range] = m;
	}
	return (int) loaded.size();
}

// man elf or google c read elf header
int elf64_parser(const string prog_path, unsigned long long &entry_point, unsigned long long &text_begin, unsigned long long &text_end){
	FILE* fptr = fopen(prog_path.c_str(), "rb");
	if(fptr == NULL){
		perror("** fopen");
		return -1;
	}

	// handle elf header to get entry point
	Elf64_Ehdr elf_header;
	fread(&elf_header, sizeof(elf_header), 1, fptr);
	if(memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
		cerr << "** invalid elf file" << endl;
		return -1;
	}
	entry_point = elf_header.e_entry;

	// handle section header to get the address range of .text section
	Elf64_Shdr section_header;
	fseek(fptr, elf_header.e_shoff, SEEK_SET);
	fread(&section_header, sizeof(section_header), 1, fptr);

	Elf64_Shdr shstr_entry;
	fseek(fptr, elf_header.e_shoff + elf_header.e_shstrndx * elf_header.e_shentsize, SEEK_SET);
	fread(&shstr_entry, sizeof(shstr_entry), 1, fptr);

	char shstr_section[shstr_entry.sh_size];
	fseek(fptr, shstr_entry.sh_offset, SEEK_SET);
	fread(shstr_section, 1, shstr_entry.sh_size, fptr);

	for(int i=0; i<elf_header.e_shnum; i++){
		Elf64_Shdr cur_entry;
		fseek(fptr, elf_header.e_shoff + i * elf_header.e_shentsize, SEEK_SET);
		fread(&cur_entry, 1, elf_header.e_shentsize, fptr);
		char* cur_name = shstr_section+(cur_entry.sh_name);

		if(strcmp(cur_name, ".text") == 0){
			// if current section's name is ".text"
			text_begin = cur_entry.sh_addr;
			text_end = cur_entry.sh_addr + cur_entry.sh_size;
			break;
		}
	}

	fclose(fptr);
	return 0;
}

vector<string> dbgcmd_parser(string line, char delimiter){
	size_t pos1=0, pos2;
	vector<string> result;
	while((pos2=line.find(delimiter, pos1)) != string::npos){
		string word = line.substr(pos1, pos2-pos1);
		if(word=="") {pos1+=1; continue;}
		result.push_back(word);
		pos1 = pos2 + 1;
	}
	result.push_back(line.substr(pos1));
	return result;
}
