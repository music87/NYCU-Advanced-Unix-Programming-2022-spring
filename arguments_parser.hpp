#pragma once
#include<unistd.h>
#include<regex>
void arguments_parser(int argc, char* argv[]){
	// argv is an array composed of many pointers
	// hence if you mutate argv here, the argv outside will also be mutated
	int option;
	while((option = getopt(argc, argv, "c:t:f:")) != -1){
		switch(option){
			case 'c':
				// a regular expression (REGEX) filter for filtering command line
				printf("option c: %s\n", optarg);
				break;
			case 't':
				// a TYPE filter. Valid TYPE includes  REG,  CHR,  DIR,  FIFO,  SOCK, and  unknown. TYPEs other than the listed should be considered invalid. For invalid types, your program has to print out an error message Invalid TYPE option. in a single line and terminate your program.
				printf("option t:%s \n", optarg);
				printf("Invalid TYPE option\n");
				break;
			case 'f':
				// a regular expression (REGEX) filter for filtering filenames
				printf("option f: %s\n", optarg);
				break;
			case '?':
			case ':':
			default:
				printf("bad option: %c\n", option);
				break;
		}
	}

	argc -= optind;
	argv += optind;
	for(int i=0;i<argc;i++)
		printf("argv[%d]=%s\n", i, argv[i]);



}






