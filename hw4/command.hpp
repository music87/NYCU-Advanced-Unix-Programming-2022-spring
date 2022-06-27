#pragma once
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "parser.hpp"
#include "breakpoint.hpp"

#define N_DISASM_INS 10
#define N_DUMP_AMT 2
#define N_DUMP_SIZE 80

using namespace std;
class debugger{
public:
	debugger();
	void set_break_point(vector<string>);
	void help();
	void cont();
	void delete_break_point(vector<string>);
	void disasm(vector<string>);
	void dump(vector<string>);
	void release();
	void get_single_reg(vector<string>);
	void get_all_regs();
	void load(vector<string>);
	void run();
	void vmmap();
	void set_reg(vector<string>);
	void step_in();
	void start();
	void list();

private:
	enum state_t{ LOADED, NOLOAD, RUNNING };
	enum candbp_t{ FOR_CONT, FOR_SINGLESTEP, FOR_START };
	bool last_cmd_is_set_rip;
	unsigned long long reg_handler(string, unsigned long long, struct user_regs_struct&);
	void check_hit_break_point_or_not(candbp_t);
	void check_then_restore_si_reput(unsigned long long);
	void wait_then_check_exit();
	void reput_all_breakpoints_from(unsigned long long begin_addr);
	breakpoint_table bptab;
	disassembler disasm_handler;
	string prog_path;
	unsigned long long entry_point;
	unsigned long long text_begin, text_end;
	pid_t pid;
	state_t state;
};

debugger::debugger(){
	state = NOLOAD;
	last_cmd_is_set_rip = false;
	pid = -1;
	prog_path = "";
	entry_point = -1;
	text_begin = -1;
	text_end = -1;
}

void debugger::release(){
	pid = -1;
	last_cmd_is_set_rip = false;
	state = LOADED;
	// we hope to maintain bptab in the future, hence we don't clear bptab here
}

void debugger::list(){
	bptab.show();
}

void debugger::dump(vector<string> cmds){
	// the output should include the machine code cc if there is a break point
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(cmds.size() < 2){
		fprintf(stderr, "** no addr is given\n");
		return;
	}
	unsigned long long tar_addr = strtol(cmds.at(1).c_str(), NULL, 0);
	for(int i=0; i<N_DUMP_SIZE; i+=N_DUMP_AMT*sizeof(unsigned long long)){
		fprintf(stdout, "\t0x%llx: ", tar_addr);
		for(size_t j=0; j<N_DUMP_AMT; j++){
			unsigned long long code = ptrace(PTRACE_PEEKTEXT, pid, tar_addr+j*sizeof(unsigned long long), 0); // 8 byte once
			unsigned char *code_ptr = (unsigned char*) &code;
			for(size_t k=0; k<sizeof(unsigned long long); k++)
				fprintf(stdout, "%2.2x ", code_ptr[k]);
		}
		fprintf(stdout, "\t |");
		for(size_t j=0; j<N_DUMP_AMT; j++){
			unsigned long long code = ptrace(PTRACE_PEEKTEXT, pid, tar_addr+j*sizeof(unsigned long long), 0); // 8 byte once
			unsigned char *code_ptr = (unsigned char*) &code;
			for(size_t k=0; k<sizeof(unsigned long long); k++){
				if(!isprint(code_ptr[k])) fprintf(stdout, ".");
				else fprintf(stdout, "%c", code_ptr[k]);
			}
		}
		fprintf(stdout, "|\n");
		tar_addr += N_DUMP_AMT*sizeof(unsigned long long);
	}
}

void debugger::get_all_regs(){
	struct user_regs_struct regs;
	if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0){
		perror("** ptrace GETREGS");
		return;
	}
	fprintf(stdout, "RAX\t%llx\tRBX\t%llx\tRCX\t%llx\tRDX\t%llx\t\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
	fprintf(stdout, "R8\t%llx\tR9\t%llx\tR10\t%llx\tR11\t%llx\t\n", regs.r8, regs.r9, regs.r10, regs.r11);
	fprintf(stdout, "R12\t%llx\tR13\t%llx\tR14\t%llx\tR15\t%llx\t\n", regs.r12, regs.r13, regs.r14, regs.r15);
	fprintf(stdout, "RDI\t%llx\tRSI\t%llx\tRBP\t%llx\tRSP\t%llx\t\n", regs.rdi, regs.rsi, regs.rbp, regs.rsp);
	fprintf(stdout, "RIP\t%llx\tFLAGS\t%016llx\n", regs.rip, regs.eflags);
}

