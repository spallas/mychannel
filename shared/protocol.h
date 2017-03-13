
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "common.h"

// functions prototypes:

void readln(char* str, int size);
void send_packet(int sockfd, char* buff, int packet_size);
void send_stream(int sockfd, char* buff, int max_msg_size);
void recv_packet(int sockfd, char* buff, int packet_size);
int recv_stream(int sockfd, char* buff, int max_msg_size);

#endif
