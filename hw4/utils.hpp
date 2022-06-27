#pragma once
#include <string>
#include <stdio.h>
#define BUFFER_SIZE 256
using namespace std;

string ull2hexstr(unsigned long long int_val){
	char str_val[BUFFER_SIZE];
	sprintf(str_val, "%llx", int_val);
	return string(str_val);
}

