#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "handshake.h"
#include "common.h"

HandshakeHeader* init_handshake(){
	return malloc(sizeof(HandshakeHeader));
}

void clear_handshake(HandshakeHeader* h){
	free(h->srRequest);
	free(h->host);
	free(h->connection);
	free(h->upgrade);
	free(h->secWebsocketKey);
	free(h->origin);
	free(h->secWebsocketProtocol);
	free(h->secWebsocketVersion);
	free(h->secWebsocketExtensions);
	free(h);
}
//The buffer will be modified to allow for easier parsing
HandshakeHeader* handshake_parseMsg(char* buffer, int bufSize){

	//declare enum for ease of code readability
	typedef enum {NONE, HOST, CONNECTION, UPGRADE, SEC_WEB_KEY,
					ORIGIN, SEC_WEB_PROT, SEC_WEB_VERS, SEC_WEB_EXT, OTHER} headerField;

	HandshakeHeader* result = malloc(sizeof(HandshakeHeader));
	result->srRequest = NULL;
	result->host = NULL;
	result->connection = NULL;
	result->upgrade = NULL;
	result->secWebsocketKey = NULL;
	result->origin = NULL;
	result->secWebsocketProtocol = NULL;
	result->secWebsocketVersion = NULL;
	result->secWebsocketExtensions = NULL;
	
	char* headerName;
	char* headerData;
	
	bool is_srRequest = true;

printf("%s",buffer);

	//parse the buffer one character at a time
	int startingLocation = 0;
	headerField currentHeader = NONE;
	for(int i=0;i<bufSize;i++){
		switch(buffer[i]){
			case ':':
				//if we are looking for a header, it has been found
				if(currentHeader == NONE){
					int strSize = i-startingLocation+1;
					headerName = malloc(strSize*sizeof(char));
					strncpy(headerName,&buffer[startingLocation],strSize-1);
					headerName[strSize-1] = '\0';
					//set currentHeader
					if(strcmp(headerName,"host")==0){
						currentHeader = HOST;
					}else if(strcmp(headerName,"connection")==0){
						currentHeader = CONNECTION;
					}else if(strcmp(headerName,"upgrade")==0){
						currentHeader = UPGRADE;
					}else if(strcmp(headerName,"sec-websocket-key")==0){
						currentHeader = SEC_WEB_KEY;
					}else if(strcmp(headerName,"origin")==0){
						currentHeader = ORIGIN;
					}else if(strcmp(headerName,"sec-websocket-protocols")==0){
						currentHeader = SEC_WEB_PROT;
					}else if(strcmp(headerName,"sec-websocket-version")==0){
						currentHeader = SEC_WEB_VERS;
					}else if(strcmp(headerName,"sec-websocket-extensions")==0){
						currentHeader = SEC_WEB_EXT;
					} else {
						currentHeader = OTHER;
					}

					startingLocation = i+1;
				}
				break;
			case '\n':
				if(currentHeader  !=  NONE){
					int strSize = i-startingLocation+1;
					headerData = malloc(strSize*sizeof(char));	
					strncpy(headerData,&buffer[startingLocation],strSize-1);
					headerData[strSize-1] = '\0';
					char* trimmed = trimWhiteSpace(headerData); 
					int trimmedSize = strlen(trimmed);
					trimmedSize++;
					switch(currentHeader){
						case HOST:
							result->host = malloc(trimmedSize*sizeof(char));
							strncpy(result->host,trimmed,trimmedSize-1);
							result->host[trimmedSize-1] = '\0';	
							break;
						case CONNECTION:
							result->connection = malloc(trimmedSize*sizeof(char));
							strncpy(result->connection,trimmed,trimmedSize-1);
							result->connection[trimmedSize-1] = '\0';	
							break;
						case UPGRADE:
							result->upgrade = malloc(trimmedSize*sizeof(char));
							strncpy(result->upgrade,trimmed,trimmedSize-1);
							result->upgrade[trimmedSize-1] = '\0';	
							break;
						case SEC_WEB_KEY:
							result->secWebsocketKey = malloc(trimmedSize*sizeof(char));
							strncpy(result->secWebsocketKey,trimmed,trimmedSize-1);
							result->secWebsocketKey[trimmedSize-1] = '\0';	
							break;
						case ORIGIN:
							result->origin = malloc(trimmedSize*sizeof(char));
							strncpy(result->origin,trimmed,trimmedSize-1);
							result->origin[trimmedSize-1] = '\0';	
							break;
						case SEC_WEB_PROT:
							result->secWebsocketProtocol = malloc(trimmedSize*sizeof(char));
							strncpy(result->secWebsocketProtocol,trimmed,trimmedSize-1);
							result->secWebsocketProtocol[trimmedSize-1] = '\0';	
							break;
						case SEC_WEB_VERS:
							result->secWebsocketVersion = malloc(trimmedSize*sizeof(char));
							strncpy(result->secWebsocketVersion,trimmed,trimmedSize-1);
							result->secWebsocketVersion[trimmedSize-1] = '\0';	
							break;
						case SEC_WEB_EXT:
							result->secWebsocketExtensions = malloc(trimmedSize*sizeof(char));
							strncpy(result->secWebsocketExtensions,trimmed,trimmedSize-1);
							result->secWebsocketExtensions[trimmedSize-1] = '\0';	
							break;
						case NONE:
						case OTHER:
						default:
							//do nothing
							;
					}
					free(headerName);
					free(headerData);
				} else if(is_srRequest){
					int strSize = i-startingLocation+1;
					headerData = malloc(strSize*sizeof(char));	
					strncpy(headerData,&buffer[startingLocation],strSize-1);
					headerData[strSize-1] = '\0';
					char* trimmed = trimWhiteSpace(headerData); 
					int trimmedSize = strlen(trimmed);
					trimmedSize++;
					//store in result->srRequest
					result->srRequest = malloc(trimmedSize*sizeof(char));
					strncpy(result->srRequest,trimmed,trimmedSize-1);
					result->srRequest[trimmedSize-1] = '\0';	
					free(headerData);
					is_srRequest = false;
					
				}
				currentHeader = NONE;
				startingLocation = i+1;
				break;
			default:
				//lowercase header chars to make strcmp easier
				if(currentHeader == NONE){
					buffer[i] = tolower(buffer[i]);
				}
		}
	}

	return result;
} 


