#pragma once
#include <unordered_map>
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
		fprintf(stdout, "breakpoint @\t 0x%" PRIx64 ": ", insn[0].address);
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
	breakpoint_info(unsigned long input_code) : code(input_code) { idx=count++;  }
	int idx;
	unsigned long code;
	static int count;
};
int breakpoint_info::count;

class breakpoint_table{
public:
	breakpoint_table();
	~breakpoint_table();
	breakpoint_info* operator[](unsigned long);
	void add(unsigned long, unsigned long);
	void del(int);
	bool include(unsigned long);
	void show();
private:
	// TODO: why not use int as key(if so ... maybe vector is a better choice?) with dict_addr2idx?
	map<unsigned long, breakpoint_info*> data;
	map<int, unsigned long> dict_idx2addr;
};

breakpoint_table::breakpoint_table(){
	breakpoint_info::count = 0;
}

breakpoint_table::~breakpoint_table(){
	for(auto it : data){
		delete (it.second);
	}
}

breakpoint_info* breakpoint_table::operator[](unsigned long tar_addr){
	return data[tar_addr];
}

void breakpoint_table::add(unsigned long addr, unsigned long code){
	data[addr] = new breakpoint_info(code);
}

void breakpoint_table::del(int tar_idx){
	unsigned long tar_addr = dict_idx2addr[tar_idx];
	breakpoint_info::count--;
	// TODO: update all the other breakpoints' index
	data.erase(tar_addr);
}

bool breakpoint_table::include(unsigned long addr){
	return data.count(addr);
}

void breakpoint_table::show(){
	for(auto it: data)
		fprintf(stdout, "%d: 0x%lx\n", it.second->idx, it.first);
}
