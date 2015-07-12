#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "player.h"

typedef struct player{
	int id;
	char* name;
} Player;

Player* init_player(int id){
	Player* p = malloc(sizeof(Player));
	p->name = malloc(sizeof(char));
	p->name[0] = '\0';
	p->id = id;
	return p;
}

void clear_player(Player* p){
	free(p->name);
	free(p);
}

void player_setName(Player* p, char* name,int nameLen){
	free(p->name);
	p->name = malloc(nameLen * sizeof(char)+1);
	strncpy(p->name, name, nameLen);
	p->name[nameLen] = '\0'; //ensure null terminated string
}

char* player_getName(Player* p){
	return p->name;
}

int player_getId(Player* p){
	return p->id;
}
