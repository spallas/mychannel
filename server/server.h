/**
 * Created by spallas on 16/jan/2017
 */

#ifndef _SERVER_H_ // avoid multiple file inclusion
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.hs>

#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {               \
        if (cond) {                                                 \
            fprintf(stderr, "%s: %s\n", msg, strerror(errCode));    \
            exit(EXIT_FAILURE);                                     \
        }                                                           \
    } while(0)

#define ERROR_HELPER(ret, msg)         GENERIC_ERROR_HELPER((ret<0), errno, msg)
#define PTHREAD_ERROR_HELPER(ret, msg) GENERIC_ERROR_HELPER((ret!=0), ret, msg)

// message config parameters
#define MSG_SIZE            1024
#define NICKNAME_SIZE       128
// server configuration parameters
#define SERVER_NICKNAME     "mychannel"
#define MAX_CH_USERS        128
#define MAX_CHANNELS        32
#define CHNAME_SIZE         32
// messages managment
#define MSG_DELIMITER_CHAR  '|'
#define COMMAND_CHAR        ':'
#define COMMAND_SIZE        16
// each command starts with :
#define JOIN_COMMAND        "join"   // join <?> channel
#define LEAVE_COMMAND       "leave"  // leave a channel but stay in the program
#define QUIT_COMMAND        "quit"   // performs leave & quit
#define LIST_COMMAND        "list"   // list channel users
#define NEW_CH_COMMAND      "create" // creaete new channel
#define STATS_COMMAND       "stats"
#define HELP_COMMAND        "help"

// messages data structure
typedef struct msg_s {
    char    nickname[NICKNAME_SIZE];
    char    msg[MSG_SIZE];
} msg_t;

// users data structure
typedef struct user_s {
    char    nickname[NICKNAME_SIZE];
    int     socket;
} user_t;

// channel data structure
typedef struct ch_s {
    char   ch_name[CHNAME_SIZE];
    user_t ch_users[MAX_CH_USERS];
    int    num_users;
} ch_t;

// functions prototypes:

/*******************************************************************************
 * This function is called when a user creates a channel. The socket used to
 * comunicate with the client identifies the owner of the channel, the only one
 * that can terminate it.
 */
int channel_main(int creator_sockfd);

 #endif
