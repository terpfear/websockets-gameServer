CC = gcc
CFLAGS = -std=c99 -g -Wall

#all: handshake.o base64.o player.o websocket.o server

websocketServer: main.c server.o server.h websocket.o player.o game.o common.o handshake.o wsgs.o lobby.o base64.o
	$(CC) $(CFLAGS) main.c server.o websocket.o player.o game.o common.o handshake.o base64.o wsgs.o lobby.o -o websocketServer -lcrypto -lm -lpthread

server.o: server.c server.h
	$(CC) $(CFLAGS) -c server.c

wsgs.o: wsgs.c wsgs.h 
	$(CC) $(CFLAGS) -c wsgs.c

lobby.o: lobby.c lobby.h
	$(CC) $(CFLAGS) -c lobby.c

websocket.o: websocket.c websocket.h
	$(CC) $(CFLAGS) -c websocket.c

handshake.o: handshake.c handshake.h
	$(CC) $(CFLAGS) -c handshake.c

player.o: player.c player.h
	$(CC) $(CFLAGS) -c player.c

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c

base64.o: base64.c base64.h
	$(CC) $(CFLAGS) -c base64.c -D_GNU_SOURCE -lm

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

clean:
	rm *.o