unsigned long long debugger::reg_handler(string tar_reg_t, unsigned long long input_val, struct user_regs_struct& regs){
	unsigned long long *tar_reg;
	if(tar_reg_t == "r15") tar_reg = &(regs.r15);
	else if(tar_reg_t == "r14") tar_reg = &(regs.r14);
	else if(tar_reg_t == "r13") tar_reg = &(regs.r13);
	else if(tar_reg_t == "r12") tar_reg = &(regs.r12);
	else if(tar_reg_t == "r11") tar_reg = &(regs.r11);
	else if(tar_reg_t == "r10") tar_reg = &(regs.r10);
	else if(tar_reg_t == "r9") tar_reg = &(regs.r9);
	else if(tar_reg_t == "r8") tar_reg = &(regs.r8);
	else if(tar_reg_t == "rbp") tar_reg = &(regs.rbp);
	else if(tar_reg_t == "rsp") tar_reg = &(regs.rsp);
	else if(tar_reg_t == "rsi") tar_reg = &(regs.rsi);
	else if(tar_reg_t == "rdi") tar_reg = &(regs.rdi);
	else if(tar_reg_t == "rip") tar_reg = &(regs.rip);
	else if(tar_reg_t == "rax") tar_reg = &(regs.rax);
	else if(tar_reg_t == "rbx") tar_reg = &(regs.rbx);
	else if(tar_reg_t == "rcx") tar_reg = &(regs.rcx);
	else if(tar_reg_t == "rdx") tar_reg = &(regs.rdx);
	else if(tar_reg_t == "flags") tar_reg = &(regs.eflags);
	else{
		fprintf(stderr, "** no such register\n");
		return -1;
	}
	if(input_val == 0){ // => for_get instead of set;
		return *tar_reg;
	} else {
		*tar_reg = input_val;
		return 0;
	}
}
void debugger::get_single_reg(vector<string> cmds){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(cmds.size() < 2){
		fprintf(stderr, "** no register is given\n");
		return;
	}
	struct user_regs_struct regs;
	if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0){
		perror("** ptrace GETREGS");
		return;
	}
	string tar_reg_t = cmds.at(1);
	unsigned long long tar_reg_val = reg_handler(tar_reg_t, 0, regs);
	if(tar_reg_val == (unsigned long long)-1)
		return; // some errors occured
	fprintf(stdout, "%s = %lld (0x%llx)\n", tar_reg_t.c_str(), tar_reg_val, tar_reg_val);
}

void debugger::set_reg(vector<string> cmds){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(cmds.size() < 3){
		fprintf(stderr, "** Not enough input arguments\n");
		return;
	}
	string tar_reg_t = cmds.at(1);
	unsigned long long input_val = strtol(cmds.at(2).c_str(), NULL, 0);
	struct user_regs_struct regs;
	if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0){
		perror("** ptrace GETREGS");
		return;
	}
	if(reg_handler(tar_reg_t, input_val, regs) == (unsigned long long)-1)
		return; // some errors occured
	if(ptrace(PTRACE_SETREGS, pid, 0, &regs) != 0){
		perror("** ptrace SETREGS");
		return;
	}
	if(tar_reg_t == "rip") last_cmd_is_set_rip = true;
}


void debugger::check_then_restore_si_reput(unsigned long long addr){
	if(!bptab.include(addr))
		return;
	// restore code (one byte)
	unsigned long long cur_code = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	if(ptrace(PTRACE_POKETEXT, pid, addr, (cur_code & 0xffffffffffffff00) | (bptab[addr]->code & 0x00000000000000ff)) != 0){
		perror("** ptrace POKETEXT");
		return;
	}
	// single step
	if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0) != 0){
		perror("** ptrace SINGLESTEP");
		return;
	}
	wait_then_check_exit(); if(state != RUNNING) return;
	// reput 0xcc (one byte)
	if(ptrace(PTRACE_POKETEXT, pid, addr, cur_code) != 0){
		perror("** ptrace POKETEXT");
		return;
	}
}

void debugger::wait_then_check_exit(){
	int status;
	if(waitpid(pid, &status, 0) < 0){
		perror("** waitpid");
		return;
	}
	// child process exit case
	if (WIFEXITED(status)) {
		fprintf(stderr, "** child process %d terminiated normally (code %d)\n", pid, status);
		release();
		return;
	}
	assert(WIFSTOPPED(status));
}

