#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "game.h"
#include "common.h"

#define GAME_QUORUM 2

typedef struct game{
	int id;
	int curNumPlayers;
	int maxPlayers;
	Player** players; 
	int turnNumber;
	int state[9][9];
	int winner[10]; //9 small boards + the big board
	int activeBoard;
	int lastBoard;
	int lastSquare;
	char* stateString;
} Game;

static int wins[8][3] = {{0,1,2},
						{3,4,5},
						{6,7,8},
						{0,4,8},
						{2,4,6},
						{0,3,6},
						{1,4,7},
						{2,5,8}};

static int winHashMap[9][4] = { {0,3,5,-1},
								{0,6,-1,-1},
								{0,4,7,-1},
								{1,5,-1,-1},
								{1,3,4,6},
								{1,7,-1,-1},
								{2,4,5,-1},
								{2,6,-1,-1},
								{2,3,7,-1} };

void _stateToString(Game* g){
	//assumes everything is initialized properly
	int index = 0;
	//initial opening bracket for json array
	g->stateString[index++] = '[';
	//the 9 boards
	for(int i=0;i<9;i++){
		g->stateString[index++] = '[';
		//the 9 squares of the boards
		for(int j=0;j<9;j++){
			g->stateString[index++] = g->state[i][j] + 48;
			if(j<8){
				g->stateString[index++] = ',';
			}
		}
		g->stateString[index++] = ']';
		g->stateString[index++] = ',';
	}
	//the active board, last board, last square
	g->stateString[index++] = '[';
	int negOffset = 0;
	if(g->activeBoard < 0){
		g->stateString[index++] = '-';
		negOffset = -2 * g->activeBoard; //helps turn -1 to 1 
	}
	g->stateString[index++] = g->activeBoard + 48 + negOffset;
	g->stateString[index++] = ',';
	negOffset = 0;
	if(g->lastBoard < 0){
		g->stateString[index++] = '-';
		negOffset = 2; //helps turn -1 to 1 
	}
	g->stateString[index++] = g->lastBoard + 48 + negOffset;
	g->stateString[index++] = ',';
	negOffset = 0;
	if(g->lastSquare < 0){
		g->stateString[index++] = '-';
		negOffset = 2; //helps turn -1 to 1 
	}
	g->stateString[index++] = g->lastSquare + 48 + negOffset;
	

	g->stateString[index++] = ']';

	g->stateString[index++] = ',';
	//winners
	g->stateString[index++] = '[';
	for(int i=0;i<10;i++){
		g->stateString[index++] = g->winner[i] + 48;
		if(i<9){
			g->stateString[index++] = ',';
		}
	}
	g->stateString[index++] = ']';

	g->stateString[index++] = ']';
	g->stateString[index] = '\0';
	printf("State String: %s\n",g->stateString);
}

void _updateWinners(Game* g, int board,int square){
	bool newWinner = false;
	int curPlayer = g->turnNumber % g->curNumPlayers;
	curPlayer++; //start index = 1, not 0
	if(g->winner[board] & (curPlayer)){
		//this player has already won this board, 
		//no further checking required
		return;
	}
	for(int i=0;i<4 && !newWinner && winHashMap[square][i] != -1;i++){
		int* winCombo = wins[winHashMap[square][i]];
		int* curBoard = g->state[board];
		if(curBoard[winCombo[0]] == curPlayer &&
				curBoard[winCombo[1]] == curPlayer &&
				curBoard[winCombo[2]] == curPlayer){
			g->winner[board] = g->winner[board] | (curPlayer);
			newWinner = true;
		}
	}
	if(newWinner){
		//check the large board for a winner
		newWinner = false;
		for(int i=0;i<4 && !newWinner && winHashMap[board][i] != -1;i++){
			int* winCombo = wins[winHashMap[board][i]];
			int* curBoard = g->winner;
			if((curBoard[winCombo[0]] & curPlayer) == curPlayer &&
					(curBoard[winCombo[1]] & curPlayer) == curPlayer &&
					(curBoard[winCombo[2]] & curPlayer) == curPlayer){
				//The game has a winner!
				g->winner[9] = curPlayer;
				newWinner = true;
			}
		}
	}
	
}

