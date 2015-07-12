#pragma once
#include <stdbool.h>

void error(const char* msg, bool isSysError);
void nf_error(const char* msg, bool isSysError);
char* trimWhiteSpace(char *str);
void printByte(char byte);
