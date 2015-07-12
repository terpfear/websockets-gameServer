#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "server.h"
#include "lobby.h"

int main(int argc, char *argv[]){
	int portNum;
	//check command line arguments
	if (argc < 2){
		int size = 1000;
		char msg[size];
		snprintf(msg, size, "WebSocket v0.1\nA simple websocket server\nUsage: %s <port>\n",argv[0]);
		error(msg,false);
	}
	portNum = atoi(argv[1]);

	//create game lobby
	Lobby* l = init_lobby(100);
	pthread_t t;
	pthread_create(&t, NULL, lobbyLoop, (void*) l);	

	server_start(portNum, (void*) l);	
    return 0; 
}
