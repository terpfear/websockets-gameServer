# websockets-gameServer
This is a threaded TCP server written in C that implements RFC 6455 (The WebSocket Protocol)

#Quickstart
This project has been tested on ubuntu 14.04 and is not guarenteed to work with other any other OS. Based on experience, this project should work with any debian distribution and will probably work on most *nix based machines but has not been specifically tested.

1. Clone Repository
2. Run 'make'
3. Run './websocketServer [desired port number]' ie './websocketServer 8080'

#Overview
This project is has many parts and is really a combination of many distinct parts: TCP Server, Websocket API, an implemented Game (a more fun version of Tic Tac Toe), and a WebSocket-gameServer to bring all the parts together.

##TCP Server
[server.c, server.h] 
This is the server loop that continuosly listens for, and accepts, new connections. The actual nuts and bolts of what happens inside the server loop is expected to be implemented by another file. This allows this TCP server to be used in multiple projects with no project dependent implementations. 

Secondly, this file provides an API for all socket interactions (recv, read, write, close). By providing a wrapper to the C calls, debugging is made easier because all reads and writes to the socket are handled in the same place, ie if the programmer wanted to output all bytes written to any given socket to STDOUT, it could be handled in this file without effecting any other piece of code.

##WebSocket API
[websocket.c, websocket.h, handshake.c, handshake.h]
These files provide the implementation of RFC 6455. All of the important functions return a WebSocketMsg* (pointer to a struct that contains all the important elements of WebSocket Message). The functions that I recommend using are:

websocket_handshake - Create a response handshake to a clients WebSocket request
websocket_msg - Create a WebSocket message with the given char* and WebSocket Type
websocket_ping - Create a WebSocket message to ping a client
websocket_close - Create a WebSocket message to close a connection with a client

##Game
[lobby.c, lobby.h, game.c, game.h, player.c, player.h]

I decided that all games must have a game lobby, it can completely trivial or a full fledged game lobby, the one provided is a fairly trivial lobby that waits until there are at least two people in the lobby and then spawns a new game. The game and player must have a few functions implemented for a game to work and it is completely implementation dependent:
  Player:
    init_player();
    clear_player();
    player_setName
    player_getName
    player_getId
  Game:
    init_game();
    clear_game();
    game_getQuorum();
    game_getId
    game_getCurNumPlayers
    game_getMaxPlayers
    game_addPlayer
    game_getStateString
    game_getPlayer
    game_changeState 
    game_playerQuit
As long as player and game can respond to these simple requests, any game can be implemented. In general, the lobby will create players, add them to a game when appropriate and create game threads. Game threads will pass messages to game.c requesting to the change the state of the game and the game will respond with a message. All of these message are completely generic and are defined by the game implementor. More than likely, the message will only make sense to the functions in game.c and wherever the client code lives (in the case of websockets, this is the javascript client side).

##WebSocket-GameServer
[wsgs.c, wsgs.h]
This combines makes use of all the above files above and is where the crux of the WebSocket GameServer lives. It contains four very important functions

###-serverLoop
This loop contains all the code for accepting new clients, performing the handshake and adding them to the game lobby.

###-gameLoop
This loop is a pthread that lobby creates for every game. It runs the game by taking messages from the clients and passing them onto implemented game and vice versa. There is also a seperate type of message that starts with "chat:" that does not get passed to the game, instead it is sent to all of the particpants of the game.

###-wsgs_sendMessage
This functions passes a string message to all of the connected clients of the game

###-wsgs_sendSingleMessage
This function passes a string message to single participant in the game

#Run Time Structure

There are two main loops and a theoretically infinite number of game loops. The main loops are the TCP server and the Game Lobby. The game lobby will spawn a pthread for each game it creates.

Main.c
|-----------------------------------------|
Server.c                                  |
|                                         |
wsgs.c(ServerLoop)                        |
|---------------------[lobby queue]-----lobbyLoop
                                          |
                                          |---------------------wsgs.c(gameLoop)
                                          |---------------------wsgs.c(gameLoop)
                                          |---------------------wsgs.c(gameLoop)
