#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#include "server.h"
#include "common.h"

static bool dead = false;


void intHandler(){
	dead = true;
}


void server_start(int portNum, void* data){
	int sockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;



	//create TCP socket descriptor
		//AF_INET -- IPv4 Internet Protocol
		//SOCK_STREAM -- "reliable, two-way, connection-based byte streams" (TCP)
		//0 -- "Not used for Internet Sockets" https://csperkins.org/teaching/ns3/labs-intro.pdf
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		error("ERROR: opening socket", true);
	}


	//clear serv_addr before use
	memset((char *) &serv_addr,0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portNum);
	if(bind(sockfd, (struct sockaddr *) & serv_addr, 
				sizeof(serv_addr)) < 0){
		if(close(sockfd) != 0){
			error("ERROR: unable to bind. Unable to close",true);
		}
		error("ERROR: unable to bind", true);
	}

	listen(sockfd,5);
    clilen = sizeof(cli_addr);

	//register signal handler
	signal(SIGINT, intHandler);

//Crux of server
	while(!dead){
		serverLoop(sockfd, cli_addr, clilen, &dead, data);
	}

    if(server_close(sockfd) != 0){
		error("close",true);
	}
	printf("Succesfully exited program\n");
}

int server_recv(int fd, char* buffer, int bufferSize){
	//simple wrapper to keep all reads in one place
	int n;
	n = recv(fd, buffer, bufferSize, 0);
	return n;
}


int server_write(int fd, const char* msg, int msgSize){
	int n;
    n = write(fd,msg,msgSize);
    if (n < 0){
		 nf_error("ERROR: writing to socket", true);
	}
	return n;
}

int server_read(int fd, char* buffer,int bufferSize){
	int n;
	memset(buffer,0,bufferSize);
	n = read(fd,buffer,bufferSize-1);
	if (n < 0){
		error("ERROR: reading from socket",true);
	}	
	return n;
}

int server_close(int fd){
	//simple close wrapper to keep all server code in the same file
	return close(fd);
}