void debugger::check_hit_break_point_or_not(candbp_t cbpt){
	// cont may 1) stop at the byte "after" 0xcc or 2) normaly exit
	// step_in will 1) stop at the starting byte of next instruction which may also be the byte "on" 0xcc 2) normaly exit
	// now lets determine whether the break points are hit
	// child process stopped case. check whether the child process hit the breakpoint
	struct user_regs_struct regs; if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0) {perror("** ptrace GETREGS"); return;}
	unsigned long long addr = (cbpt == FOR_CONT)? regs.rip-1 : regs.rip;
	if(!bptab.include(addr)){
		fprintf(stdout, "** hit no breakpoint\n");
		return;
	}
	if(cbpt == FOR_CONT) {
		// set rip = rip - 1
		regs.rip = regs.rip - 1;
		if(ptrace(PTRACE_SETREGS, pid, 0, &regs) != 0){
			perror("** ptrace SETREGS");
			return;
		}
	}
	fprintf(stdout, "** breakpoint @");
	disasm_handler.translate(addr, (unsigned char*)&(bptab[addr]->code));
}

void debugger::help(){
	string message = "- break {instruction-address}: add a break point\n"
		"- cont: continue execution\n"
		"- delete {break-point-id}: remove a break point\n"
		"- disasm addr: disassemble instructions in a file or a memory region\n"
		"- dump addr: dump memory content\n"
		"- exit: terminate the debugger\n"
		"- get reg: get a single value from a register\n"
		"- getregs: show registers\n"
		"- help: show this message\n"
		"- list: list break points\n"
		"- load {path/to/a/program}: load a program\n"
		"- run: run the program\n"
		"- vmmap: show memory layout\n"
		"- set reg val: get a single value to a register\n"
		"- si: step into instruction\n"
		"- start: start the program and stop at the first instruction\n";
	cout <<  message;
}

void debugger::vmmap(){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	map<range_t, map_entry_t> vmmap;
	if(maps_parser(pid, vmmap) < 0){
		fprintf(stderr, "** fail to load memory mappings\n");
		return;
	}
}

void debugger::disasm(vector<string> cmds){
	// the output should not have the machine code cc
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(cmds.size() < 2){
		fprintf(stderr, "** no addr is given\n");
		return;
	}
	unsigned long long tar_addr = strtol(cmds.at(1).c_str(), NULL, 0);
	if(tar_addr < text_begin || tar_addr > text_end){
		fprintf(stderr, "** the address is out of the range of the text segment [0x%llx - 0x%llx]\n", text_begin, text_end);
		return;
	}
	for(int i=0; i<N_DISASM_INS && tar_addr>=text_begin && tar_addr <=text_end; i++){
		unsigned long long code;
		if(bptab.include(tar_addr))
			code = bptab[tar_addr]->code;
		else
			code = ptrace(PTRACE_PEEKTEXT, pid, tar_addr, 0);

		size_t count = disasm_handler.translate(tar_addr, (unsigned char*)&code);
		tar_addr += count;
	}
}

void debugger::load(vector<string> cmds){
	// set state and prog_path
	if(state == LOADED || state == RUNNING){
		fprintf(stderr, "** state must be NOT LOADED\n");
		return;
	}
	if(cmds.size() < 2){
		fprintf(stderr, "** no program path is given\n");
		return;
	}
	// load
	prog_path = cmds.at(1);
	if((elf64_parser(prog_path, entry_point, text_begin, text_end)) == -1) // some errors have happened
		return;
	fprintf(stdout, "** program '%s' loaded. entry point 0x%llx \n", prog_path.c_str(), entry_point);
	state = LOADED;
	last_cmd_is_set_rip = false;
}

void debugger::reput_all_breakpoints_from(unsigned long long begin_addr){
	// for debugger::start(), debugger::set_reg()
	for(int i=0; i<bptab.size(); i++){
		if(bptab[i]->addr < begin_addr) continue;
		// get the original code at target address
		unsigned long long code = ptrace(PTRACE_PEEKTEXT, pid, bptab[i]->addr, 0);
		// set break point by 0xcc (one byte)
		if(ptrace(PTRACE_POKETEXT, pid, bptab[i]->addr, (code & 0xffffffffffffff00) | 0xcc) != 0){
			perror("** ptrace POKETEXT");
			return;
		}
	}
}

void debugger::run(){
	if(state != LOADED && state != RUNNING){
		fprintf(stderr, "** state must be LOADED or RUNNING\n");
		return;
	}
	if(state == LOADED){
		start();
		cont();
	}
	else if(state == RUNNING){
		fprintf(stderr, "** program %s is already running\n", prog_path.c_str());
		cont();
	}
	last_cmd_is_set_rip = false;
}

