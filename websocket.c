/*****************
 * 
 * Websockets.c 
 * Author: S. Feingold
 * Based on: http://www.linuxhowtos.org/data/6/server.c
 * Date: March 3, 2015
 * Version .01
 *
 * This program will act a server listening and responding to websocket requests
 */


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <openssl/sha.h>

#include "common.h"
#include "websocket.h"
#include "handshake.h"
#include "base64.h"

WebSocketMsg* init_webSocketMsg(){
	WebSocketMsg* result = malloc(sizeof(WebSocketMsg));
	result->bytes = NULL;
	result->numBytes = 0L;
	result->msg = NULL;
	result->msgSize = 0L;
	result->opcode = WS_OP_UNKNOWN;	
	result->isFin = 0;
	result->isMasked = 0;
	result->mask[0] = 0;
	result->mask[1] = 0;
	result->mask[2] = 0;
	result->mask[3] = 0;

	return result;
}

void clear_webSocketMsg(WebSocketMsg* w){
	free(w->msg);
	free(w->bytes);
	free(w);
}

void websocket_setBytes(WebSocketMsg* w,const char* bytes, int numBytes){
	free(w->bytes);
	w->bytes = malloc(numBytes * sizeof(char));
	w->numBytes = numBytes;
	for(int i=0;i<numBytes;i++){
		w->bytes[i] = bytes[i];
	}
}

void websocket_setMsg(WebSocketMsg* w,const char* msg, int msgSize){
	if(msgSize > 0){
		w->msg = malloc(msgSize * sizeof(char));
		for(int i=0;i<msgSize;i++){
			w->msg[i] = msg[i];
		}
	}
	w->msgSize = msgSize;
}

//msg is NOT guarenteed to be unchanged by this function
WebSocketMsg* websocket_handshake(char* msg, int msgSize){
	HandshakeHeader* clientHandshake;
	WebSocketMsg* result = init_webSocketMsg();

	//perform handshake
	clientHandshake = handshake_parseMsg(msg,msgSize);
	//check each part of the handshake
	int len = strlen(clientHandshake->srRequest);
	if(clientHandshake->srRequest[0] != 'g' ||
			clientHandshake->srRequest[1] != 'e'	||
			clientHandshake->srRequest[2] != 't' ||
			clientHandshake->srRequest[len-3] != '1' ||
			clientHandshake->srRequest[len-2] != '.' ||
			clientHandshake->srRequest[len-1] != '1'){
		nf_error("ERROR: not a proper request - 01\n", false);
	}

	if(clientHandshake->host == NULL){
		nf_error("ERROR: not a proper request - 02\n", false);
		return NULL;
	}

	if(clientHandshake->upgrade == NULL || strcasecmp(clientHandshake->upgrade,"websocket") != 0){
		nf_error("ERROR: not a proper request -03\n", false);
		return NULL;
	}

	//its possible for connection to be a comma separated list
	bool hasUpgrade = false;
	if(clientHandshake->connection != NULL){
		int stringSize = strlen(clientHandshake->connection);
		int csvIndexes[stringSize];
		int csvSize = 1;
		int searching = false;
		for(int i=0;i<stringSize;i++){
			csvIndexes[i] = 0;
			if(clientHandshake->connection[i] == ','){
				clientHandshake->connection[i] = '\0';
				searching = true;
			} else if(searching && clientHandshake->connection[i] != ' '){
				searching = false;
				csvIndexes[csvSize] = i;
				csvSize++;
			}
		}
		//csv now contains all the strings
		for(int i=0;i<csvSize;i++){
			if(strcmp(&clientHandshake->connection[csvIndexes[i]],"Upgrade") == 0){
				hasUpgrade = true;
				break;
			}
		}
	}
	if(clientHandshake->connection == NULL || !hasUpgrade){
		nf_error("ERROR: not a proper request -04\n", false);
		return NULL;
	}

	if(clientHandshake->secWebsocketKey == NULL){
		nf_error("ERROR: not a proper request -05\n", false);
		return NULL;
	}

	if(clientHandshake->secWebsocketVersion == NULL || 
			strcmp(clientHandshake->secWebsocketVersion,"13")){
		nf_error("ERROR: not a proper request 06\n", false);
		return NULL;
	}

	//valid client handshake received!
	HandshakeHeader responseHandshake;
	responseHandshake.srRequest = "HTTP/1.1 101 Switching Protocol";
	responseHandshake.upgrade = "websocket";
	responseHandshake.connection = "Upgrade";

	char* magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	int magicStringLen = strlen(magicString);
	int keyLength = strlen(clientHandshake->secWebsocketKey);

	char acceptRaw[keyLength+magicStringLen+1];
	char acceptSha[SHA_DIGEST_LENGTH];
	acceptRaw[0] = '\0';
	strncat(acceptRaw,clientHandshake->secWebsocketKey,keyLength);
	strncat(acceptRaw,magicString,magicStringLen);
	//hash the Raw
	SHA1((unsigned char*) acceptRaw,keyLength+magicStringLen,(unsigned char*)acceptSha);
	//base64 encode the sha
	base64_encode(acceptSha,SHA_DIGEST_LENGTH,&responseHandshake.secWebsocketKey);

	//craft the response
	int size = 1024;
	result->msg = malloc (size* sizeof(char));
	memset(result->msg,0,size);
	snprintf(result->msg, size, "%s\r\nUpgrade: %s\r\nConnection: %s\r\nSec-WebSocket-Accept: %s\r\n\r\n",responseHandshake.srRequest, 
				responseHandshake.upgrade, responseHandshake.connection, responseHandshake.secWebsocketKey);
	free(responseHandshake.secWebsocketKey);
	clear_handshake(clientHandshake);
	result->msgSize = strlen(result->msg);
	result->opcode = WS_OP_HEADER;
	return result;
}