Game* init_game(int id){
	Game* result = malloc(sizeof(Game));
	result->id = id;
	result->maxPlayers = 2;
	result->curNumPlayers = 0;
	result->players = malloc(result->maxPlayers * sizeof(Player*));
	result->turnNumber = 0;
	//tic tac toe part
	for(int i=0;i<9;i++){
		for(int j=0;j<9;j++){
			result->state[i][j] = 0;
		}
	}
	for(int i=0;i<10;i++){
		result->winner[i] = 0;
	}
	result->activeBoard = -1;
	result->lastBoard = -1;
	result->lastSquare = -1;
	result->stateString = malloc(212*sizeof(char));
	_stateToString(result);
	return result;
}

void clear_game(Game* g){
	for(int i=0;i<g->curNumPlayers;i++){
		clear_player(g->players[i]);
	}
	free(g->stateString);
	free(g);
}

int game_getQuorum(){
	return GAME_QUORUM;
}


int game_getId(Game* g){
	return g->id;
}

int game_getCurNumPlayers(Game* g){
	return g->curNumPlayers;
}

int game_getMaxPlayers(Game* g){
	return g->maxPlayers;
}

Player* game_getPlayer(Game* g, int index){
	return g->players[index];
}

char* game_getStateString(Game* g){
	 _stateToString(g);
	return g->stateString;
}

void game_addPlayer(Game* g, Player* p){
	if(g->curNumPlayers < g->maxPlayers)
	g->players[g->curNumPlayers] = p;
	g->curNumPlayers++;
}

void game_removePlayer(Game* g, Player* p){
	bool found = false;
	int playerIndex = -1;
	for(int i=0;i<g->curNumPlayers && !found ;i++){
		if(g->players[i] == p){
			found = true;
			playerIndex = i;
		}
	}
	if(found){
		//slide everyone down
		for(int i=playerIndex+1;i<g->curNumPlayers;i++){
			g->players[i-1] = g->players[i];
		}
		g->curNumPlayers--;
	}
}

/*
 * Changes the state of the game
 * returns character string represting new game state OR
 * NULL, if game change remains unchanged;
 */
char* game_changeState(Game* g, const char* msg, int msgLen, int playerId){
//state can only be changed if there is quorum of players (all players present)
	if(g->curNumPlayers == g->maxPlayers){
		//check to see if msg is of valid format
		if(msgLen == 3 && //two for the message and 1 null byte (\0)
				msg[0] >= 48 && msg[0] <=57 &&
				msg[1] >= 48 && msg[1] <=57 &&
				msg[2] == 0 &&
					//check to see if it was my turn
				player_getId(g->players[g->turnNumber % g->curNumPlayers]) == playerId)	{
			//hooray, its my turn! 
			//is it a valid move?
			int board = atoi(msg) / 10;
			int square = atoi(msg) % 10;
	//DEBUG
		printf("board: %d\tsquare:%d\tactive:%d\n",board,square,g->activeBoard);
	//END DEBUG
			if((board == g->activeBoard || g->activeBoard == -1) //active board
		&& g->state[board][square] == 0 //the square is empty
		&& g->winner[9] == 0 /*the game has no winner yet*/){
				//update state
				g->state[board][square] = (g->turnNumber % g->curNumPlayers) + 1;
				g->activeBoard = square;
				g->lastBoard = board;
				g->lastSquare = square;
				//slow but this will work, check to see if the board is full
				bool isFull = true;
				for(int i=0;isFull && i<9;i++){
					if(g->state[g->activeBoard][i] == 0){
						isFull = false;
					}
				}
				if(isFull){
					g->activeBoard = -1;
				}
				//check win condition on new board
				_updateWinners(g,board,square);
				g->turnNumber++;
				_stateToString(g);
				return g->stateString;
			}
		} 
	}
	return NULL;
}

void game_playerQuit(Game* g, int playerId, bool* endGame){
	//lets just remove them
	Player* toRemove = NULL;
	for(int i=0;i<g->curNumPlayers && toRemove == NULL;i++){
		if( player_getId(g->players[i]) == playerId){
			toRemove = g->players[i];
		}
	}
	game_removePlayer(g, toRemove);
	if(g->curNumPlayers < GAME_QUORUM){
		*endGame = true;
	}
}
