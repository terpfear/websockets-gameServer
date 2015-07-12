#pragma once
typedef struct {
	char* srRequest;
	char* host;
	char* connection;
	char* upgrade;
	char* secWebsocketKey;
	char* origin;
	char* secWebsocketProtocol;
	char* secWebsocketVersion;
	char* secWebsocketExtensions;
} HandshakeHeader;

HandshakeHeader* handshake_parseMsg(char* buffer, int size);
HandshakeHeader* init_handshake();
void clear_handshake(HandshakeHeader* h);

