#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include "command.hpp"
#include "parser.hpp"

using namespace std;

int main(int argc, char* argv[]){
	istream *in;
	ifstream fin;
	debugger dbg;
	string line, script="", program="";
	setvbuf(stdout, NULL, _IONBF, 0);
	arg_parser(argc, argv, script, program);
	// handler script
	if(script != ""){
		fin.open(script.c_str());
		in = &fin;
	} else {
		in = &cin;
	}
	// handle program path loading
	if(program != ""){
		vector<string> cmds = dbgcmd_parser(("load "+program).c_str(), ' ');
		dbg.load(cmds);
	}

	if(script == "") cout << "sdb> ";
	while(getline(*in, line, '\n')){
		vector<string> cmds = dbgcmd_parser(line, ' ');
		if(cmds.at(0) == "exit" || cmds.at(0) == "q"){
			dbg.release();
			fin.close();
			exit(0);
		} else if(line.empty()){
			continue;
		} else if(cmds.at(0) == "help" || cmds.at(0) == "h"){
			dbg.help();
		} else if(cmds.at(0) == "load"){
			dbg.load(cmds);
		} else if(cmds.at(0) == "start"){
			dbg.start();
		} else if(cmds.at(0) == "cont" || cmds.at(0) == "c"){
			dbg.cont();
		} else if(cmds.at(0) == "break" || cmds.at(0) == "b"){
			dbg.set_break_point(cmds);
		} else if(cmds.at(0) == "list" || cmds.at(0) == "l"){
			dbg.list();
		} else if(cmds.at(0) == "si"){
			dbg.step_in();
		} else if(cmds.at(0) == "delete" || cmds.at(0) == "del"){
			dbg.delete_break_point(cmds);
		} else if(cmds.at(0) == "get" || cmds.at(0) == "g"){
			dbg.get_single_reg(cmds);
		} else if(cmds.at(0) == "getregs"){
			dbg.get_all_regs();
		} else if(cmds.at(0) == "set" || cmds.at(0) == "s"){
			dbg.set_reg(cmds);
		} else if(cmds.at(0) == "vmmap" || cmds.at(0) == "m"){
			dbg.vmmap();
		} else if(cmds.at(0) == "disasm" || cmds.at(0) == "d"){
			dbg.disasm(cmds);
		} else if(cmds.at(0) == "dump" || cmds.at(0) == "x"){
			dbg.dump(cmds);
		} else if(cmds.at(0) == "run" || cmds.at(0) == "r"){
			dbg.run();
		} else{
			fprintf(stderr, "** nothing happened\n");
		}
		if(script == "") cout << "sdb> ";
	}
	return 0;
}
