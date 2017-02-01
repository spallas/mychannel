
#include "client.h"

#define SERVER_PORT 8500
#define SERVER_IP "127.0.0.1"
// use "139.59.131.72" to try with a real server
#define DEBUG 1

void* send_msg(void*);
void* recv_msg(void*);

int main(int argc, char const *argv[]) {

    char nickname[NICKNAME_SIZE];
    printf("Insert your nickname: ");
    readln(nickname, NICKNAME_SIZE);
    nickname[strlen(nickname)-1] = '\0';
    char command[COMMAND_SIZE];
    printf("Do you want to join or create a channel?\n");
    printf("(use :join or :create)\n> ");
    int join = 0;
    int create = 0;
    while(1){
        readln(command, COMMAND_SIZE);
        command[strlen(command)-1] = '\0'; // readln adds '\'
        if(strcmp(command, JOIN_COMMAND) == 0) {
            join = 1;
            break;
        } else if(strcmp(command, CREATE_COMMAND) == 0){
            create = 1;
            break;
        } else {
            printf("Please enter a valid command (:join or :create)\n> ");
            continue;
        }
    }
    char channel[CHNAME_SIZE];
    if(join)
        printf("Which channel do you want to join?\n> ");
    else if(create)
        printf("What is the name of your new channel?\n> ");
    readln(channel, CHNAME_SIZE);

    unsigned short server_port = htons(SERVER_PORT);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sockfd, "socket()");

    struct sockaddr_in server_addr = {0};
    int addr_size = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = server_port;
    inet_aton(SERVER_IP, &server_addr.sin_addr);

    int err = connect(sockfd, (struct sockaddr*) &server_addr, addr_size);
    ERROR_HELPER(err, "connect()");

    send_packet(sockfd, nickname, NICKNAME_SIZE);
    if (join) {
        send_packet(sockfd, JOIN_COMMAND, COMMAND_SIZE);
    } else if (create) {
        send_packet(sockfd, CREATE_COMMAND, COMMAND_SIZE);
    }
    send_packet(sockfd, channel, CHNAME_SIZE);

    pthread_t threads[2];
    pthread_create(&threads[0], NULL, send_msg, (void*) sockfd);
    pthread_create(&threads[1], NULL, recv_msg, (void*) sockfd);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    exit(EXIT_SUCCESS);
}


void* send_msg(void* args) {
    int sockfd = (int) args;
    char message[MSG_SIZE];

    while(1) {
        LOGi("Sending message to server...");
        readln(message, MSG_SIZE);
        send_stream(sockfd, message, MSG_SIZE);
    }
    pthread_exit(NULL);
}


void* recv_msg(void* args) {
    int sockfd = (int) args;
    char message[MSG_SIZE];

    while(1) {
        recv_stream(sockfd, message, MSG_SIZE);
        LOGi("Received message from server: ");
        message[strlen(message)-1] = '\0';
        printf("%s\n", message);
    }

    pthread_exit(NULL);
}
