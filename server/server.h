/**
 * Created by spallas on 16/jan/2017
 */

// avoid multiple file inclusion
#ifndef _SERVER_H_
#define _SERVER_H_


#include "../shared/common.h"

// server configuration parameters
#define SERVER_NICKNAME     "mychannel"
#define MAX_CH_USERS        128
#define MAX_CHANNELS        32
#define QUEUE_SIZE          256

// messages data structure
typedef struct msg_s {
    char    nickname[NICKNAME_SIZE];
    char    data[MSG_SIZE];
} msg_t;

// users data structure
typedef struct user_s {
    char    nickname[NICKNAME_SIZE];
    int     socket;
} user_t;

// channel data structure
typedef struct ch_s {
    char ch_owner[NICKNAME_SIZE];
    char   ch_name[CHNAME_SIZE];
    user_t* ch_users[MAX_CH_USERS];
    int    num_users;
    msg_t*   ch_queue[QUEUE_SIZE];
    int read_index;
    int write_index;
} ch_t;

// functions prototypes:


 #endif
