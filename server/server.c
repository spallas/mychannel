/**
 * Created by spallas on 26/jan/2017
 */

#include "server.h"

#define SERVER_PORT 8000
#define MAX_PENDING_REQ 3

ch_t channels[MAX_CHANNELS];
int num_channels;

void* dialogue(user_t* user, ch_t* channel) {

    while(1) {
        char msg[256] = {0};
        read(user.socket, msg, 256);

        // check if message contains commands
        // broadcast(channel, user, buff);
        enqueue(channel->queue, user->nickname, msg);

    }
}


void* user_main(void* args) {

    user_t user;
    user.socket = (int) args;

    read(user.socket, user.nickname, NICKNAME_SIZE);

    char command[COMMAND_SIZE];
    int command_len;

    command_len = read(sockfd, command, COMMAND_SIZE);
    command[command_len-1] = '\0';
    if (strcmp(command, ":join") == 0) {
        char channel_name[CHNAME_SIZE];
        read(sockfd, channel_name, CHNAME_SIZE);
        ch_t* channel = find_ch_byname(channel_name);
        channel->ch_users[channel->num_users] = user;  //TODO: check dimension
        channel->num_users++;

        dialogue(&user, &channel);


    } else if (strcmp(command, ":create") == 0) {
        ch_t channel = {0};
        read(user.socket, channel.ch_name, CHNAME_SIZE);
        channel->ch_users[channel->num_users] = user;  //TODO: chek dimension
        channel->num_users++;

        dialogue(&user, &channel);

    }

}


int main(int argc, char const *argv[]) {

    // the server listens on port 8000
    unsigned short server_port = htons(SERVER_PORT);

    // initialize the listening socket, use default protocol
    int server_desc = socket(AF_INET, SOCK_STREAM, 0);

    // initialize the server address with port defined above
    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = server_port;

    // relate the address just created to the server listening socket
    bind(server_desc, (struct sockaddr*) &server_addr, sizeof(server_addr));

    // mark the server socket as listening: it will only issue accept/close
    listen(server_desc, MAX_PENDING_REQ);

    // declare a variable to hold client address and its length
    // these will be filled by accept
    struct sockaddr_in client_addr;
    int client_addr_len;

    // declare variable for descriptor returned by accept
    int client_desc;

    // in this infinite loop accept new connections
    // upon a connection launch a process to interpret what the client wants to
    // do between creating a channel and joining a channel
    while(1) {

        client_desc = accept(server_desc,
                             (struct sockaddr*) &client_addr,
                             &client_addr_len);

        pthread_t user_thread;
        pthread_create(&user_thread, NULL, user_main, (void*) client_desc);
     }

     exit(EXIT_SUCCESS);
 }