void websocket_decode(WebSocketMsg* wsm){
	unsigned char tempSize = 0;

	unsigned long byteIterator = 0;
	char* buffer = wsm->bytes;

	unsigned long byte = buffer[byteIterator++];


	//TODO: 
	//Check size of buffer to prevent underflow

	wsm->isFin = byte >> 63;
	wsm->opcode = byte & 15;

	byte = buffer[byteIterator++];

	wsm->isMasked = byte>> 63;
	tempSize = byte & 127;

	if(tempSize < 126){
		wsm->msgSize = tempSize;
	} else if(tempSize == 126){
		//message size is the next two bytes (2^16)
		byte = buffer[byteIterator++];
		wsm->msgSize = byte << 8;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte;	
	} else {
		//tempSize == 127
		byte = buffer[byteIterator++];
		wsm->msgSize = byte << 56;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte << 48;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte << 40;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte << 32;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte << 24;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte << 16;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte << 8;
		byte = buffer[byteIterator++];
		wsm->msgSize += byte;
	}
	//msgSize is now set
	if(!wsm->isMasked){
		//error
		//per rfc "The server MUST close the connection upon receiving a
		//frame that is not masked"
		clear_webSocketMsg(wsm);
		wsm = websocket_close("");
	}

	if(wsm->msgSize > 0){
		wsm->msg = malloc(wsm->msgSize * sizeof(char));
		wsm->mask[0] = buffer[byteIterator++];
		wsm->mask[1] = buffer[byteIterator++];
		wsm->mask[2] = buffer[byteIterator++];
		wsm->mask[3] = buffer[byteIterator++];

		for(int i=0;i<wsm->msgSize;i++){
			wsm->msg[i] = buffer[byteIterator++] ^ wsm->mask[i % 4];
		}
	}

	//null terminate the string
	if(wsm->msgSize > 0 && wsm->opcode != WS_OP_BIN) { //treat msg as text
		wsm->msg[wsm->msgSize-1] = '\0';
	}
//DEBUG
	printf("%d: %d\n",wsm->isFin,wsm->opcode);
	printf("%d: %d\n",wsm->isMasked,tempSize);
	printf("%lu\n",wsm->msgSize);
	if(wsm->msgSize > 0){
		printf("%s\n",wsm->msg);
	}
//END DEBUG
}

//Set the wsm->bytes and wsm->numBytes fields
// according to RFC 
void websocket_encode(WebSocketMsg* wsm){
	char payloadLen;
	unsigned long byteIterator = 0L;
	wsm->numBytes = 2 + 4 * wsm->isMasked;
	if(wsm->msgSize < 126){
		payloadLen = wsm->msgSize;
		wsm->numBytes += wsm->msgSize;
	} else if(wsm->msgSize < 65535){
		payloadLen = 126;
		wsm->numBytes += wsm->msgSize + 2; //add 16 bits to encode payload length
	} else {
		payloadLen = 127;
		wsm->numBytes += wsm->msgSize + 8; //add 64 bits to encode payload length
	}
	//clear anything that was in bytes already
	free(wsm->bytes);
	
	wsm->bytes = malloc(wsm->numBytes * sizeof(char));
	wsm->bytes[byteIterator++] = (wsm->isFin << 7) + wsm->opcode;
	wsm->bytes[byteIterator++] = (wsm->isMasked << 7) + payloadLen;
	//variable length starts here

	//encode 0,2 or 8 bytes of size data
	if(payloadLen == 126){
		//encode the next 2 bytes
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 8) & 255;  //first 8 bits
		wsm->bytes[byteIterator++] = wsm->msgSize & 255; //last 8 bits	
	} else if(payloadLen == 127){
		//encode the next 8 bytes
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 56) & 255;
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 48) & 255;
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 40) & 255;
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 32) & 255;
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 24) & 255;
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 18) & 255;
		wsm->bytes[byteIterator++] = (wsm->msgSize >> 8) & 255;
		wsm->bytes[byteIterator++] = wsm->msgSize & 255;
	}

	//set the mask if there is one
	if(wsm->isMasked){
		wsm->bytes[byteIterator++] = wsm->mask[0];
		wsm->bytes[byteIterator++] = wsm->mask[1];
		wsm->bytes[byteIterator++] = wsm->mask[2];
		wsm->bytes[byteIterator++] = wsm->mask[3];
	} else {
		//make sure the mask is clear
		wsm->mask[0] = 0;
		wsm->mask[1] = 0;
		wsm->mask[2] = 0;
		wsm->mask[3] = 0;
	}
	
	//set the message
	for(int i = 0;i < wsm->msgSize;i++){
		wsm->bytes[byteIterator++] = wsm->msg[i] ^ wsm->mask[i % 4];
	} 
}

WebSocketMsg* websocket_msg(const char* msg, unsigned long numBytes, Opcode op){
	WebSocketMsg* result= init_webSocketMsg();
	result->opcode = op;
	result->isMasked = 0;
	result->isFin = 1;
	websocket_setMsg(result, msg,numBytes);
	websocket_encode(result);
	return result;
}

WebSocketMsg* websocket_ping(const char* msg){
	WebSocketMsg* result = init_webSocketMsg();
	result->opcode = WS_OP_PING;
	result->isMasked = 0;
	result->isFin = 1;
	websocket_setMsg(result, msg,strlen(msg));
	websocket_encode(result);
	return result;
}

WebSocketMsg* websocket_close(const char* msg){
	WebSocketMsg* result = init_webSocketMsg();
	result->opcode = WS_OP_CLOSE;
	result->isMasked = 0;
	result->isFin = 1;
	websocket_setMsg(result, msg,strlen(msg));
	websocket_encode(result);
	return result;
}
