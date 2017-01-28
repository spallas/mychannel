/**
 * Created by spallas on 26/jan/2017
 */

#include "server.h"

#define SERVER_PORT 8000
#define MAX_PENDING_REQ 3

ch_t* channels[MAX_CHANNELS];
int num_channels;

int add_user(ch_t* channel, user_t user) {
    /**** CS ****/
    channel->ch_users[channel->num_users] = user;  //TODO: chek dimension
    channel->num_users++;
    /**** CS ****/
    return 0;
}


void* broadcast_routine(void* args) {
    ch_t* channel = (ch_t*) args;

    return NULL;
}


ch_t* init_channel(char* channel_name) {

    /**** CS ****/
    int i = num_channels;
    channels[i] = malloc(sizeof(ch_t));
    sprintf(channels[i]->ch_name, "%s", channel_name);
    channels[i]->num_users = 0;
    channels[i]->read_index = 0;
    channels[i]->write_index = 0;
    num_channels++;
    /**** CS ****/
    pthread_t thread;
    pthread_create(&thread, NULL, broadcast_routine, (void*) &channels[i]);
    return channels[i];
}


ch_t* find_ch_byname(char* name) {
    int i;
    for (i = 0; i < MAX_CHANNELS; i++) {
        if(channels[i] == NULL) return NULL;
        if(strcmp(channels[i]->ch_name, name) == 0) return channels[i];
    }
    return NULL;
}


void enqueue(ch_t* channel, msg_t* message) {


}


msg_t* dequeue(ch_t* channel) {

    return NULL;
}


void* dialogue(user_t* user, ch_t* channel) {
    while(1) {
        msg_t* message = malloc(sizeof(msg_t));
        sprintf(message->nickname, "%s", user->nickname);
        read(user->socket, message->data, MSG_SIZE);

        // check if message contains commands

        enqueue(channel, message);

    }
    return NULL;
}


void* user_main(void* args) {

    user_t user;
    user.socket = (int) args;

    read(user.socket, user.nickname, NICKNAME_SIZE);

    char command[COMMAND_SIZE];
    int command_len;

    command_len = read(user.socket, command, COMMAND_SIZE);
    command[command_len-1] = '\0';

    if (strcmp(command, ":join") == 0) {
        char channel_name[CHNAME_SIZE];
        read(user.socket, channel_name, CHNAME_SIZE);
        ch_t* channel = find_ch_byname(channel_name);

        add_user(channel, user);

        dialogue(&user, channel);


    } else if (strcmp(command, ":create") == 0) {
        char channel_name[CHNAME_SIZE];
        int len;
        len = read(user.socket, channel_name, CHNAME_SIZE-1);
        channel_name[len+1] = '\0';

        ch_t* channel = init_channel(channel_name);
        add_user(channel, user);

        dialogue(&user, channel);

    }
    return NULL;
}


int main(int argc, char const *argv[]) {

    // initialize semaphores

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
    unsigned int client_addr_len;

    // declare variable for descriptor returned by accept
    int client_desc;

    // in this infinite loop accept new connections
    // upon a connection launch a thread to interpret what the client wants to
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
