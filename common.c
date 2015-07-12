#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void error(const char *msg, bool isSysError){
    if(isSysError){
        perror(msg);
    } else {
        fprintf(stderr,"%s",msg);
    }
    exit(1);
}

//non_fatal error
void nf_error(const char *msg, bool isSysError){
    if(isSysError){
        perror(msg);
    } else {
        fprintf(stderr,"%s",msg);
    }
}

//This should only be used on stack strings
//makes the str pointer unable to be freed
char* trimWhiteSpace(char *str){
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

void printByte(char byte){
	for(int i=0;i<8;i++){
		char temp = (byte >> (7-i)) & 1;
		printf("%d: %d\n",i,temp);
	}
}
