#pragma once
typedef enum {WS_OP_CONT = 0, WS_OP_TEXT = 1, WS_OP_BIN = 2, WS_OP_CLOSE = 8, 
				WS_OP_PING = 9, WS_OP_PONG = 10, WS_OP_HEADER = 17, 
				WS_OP_UNKNOWN = 18} Opcode;
typedef struct {
	char* bytes;
	unsigned long numBytes;
	char* msg;
	unsigned long msgSize;
	Opcode opcode;
	char isFin;
	char isMasked;	
	char mask[4];
} WebSocketMsg;

WebSocketMsg* init_webSocketMsg();
void clear_webSocketMsg(WebSocketMsg*);
void websocket_setBytes(WebSocketMsg* w, const char* bytes, int numBytes);
void websocket_setMsg(WebSocketMsg* w, const char* msg, int msgSize);
WebSocketMsg* websocket_handshake(char* buffer, int buffSize);
void websocket_decode(WebSocketMsg* wsm);
void websocket_encode(WebSocketMsg* wsm);
WebSocketMsg* websocket_msg(const char* msg, unsigned long numBytes, Opcode op);
WebSocketMsg* websocket_ping(const char* msg);
WebSocketMsg* websocket_close(const char* msg);
