#pragma once
#include <unistd.h>
#include <regex>
#include <stdio.h>
#include <string>
using namespace std;

bool argument_parser(int argc, char* argv[], regex &filter_command, regex &filter_type, regex &filter_file_name){
	// argv is an array composed of many pointers
	// hence if you mutate argv here, the argv outside will also be mutated
	int option;
	while((option=getopt(argc, argv, "c:t:f:")) != -1){

		switch(option){
			case 'c':
				// a regular expression (REGEX) filter for filtering command line
				filter_command.assign(string(optarg), regex::ECMAScript);
				break;
			case 't':
				// a TYPE filter. Valid TYPE includes  REG,  CHR,  DIR,  FIFO,  SOCK, and  unknown. TYPEs other than the listed should be considered invalid. For invalid types, your program has to print out an error message Invalid TYPE option. in a single line and terminate your program.
				if(!regex_match(string(optarg), filter_type)){
					printf("Invalid TYPE option\n");
					return false;
				}
				filter_type.assign(string(optarg), regex::ECMAScript);
				break;
			case 'f':
				// a regular expression (REGEX) filter for filtering filenames
				filter_file_name.assign(string(optarg), regex::ECMAScript);
				break;
			case '?':
			case ':':
			default:
				//printf("bad option: %c\n", option);
				break;
		}
	}

	argc -= optind;
	argv += optind;
	//for(int i=0;i<argc;i++)
	//	printf("argv[%d]=%s\n", i, argv[i]);
	return true;
}
