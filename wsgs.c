/******
 *
 * WebSocket GameServer (wsgs)
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/poll.h>

#include "wsgs.h"
#include "websocket.h"
#include "server.h"
#include "lobby.h"
#include "common.h"

//"private" functions
void* gameLoop();
void _processMsg(Game* g, WebSocketMsg* wsm, int playerId);
void _processClose(Game* game, int id, bool* endGame);

void serverLoop(int sockfd, struct sockaddr_in cli_addr, socklen_t clilen, bool* endFlag, void* lobby){
	Lobby* lob = (Lobby*) lobby;
	int bufSize = 1024;
	char buffer[bufSize]; //overhead of creating this everyloop, might move
	Player* player;
	int playerId;
	WebSocketMsg* response;
	playerId = accept(sockfd, 
			(struct sockaddr *) &cli_addr, 
			&clilen);
	if (playerId < 0){
		if(endFlag){
			return; //someone else changed endFlag - let someone else deal with cleanup
		}

		if(server_close(sockfd) != 0){
			error("ERROR: Unable to  server sock",true);
		}
		error("ERROR: could not accept client connection",true);
	}

	memset(buffer,0,bufSize);
	server_read(playerId,buffer,bufSize);

	//handshake 
	response = websocket_handshake(buffer, bufSize);
	if(response == NULL){
		//bad handshake, ignore this player
		server_close(playerId);
		return;
	}
	server_write(playerId,response->msg,response->msgSize);
	clear_webSocketMsg(response);
	//handshake complete 

	//create and add player to game
	player = init_player(playerId);
	//add to lobby
	lobby_addPlayer(lob, player);
}

void* gameLoop(void* param){
	Game* game = (Game*) param;
	int readBufferSize = 1024;
	char readBuffer[readBufferSize];
	struct pollfd pfds[game_getCurNumPlayers(game)]; //poll file descriptors 
	int numEvents = 0;
	bool endGame = false;
	WebSocketMsg* wsm = init_webSocketMsg();
	for(int i=0;i<game_getCurNumPlayers(game);i++){
		Player* curPlayer = game_getPlayer(game, i);
		pfds[i].fd = player_getId(curPlayer);
		pfds[i].events = POLLIN | POLLPRI; 
		//lets give the players some names
		int nameBuff = 10;
		if(i<999999999){
			char name[nameBuff];
			sprintf(name,"%d",i);
			player_setName(curPlayer, name, strlen(name));
			char sendString[nameBuff+20];
			sprintf(sendString,"serv:You are player %d",i+1);
			wsgs_sendSingleMessage(curPlayer,sendString);
		}
	}
	
//DEBUG
	printf("New Thread for: %d\n",game_getId(game));
//END DEBUG
	//send initial game state to clients
	wsgs_sendMessage(game, game_getStateString(game));
	
	while(!endGame){
		numEvents = poll(pfds, game_getCurNumPlayers(game), 1000);
		if(numEvents == -1){
			//An error occured, but we shouldn't kill the application, just the thread
			nf_error("poll failed, exiting game\n",true);
			endGame = true;
		} else if(numEvents == 0){
			//Timeout occured, lets heartbeat to make sure everyone is still connected
/*
			printf("Pinging\n");
			WebSocketMsg* pinger = websocket_ping("");
			for(int i=0;i<game->numPlayers;i++){
				server_write(game->players[i]->fd,pinger->bytes,pinger->numBytes);
			}
			clear_webSocketMsg(pinger);
*///Temp stop pinging
		} else {
			//numEvent is positve
			int eventsFound = 0;
			for(int i=0;eventsFound < numEvents;i++){
				if(pfds[i].revents & POLLIN){
					memset(readBuffer,0,readBufferSize);
					int n = server_recv(pfds[i].fd,readBuffer, readBufferSize);
					if(n < 0){
						nf_error("read failed, do not process\n", true);
					} else {
						websocket_setBytes(wsm, readBuffer, n);
						websocket_decode(wsm);	
						switch(wsm->opcode){
							case WS_OP_BIN:
								break;
							case WS_OP_TEXT:
								_processMsg(game, wsm, pfds[i].fd);
								break;
							case WS_OP_PONG:
								//handle heartbeat
								break;
							case WS_OP_CLOSE:
								printf("Num Players: %d\n",game_getCurNumPlayers(game));
								_processClose(game,pfds[i].fd, &endGame);
								pfds[i].fd = -1; //remove player from wsgs
								break;
							default:
								//nothing for now
								;
						}//end switch
					}// end recieve processing
					eventsFound++;	
//DEBUG
printf("eventsFound:%d \t numEvents:%d",eventsFound, numEvents);
//END DEBUG
					//mem management
					clear_webSocketMsg(wsm);
					wsm = init_webSocketMsg(wsm);
				}//end event handling for pfds[i] 
			}//all revents processed that triggered poll
		}//dealt with poll return 
	}//game loop
	
	//clean up before exit
	clear_game(game);
	clear_webSocketMsg(wsm);
	printf("returning from pthread\n");	
	return NULL;
}

