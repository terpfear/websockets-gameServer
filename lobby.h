#pragma once 

typedef struct player Player;
typedef struct lobby Lobby;

extern void clear_player(Player*);
extern int game_getQuorum();

Lobby* init_lobby(int size);
void clear_lobby(Lobby*);
void* lobbyLoop(void* lobby); //thread loop
void lobby_addPlayer(Lobby* l, Player* p);
