/**
 * Created by spallas on 26/jan/2017
 */

 #include "server.h"

#define SERVER_PORT 1995
#define MAX_PENDING_REQ 3

typedef struct conn_thread_args_s {
    int socket;
} conn_thread_args_t;

void* manage_new_connection(void* args){
    // ask client whether he wants to join or create a channel
    int sockfd = args->socket;

    char command[COMMAND_SIZE];

    int command_len;
    command_len = read(sockfd, command, COMMAND_SIZE);
    command[command_len-1] = '\0';
    if (strcmp(command, ":join") == 0) {
        char channel_name[CHNAME_SIZE];
        read(sockfd, channel_name, CHNAME_SIZE);
        ch_t channel = find_ch_byname(channel_name);
        // send channel socket so that client communicates in that channel?
    } else if (strcmp(command, ":create") == 0) {
        create_channel(sockfd);
        // what to do in create channel?
    }
}


int main(int argc, char const *argv[]) {

    // the server listens on port 1995
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
    // upon a connection launch a thread to interpret what the client wants to
    // do between creating a channel and joining a channel
    while(1) {

        client_desc = accept(server_desc,
                             (struct sockaddr*) &client_addr,
                             &client_addr_len);
        conn_thread_args_t* args = malloc(sizeof(conn_thread_args_t));
        args->socket  = client_desc;

        pthread_t conn_thread;
        pthread_create(&conn_thread, NULL, manage_new_connection, args);

        pthread_detach(conn_thread);

     }

     exit(EXIT_SUCCESS);
 }
