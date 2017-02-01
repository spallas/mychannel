
#include "protocol.h"

void readln(char* str, int size) {
    fgets(str, size, stdin);
    int len = strlen(str);
    str[len-1] = MSG_DELIMITER_CHAR;
}

void send_packet(int sockfd, char* buff, int packet_size) {
    int sent_bytes = 0;
    int bytes_left = packet_size;
    while(bytes_left) {
        int ret = send(sockfd, buff + sent_bytes, packet_size, 0);
        if(ret == -1 && errno == EINTR) continue;
        if(ret == 0) break;
        ERROR_HELPER(ret, "Could not write to socket in send_packet");
        sent_bytes += ret;
        bytes_left -= ret;
    }
}

void send_stream(int sockfd, char* buff, int max_msg_size) {
    int sent_bytes = 0;
    int bytes_left = max_msg_size;
    while(bytes_left) {
        int ret = send(sockfd, buff + sent_bytes, 1, 0);
        if(ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Could not write to socket in send_stream");
        sent_bytes += ret;
        bytes_left -= ret;
        if(buff[sent_bytes-1] == MSG_DELIMITER_CHAR) break;
    }
}

void recv_packet(int sockfd, char* buff, int packet_size) {
    int sent_bytes = 0;
    int bytes_left = packet_size;
    while(bytes_left) {
        int ret = recv(sockfd, buff + sent_bytes, packet_size, 0);
        if(ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Could not read from socket in recv_packet");
        sent_bytes += ret;
        bytes_left -= ret;
    }
}

void recv_stream(int sockfd, char* buff, int max_msg_size) {
    int sent_bytes = 0;
    int bytes_left = max_msg_size;
    while(bytes_left) {
        int ret = recv(sockfd, buff + sent_bytes, 1, 0);
        if(ret == -1 && errno == EINTR) continue;
        if(ret == 0) break;
        ERROR_HELPER(ret, "Could not read from socket in recv_stream");
        sent_bytes += ret;
        bytes_left -= ret;
        if(buff[sent_bytes-1] == MSG_DELIMITER_CHAR) break;
    }
}
