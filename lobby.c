#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/poll.h>

#include "lobby.h"
#include "wsgs.h"
#include "server.h"
#include "websocket.h"
#include "common.h"

typedef struct lobby{
	int size;
	int curNum;
	Player** players;
	pthread_mutex_t playerLock;
	Player** playerQueue;
	int queueSize;
	int curNumInQueue;
} Lobby;

void* gameLoop();
void _processMsg(Game* g, WebSocketMsg* wsm, int playerId);

Lobby* init_lobby(int size){
	Lobby* result = (Lobby*) malloc(sizeof(Lobby));
	result->size = size;
	result->curNum = 0;
	result->players = (Player**) malloc(sizeof(Player*)*result->size);
	result->queueSize = size;
	result->playerQueue = (Player**) malloc(sizeof(Player*)*result->queueSize);
	result->curNumInQueue = 0;
	pthread_mutex_init(&(result->playerLock), NULL);
	return result;
}

void clear_lobby(Lobby* l){
	for(int i=0;i<l->curNum;i++){
		clear_player(l->players[i]);
	}
	free(l->players);
	for(int i=0;i<l->curNumInQueue;i++){
		clear_player(l->playerQueue[i]);
	}
	free(l->playerQueue);
	free(l);
}

//adds player to playerQueue
void lobby_addPlayer(Lobby* lob, Player* p){
	//Make this atomic
	pthread_mutex_lock(&(lob->playerLock));
	//check to see if there is room
	if(lob->curNumInQueue >= lob->queueSize){
		char* closeMsg = "Gamme room is full";
		WebSocketMsg* msg = websocket_close(closeMsg);
		server_write(player_getId(p), msg->bytes,msg->numBytes);
		server_close(player_getId(p));
		clear_player(p);
	} else {
		//add the player to lobby Queue
		lob->playerQueue[lob->curNumInQueue] = p;
		lob->curNumInQueue++;
	}

	//send message to user
	wsgs_sendSingleMessage(p, "serv:Joining lobby...\n");	
	printf("added player to lobbyQueue -- %d\n", player_getId(p));
	printf("%d players in lobbyQueue\n",lob->curNumInQueue);
	pthread_mutex_unlock(&(lob->playerLock));
}

//removes players from lobby (Not queue)
void lobby_removePlayer(Lobby* lob, Player* p){
	pthread_mutex_lock(&(lob->playerLock));
	//find player in list
	int index;
	for(index = 0;(lob->players[index] != p) && (index < lob->curNum); index++);
	//index is now playerIndex or lob->curNum (Player not found)
	if(index == lob->curNum){
		//no such player
		return;
	}
	//shift everyone past index down 1
	for(int i=index+1;i<lob->curNum;i++){
		lob->players[i-1] = lob->players[i];
	}
	lob->curNum--;
	printf("removed player from lobby -- %d\n", player_getId(p));
	printf("%d players in lobby\n",lob->curNum);
	pthread_mutex_unlock(&(lob->playerLock));
}

