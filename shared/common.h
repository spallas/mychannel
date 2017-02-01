
#ifndef _COMMON_H_
#define _COMMON_H_

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "protocol.h"
#include "semaphore.h"

#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {               \
        if (cond) {                                                 \
            fprintf(stderr, "%s: %s\n", msg, strerror(errCode));    \
            exit(EXIT_FAILURE);                                     \
        }                                                           \
    } while(0)

#define ERROR_HELPER(ret, msg)         GENERIC_ERROR_HELPER((ret<0), errno, msg)
#define PTHREAD_ERROR_HELPER(ret, msg) GENERIC_ERROR_HELPER((ret!=0), ret, msg)

#define LOGi(info) do { fprintf(stderr, "[INFO]: %s\n", info); } while(0)

// message config parameters
#define MSG_SIZE            1024
#define NICKNAME_SIZE       128
#define CHNAME_SIZE         32

// messages managment
#define MSG_DELIMITER_CHAR  '|'
//#define COMMAND_CHAR        ':'
#define COMMAND_SIZE        16

// each command starts with :
#define JOIN_COMMAND        ":join"   // join <?> channel
#define LEAVE_COMMAND       ":leave"  // leave a channel but stay in the program
#define QUIT_COMMAND        ":quit"   // performs leave & quit
#define LIST_COMMAND        ":list"   // list channel users
#define CREATE_COMMAND      ":create" // creaete new channel
#define STATS_COMMAND       ":stats"
#define HELP_COMMAND        ":help"


#endif
