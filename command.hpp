#pragma once
#include <stdio.h>
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

#define LOADED 0x1
#define NOLOAD 0x2
#define RUNNING 0x3

using namespace std;
class debugger{
public:
	debugger();
	void set_break_point(vector<string>);
	void help();
	void cont();
	void delete_break_point(vector<string>);
	void disasm();
	void dump();
	void release();
	void get_single_reg();
	void get_all_regs();
	void load(vector<string>);
	void run();
	void vmmap();
	void set_reg(vector<string>);
	void step_in();
	void start();
	void list();

private:
	enum candbp_t{ FOR_CONT, FOR_SINGLESTEP };
	bool tmp;
	void check_and_handle_breakpoint(candbp_t);
	void reset_all_breakpoints_from(unsigned long begin_addr);
	breakpoint_table bptab;
	disassembler disasm_handler;
	string prog_path;
	unsigned long long entry_point;
	unsigned long long text_begin, text_end;
	pid_t pid;
	int state;
};

debugger::debugger(){
	state = NOLOAD;
	pid = -1;
	prog_path = "";
	entry_point = -1;
	text_begin = -1;
	text_end = -1;
}

void debugger::release(){
	pid = -1;
	state = LOADED;
	// we hope to maintain bptab in the future, hence we don't clear bptab here
}

void debugger::list(){
	bptab.show();
}

void debugger::dump(){



}

void debugger::check_and_handle_breakpoint(candbp_t cbpt){
	// cont may 1) stop at the byte "after" 0xcc or 2) normaly exit
	// step_in will 1) stop at the starting byte of next instruction which may also be the byte "on" 0xcc 2) normaly exit
	// now lets determine whether the break points are hit
	// wait until child process' status change
	int status;
	unsigned long bp_candidate;
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
	// child process stopped case. only care about that child process encounter breakpoint
	assert(WIFSTOPPED(status));
	struct user_regs_struct regs;
	if(ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0){
		perror("** ptrace GETREGS");
		return;
	}
	bp_candidate = (cbpt == FOR_CONT)? regs.rip-1 : regs.rip;
	if(!bptab.include(bp_candidate)){
		fprintf(stdout, "** hit no breakpoint, rip @ 0x%llx\n", regs.rip);
		return;
	}
	// child process hit one breakpoint. restore original code (one byte)
	unsigned long cur_code = ptrace(PTRACE_PEEKTEXT, pid, bp_candidate, 0);
	if(ptrace(PTRACE_POKETEXT, pid, bp_candidate, (cur_code & 0xffffffffffffff00) | (bptab[bp_candidate]->code & 0x00000000000000ff)) != 0){
		perror("** ptrace POKETEXT");
		return;
	}
	disasm_handler.translate(bp_candidate, (unsigned char*)&(bptab[bp_candidate]->code));
	if(cbpt == FOR_CONT) regs.rip = regs.rip-1; // return rip to before exec 0xcc (now is restore to its original byte)
	if(ptrace(PTRACE_SETREGS, pid, 0, &regs) != 0){
		perror("** ptrace SETREGS");
		return;
	}
	// TODO: put back 0xcc to avoid set rip
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
	//map<range_t, map_entry_t> vmmap;


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
}

void debugger::reset_all_breakpoints_from(unsigned long begin_addr){
	// for debugger::start(), debugger::set_regs()
	for(int i=0; i<bptab.size(); i++){
		if(bptab[i]->addr < begin_addr) continue;
		// get the original code at target address
		unsigned long code = ptrace(PTRACE_PEEKTEXT, pid, bptab[i]->addr, 0);
		// set break point by 0xcc (one byte)
		if(ptrace(PTRACE_POKETEXT, pid, bptab[i]->addr, (code & 0xffffffffffffff00) | 0xcc) != 0){
			perror("** ptrace POKETEXT");
			return;
		}
	}
}

void debugger::set_reg(vector<string> cmds){
	// TODO: reset_all_breakpoints_from();

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
		reset_all_breakpoints_from(text_begin);
		fprintf(stdout, "** pid %d\n", pid);
		state = RUNNING;
	}
}

void debugger::step_in(){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0) != 0){
		perror("** ptrace SINGLESTEP");
		return;
	}
	check_and_handle_breakpoint(FOR_SINGLESTEP);
}

void debugger::cont(){
	if(state != RUNNING){
		fprintf(stderr, "** state must be RUNNING\n");
		return;
	}
	if(ptrace(PTRACE_CONT, pid, 0, 0) != 0){
		perror("** ptrace CONT");
		return;
	}
	check_and_handle_breakpoint(FOR_CONT);
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
	unsigned long cur_code = ptrace(PTRACE_PEEKTEXT, pid, bptab[tar_idx]->addr, 0);
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
	unsigned long tar_addr = strtol(cmds.at(1).c_str(), NULL, 0);
	if(tar_addr < text_begin || tar_addr > text_end){
		fprintf(stderr, "** the address is out of the range of the text segment [0x%llx - 0x%llx]\n", text_begin, text_end);
		return;
	}
	if(bptab.include(tar_addr)){
		fprintf(stderr, "** the breakpoint is already exists. (breakpoint %d)\n", bptab.dict_addr2idx(tar_addr));
	}

	// get the original code at target address
	unsigned long code = ptrace(PTRACE_PEEKTEXT, pid, tar_addr, 0);
	// set break point by 0xcc (one byte)
	if(ptrace(PTRACE_POKETEXT, pid, tar_addr, (code & 0xffffffffffffff00) | 0xcc) != 0){
		perror("** ptrace POKETEXT");
		return;
	}
	// record break point
	// TODO: however, the code may fill with 0xcc ...
	bptab.add(tar_addr, code);
	// comment out because of bptab's index may change when delete some breakpoints and hence will cause confuse //fprintf(stdout, "** breakpoint %d @ 0x%lx\n", bptab[tar_addr]->idx, tar_addr);
}
