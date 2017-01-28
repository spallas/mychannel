/**
 * Created by spallas on 26/jan/2017
 */

#include "server.h"

#define SERVER_PORT 8000
#define MAX_PENDING_REQ 3


void manage_new_connection(int sockfd){
    // ask client whether he wants to join or create a channel
    // If the user is joining a channel redirect to the appropriate process
    // else this process will be dedicated to the new channel.

    char command[COMMAND_SIZE];

    int command_len;
    command_len = read(sockfd, command, COMMAND_SIZE);
    command[command_len-1] = '\0';
    if (strcmp(command, ":join") == 0) {
        char channel_name[CHNAME_SIZE];
        read(sockfd, channel_name, CHNAME_SIZE);
        ch_t channel = find_ch_byname(channel_name);
        // send channel socket descriptor
        // so that client communicates to the channel process
        // the steps to take are:
        // - open AF_UNIX socket: the path is equal to the name of the channel
        // note: the channel process main routine repeatedly listens to
        // the socket waiting for new users. On different threads it will
        // listen for users messages.
        // - send message over socket to process. Use fdshare utilities.

    } else if (strcmp(command, ":create") == 0) {
        channel_main(sockfd);
        // what to do in channel process?
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

        if(fork() == 0) {
            manage_new_connection(client_desc);
            break;
        }
        close(client_desc);
     }

     exit(EXIT_SUCCESS);
 }