void _processMsg(Game* g, WebSocketMsg* wsm, int playerId){
	char* responseMsg = game_changeState(g, wsm->msg,wsm->msgSize, playerId);
	if(responseMsg != NULL){
		wsgs_sendMessage(g, responseMsg);
	} else {
		//check to see if it is a chat msg
		if(wsm->msgSize >= 5 &&
				wsm->msg[0] == 'c' &&
				wsm->msg[1] == 'h' &&
				wsm->msg[2] == 'a' && 
				wsm->msg[3] == 't' &&
				wsm->msg[4] == ':'){
			//send back original string with player name that sent the msg
			char* playerName = NULL;
			for(int i=0;i<game_getCurNumPlayers(g);i++){
				if(player_getId(game_getPlayer(g,i))==playerId){
					playerName = player_getName(game_getPlayer(g,i));
					break;
				}
			}
			int chatMsgSize = strlen(playerName) + wsm->msgSize+1;
			char* chatMsg = malloc(chatMsgSize*sizeof(char));
			sprintf(chatMsg,"chat:%s:%s",playerName,&wsm->msg[5]);
			wsgs_sendMessage(g, chatMsg);
			free(chatMsg);
		}
	}
}

void _processClose(Game* game, int id, bool* endGame){
	//end game code
	printf("Inside processClose");
	bool found = false;
	Player* p = NULL;
	//find player that left game
	for(int j=0;j<game_getCurNumPlayers(game) && !found;j++){
		if(player_getId(game_getPlayer(game,j)) == id){
			found = true;
			p = game_getPlayer(game,j);
		}
	}
	if(found){
		char* playerName = player_getName(p);
		char sendString[strlen(playerName)+31];
		sprintf(sendString,"serv:Player %d has left the game",atoi(playerName)+1);
		
		game_playerQuit(game, id, endGame);
		//let other players know
		for(int j=0;j<game_getCurNumPlayers(game);j++){
			Player* curPlayer = game_getPlayer(game, j);
			wsgs_sendSingleMessage(curPlayer,sendString);
			if(endGame){
				//let the player know the game is over
				wsgs_sendSingleMessage(curPlayer, "serv:The game is over, Thank you for playing");
			}
		}
	}
}

void wsgs_sendMessage(Game* g, const char* msg){
	WebSocketMsg* wsmResponse = websocket_msg(msg, strlen(msg), WS_OP_TEXT);
	for(int i=0;i<game_getCurNumPlayers(g);i++){
		int fd = player_getId(game_getPlayer(g,i));		
		server_write(fd, wsmResponse->bytes, wsmResponse->numBytes);
	}
	clear_webSocketMsg(wsmResponse);
}


int wsgs_sendSingleMessage(Player* p, const char* msg){
	int returnVal;
	WebSocketMsg* wsmResponse = websocket_msg(msg, strlen(msg), WS_OP_TEXT);
	returnVal = server_write(player_getId(p), wsmResponse->bytes, wsmResponse->numBytes);
	clear_webSocketMsg(wsmResponse);
	return returnVal;
}
