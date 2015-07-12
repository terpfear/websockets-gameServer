#pragma once
#include <netinet/in.h>

extern void serverLoop(int sockfd, struct sockaddr_in cli_addr, socklen_t clilen, bool* endFlag, void* data);

void server_start(int port, void* data);
int server_write(int fd, const char* msg, int msgSize);
int server_recv(int fd, char* buffer, int bufferSize);
int server_read(int fd, char* buffer, int bufferSize);
int server_close(int fd);