void debugger::start(){
	// set pid, state, and then fork

	if(state != LOADED){
		fprintf(stderr, "** state must be LOADED\n");
		return;
	}
	if((pid=fork()) == -1){
		// some errors have happened
		perror("** fork");
	} else if(pid == 0) {
		// child process
		if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0){
			perror("** ptrace TRACEME");
			return;
		}
		char *argv[] = {(char*)prog_path.c_str(), NULL};
		execvp(argv[0], argv);
		perror("** execvp");
		return;
	} else {
		// parent process
		int status;
		if(waitpid(pid, &status, 0) < 0){
			perror("** waitpid");
			return;
		}
		assert(WIFSTOPPED(status));
		ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);
		reput_all_breakpoints_from(text_begin);
		check_hit_break_point_or_not(FOR_START);

		fprintf(stdout, "** pid %d\n", pid);
		state = RUNNING;
		last_cmd_is_set_rip = false;
	}
}

void debugger::step_in(){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	// get rip
	struct user_regs_struct regs; if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0) {perror("** ptrace GETREGS"); return;}
	if(bptab.include(regs.rip)){
		// restore code, single step, reput 0xcc
		check_then_restore_si_reput(regs.rip); if(state != RUNNING) return;
	} else{
		// single step
		if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0) != 0){ perror("** ptrace SINGLESTEP"); return; }
		wait_then_check_exit(); if(state != RUNNING) return;
	}
	check_hit_break_point_or_not(FOR_SINGLESTEP);
	last_cmd_is_set_rip = false;
}

void debugger::cont(){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	// get rip, check, set rip = rip - 1, restore code, singel step, reput 0xcc
	struct user_regs_struct regs; if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0) {perror("** ptrace GETREGS"); return;}
	if(!last_cmd_is_set_rip && bptab.include(regs.rip)){
		check_then_restore_si_reput(regs.rip); if(state != RUNNING) return;
	}
	// cont
	if(ptrace(PTRACE_CONT, pid, 0, 0) != 0){ perror("** ptrace CONT"); return; }
	wait_then_check_exit(); if(state != RUNNING) return;
	check_hit_break_point_or_not(FOR_CONT);
	last_cmd_is_set_rip = false;
}

void debugger::delete_break_point(vector<string>cmds){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(cmds.size() < 2){
		fprintf(stderr, "** no break-point-id is given\n");
		return;
	}
	int tar_idx = atoi(cmds.at(1).c_str());
	if(!bptab.include(tar_idx)){
		fprintf(stderr, "** breakpoint %d does not exist\n", tar_idx);
		return;
	}
	// restore original code
	unsigned long long cur_code = ptrace(PTRACE_PEEKTEXT, pid, bptab[tar_idx]->addr, 0);
	if(ptrace(PTRACE_POKETEXT, pid, bptab[tar_idx]->addr, (cur_code & 0xffffffffffffff00) | (bptab[tar_idx]->code & 0x00000000000000ff)) != 0){
		perror("** ptrace POKETEXT");
		return;
	}
	bptab.del(tar_idx);
}

void debugger::set_break_point(vector<string>cmds){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(cmds.size() < 2){
		fprintf(stderr, "** no address is given\n");
		return;
	}
	unsigned long long tar_addr = strtol(cmds.at(1).c_str(), NULL, 0);
	if(tar_addr < text_begin || tar_addr > text_end){
		fprintf(stderr, "** the address is out of the range of the text segment [0x%llx - 0x%llx]\n", text_begin, text_end);
		return;
	}
	if(bptab.include(tar_addr)){
		fprintf(stderr, "** the breakpoint is already exists. (breakpoint %d)\n", bptab.dict_addr2idx(tar_addr));
	}

	// get the original code at target address
	unsigned long long code = ptrace(PTRACE_PEEKTEXT, pid, tar_addr, 0);
	// set break point by 0xcc (one byte)
	if(ptrace(PTRACE_POKETEXT, pid, tar_addr, (code & 0xffffffffffffff00) | 0xcc) != 0){
		perror("** ptrace POKETEXT");
		return;
	}
	// record break point
	// though the code (8 byte) may fill with 0xcc, actually we only need that particular one byte where 0xcc is put on
	bptab.add(tar_addr, code);
	// comment out because of bptab's index may change when delete some breakpoints and hence will cause confuse //fprintf(stdout, "** breakpoint %d @ 0x%llx\n", bptab[tar_addr]->idx, tar_addr);
}
