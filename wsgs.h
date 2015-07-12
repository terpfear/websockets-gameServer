#pragma once
/******
 *
 * WebSocket GameServer (wsgs)
 * 
 */
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct game Game;
typedef struct player Player;

//while extern is implicit in all c function declarations,
//it is here to serve as a bit of fake polymorphism
//game is expected to be declared externally with at least these functions
extern Game* init_game(int id);
extern void clear_game(Game*);
extern int game_getId(Game*);
extern int game_getCurNumPlayers(Game*);
extern int game_getMaxPlayers(Game*);
extern int game_addPlayer(Game*, Player*);
extern char* game_getStateString(Game* g);
extern Player* game_getPlayer(Game* g, int index);
extern char* game_changeState(Game* g, char* msg, int msgSize, int fd);
extern void* game_playerQuit(Game* g, int fd, bool* endGame);

//player is expected to be declared externally with at least these functions
extern Player* init_player(int id);
extern int player_getId(Player*);
extern void player_setName(Player*, char*, int);
extern char* player_getName(Player*);

void serverLoop(int sockfd, struct sockaddr_in cli_addr, socklen_t clilen, bool* endFlag, void* lobby);
void wsgs_sendMessage(Game* g, const char* msg);
int wsgs_sendSingleMessage(Player* p, const char* msg);
