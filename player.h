#pragma once
typedef struct player Player;

Player* init_player(int id);
void clear_player(Player* p);
void player_setName(Player* p, char* name, int length);
char* player_getName(Player* p);
int player_getId(Player* p);
