#pragma once
#include <unordered_map>
#include <vector>
#include <map>
#include <string>
#include <capstone/capstone.h>
#include <inttypes.h>
#include <stdio.h>

using namespace std;


// ====== disassembler =====
class disassembler{
public:
	disassembler(){};
	void translate(const unsigned long, const unsigned char*);

private:
	csh handle;
	cs_insn *insn;
	size_t count;
};

void disassembler::translate(const unsigned long addr, const unsigned char * code){
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK){
		fprintf(stderr, "disasm cs_open error\n");
		return;
	}

	count = cs_disasm(handle, code, sizeof(code)-1, addr, 0, &insn);
	if (count > 0) {
		size_t j;
		fprintf(stdout, "** breakpoint @\t 0x%" PRIx64 ": ", insn[0].address);
		for (j = 0; j <insn[0].size; j++)
			fprintf(stdout, "%2.2x ", code[j]);
		fprintf(stdout, "\t%s\t%s\n", insn[0].mnemonic ,insn[0].op_str);
	}
	cs_close(&handle);
}
// =========================

// ====== breakpoint table =====
class breakpoint_info{
public:
	breakpoint_info(){};
	breakpoint_info(unsigned long input_code, unsigned long input_addr) : code(input_code), addr(input_addr){};
	unsigned long code;
	unsigned long addr;
};

class breakpoint_table{
public:
	breakpoint_info* operator[](unsigned long); // look up by addr
	breakpoint_info* operator[](int); // look up by idx
	void add(unsigned long, unsigned long);
	void del(int);
	bool include(unsigned long);
	bool include(int);
	int size();
	void show();
	int dict_addr2idx(unsigned long);
private:
	// since the all the breakpoint indices will be updated after one of the breakpoint is deleted
	// it's better to use vector instead of hash map
	// map<unsigned long, breakpoint_info*> data;
	// map<int, unsigned long> dict_idx2addr;
	vector<breakpoint_info> data;
};

int breakpoint_table::size(){
	return (int)data.size();
}

int breakpoint_table::dict_addr2idx(unsigned long addr){
	int tar_idx=-1;
	for(unsigned int i=0; i<data.size(); i++){
		if(data[i].addr == addr){
			tar_idx = (int)i;
			break;
		}
	}
	return tar_idx;
}

breakpoint_info* breakpoint_table::operator[](int tar_idx){
	return &data[tar_idx];
}

breakpoint_info* breakpoint_table::operator[](unsigned long tar_addr){
	int tar_idx = dict_addr2idx(tar_addr);
	return &data[tar_idx];
}

void breakpoint_table::add(unsigned long addr, unsigned long code){
	data.push_back(breakpoint_info(code, addr));
}

void breakpoint_table::del(int tar_idx){
	unsigned long tar_addr = data[tar_idx].addr;
	data.erase(data.begin()+tar_idx);
	fprintf(stdout, "** delete breakpoint %d @ %lx\n", tar_idx, tar_addr);
}

bool breakpoint_table::include(int idx){
	return (idx>=0 && idx<(int)data.size())? true : false;
}

bool breakpoint_table::include(unsigned long addr){
	int tar_idx = dict_addr2idx(addr); // if not found, tar_idx will be -1
	return include(tar_idx);
}

void breakpoint_table::show(){
	for(unsigned int i=0; i<data.size(); i++)
		fprintf(stdout, "%d: 0x%lx\n", i, data[i].addr);
}