void* lobbyLoop(void* v_lobby){
		Lobby* lobby = (Lobby*) v_lobby;	
		int gameId = 1;
		int numEvents;
		int buffSize = 1024;
		char readBuffer[buffSize];	
		struct pollfd pfds[lobby->size]; //poll file descriptors 
		for(int i=0;i<lobby->size;i++){
			//clear/init all file descriptors
			pfds[i].fd = -1;
		}
	while(1){
		numEvents = 0;
		int curNum = lobby->curNum;
		
		//dequeue as many player as possible,
		//send message that they have joined the lobby
		pthread_mutex_lock(&(lobby->playerLock));
			//int numRemoved = 0;	
			for(int i=0;i<lobby->curNumInQueue && lobby->curNum < lobby->size;i++, lobby->curNum++){
				Player* curPlayer = lobby->playerQueue[i];
				int status =  wsgs_sendSingleMessage(curPlayer, "serv:Waiting for opponent...\n");
				if( status >= 0){	
					lobby->players[lobby->curNum] = curPlayer;
					pfds[lobby->curNum].fd = player_getId(curPlayer);
					pfds[lobby->curNum].events = POLLIN | POLLPRI; 
					//numRemoved++;	
					printf("added player to lobby -- %d\n", player_getId(curPlayer));
				}
				//remove the player from the lobby and slide everyone down
				for(int j=i+1;j<lobby->curNumInQueue;j++){
					lobby->playerQueue[j-1] = lobby->playerQueue[j];
				}
				lobby->curNumInQueue--;
				i--;
				printf("removed player from lobbyQueue -- %d\n", player_getId(curPlayer));
			}	
			printf("%d players in lobby\n",lobby->curNum);
			
			//the lobby is full and queue was not completely processed
			//slide players still in the queue down by 'numRemoved' spots;
			/*for(int i=numRemoved;i<lobby->curNumInQueue;i++){
				lobby->playerQueue[i-numRemoved] = lobby->playerQueue[i];
			}
	
			lobby->curNumInQueue = lobby->curNumInQueue - numRemoved;
			*/
			printf("%d players in lobbyQueue\n",lobby->curNumInQueue);
		pthread_mutex_unlock(&(lobby->playerLock));
		//process poll
		numEvents = poll(pfds, lobby->curNum, 10000);
		if(numEvents == -1){
			//An error occured
			nf_error("poll_failed-waiting on players to join\n",true);
		} else if(numEvents == 0){
			//Timeout occured,lets heartbeat with the player, make sure they are still there
		} else {
			int eventsFound = 0;
			for(int i=0;eventsFound < numEvents;i++){
				if(pfds[i].revents & POLLIN){
					memset(readBuffer, 0, buffSize);	
					int n = server_recv(pfds[i].fd, readBuffer, buffSize);	
					if(n < 0){
						nf_error("read failed, do not process - in lobby\n",true);
					} else if(n == 0) {
						//the player has left, remove from lobby and poll;
						//TODO:
						Player* toRemove = NULL;
						for(int j=0;j < curNum && toRemove == NULL;j++){
							Player* temp = lobby->players[j];
							if(player_getId(temp) == pfds[i].fd){
								toRemove = temp;
							}
						}	
						if(toRemove != NULL){
							lobby_removePlayer(lobby, toRemove);
							pfds[i].fd = -1;
							printf("Player left the lobby\n");
							clear_player(toRemove);
						}
					} else {
						WebSocketMsg* wsm = init_webSocketMsg();
						websocket_setBytes(wsm, readBuffer, n);
						websocket_decode(wsm);
						switch(wsm->opcode){
							case WS_OP_BIN:
								break;
							case WS_OP_TEXT:
								//ignore currently - no chatting in lobby
								break;
							case WS_OP_CLOSE:
								{ //create scope
									//remove player from lobby
									Player* toRemove = NULL;
									for(int j=0;j < lobby->curNum && toRemove == NULL;j++){
										Player* temp = lobby->players[j];
										if(player_getId(temp) == pfds[i].fd){
											toRemove = temp;
										}
									}	
									if(toRemove != NULL){
										lobby_removePlayer(lobby, toRemove);
										pfds[i].fd = -1;
										printf("Player left the lobby\n");
										clear_player(toRemove);
									}
								}
								break;
							default:
								//nothing yet
								;
						}
						clear_webSocketMsg(wsm);
					}
					eventsFound++;
				}
			}
		}
		//add players to games
		//put everyone in the lobby in a game
		while(lobby->curNum >= game_getQuorum()){
			printf("Starting game!\n");
			Game* g = init_game(gameId);
			for(int i=0;i<game_getMaxPlayers(g) && lobby->curNum > 0;i++){
				game_addPlayer(g, lobby->players[0]);
				bool found = false;
				//remove fd from poll
				for(int j=0;j<curNum && !found;j++){
					if(player_getId(lobby->players[0]) == pfds[j].fd){
						pfds[j].fd = -1;
						found = true;
					}
				}
				//remove player from lobby
				lobby_removePlayer(lobby, lobby->players[0]);
			}
			//start the game
			wsgs_sendMessage(g, "serv:Starting game...\n");	
			printf("About to start pthread!\n");
			//start the game!
			pthread_t t;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);//prevent memory leaks
			pthread_create(&t, &attr, gameLoop, (void*) g);	
			gameId++;
		}
		//shift pfds
		//adjust poll(take out negative numbers)
		int offset = 0;
		for(int i=0;i<curNum;i++){
			if(pfds[i].fd == -1){
				offset++;
			}else {
				pfds[i-offset] = pfds[i];
			}
		}
		
	}
	return NULL;
}
