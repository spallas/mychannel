
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

int send_stream(int sockfd, char* buff, int max_msg_size) {
    int sent_bytes = 0;
    int bytes_left = max_msg_size;
    while(bytes_left) {
        int ret = send(sockfd, buff + sent_bytes, 1, 0);
        if(ret == -1 && errno == EINTR) continue;
        if(ret == -1 && errno == EPIPE) return EPIPE;
        ERROR_HELPER(ret, "Could not write to socket in send_stream");
        sent_bytes += ret;
        bytes_left -= ret;
        if(buff[sent_bytes-1] == MSG_DELIMITER_CHAR) break;
    }
    return 0;
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

int recv_stream(int sockfd, char* buff, int max_msg_size) {
    int recvd_bytes = 0;
    int bytes_left = max_msg_size;
    int ret = 0;
    while(bytes_left) {
        ret = recv(sockfd, buff + recvd_bytes, 1, 0);
        if(ret == -1 && errno == EINTR) continue;
        if(ret == 0) break;
        ERROR_HELPER(ret, "Could not read from socket in recv_stream");
        recvd_bytes += ret;
        bytes_left -= ret;
        if(buff[recvd_bytes-1] == MSG_DELIMITER_CHAR) break;
    }
    return ret;
}
