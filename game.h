#pragma once
typedef struct game Game;
typedef struct player Player;

extern Player* init_player();
extern void clear_player();
extern void player_setName(Player*, char*, int);
extern char* player_getName(Player*);
extern int player_getId(Player*);

Game* init_game(int id);
void clear_game();
int game_getQuorum();

