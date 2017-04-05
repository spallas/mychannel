
#ifndef _COMMON_H_
#define _COMMON_H_

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "protocol.h"
#include "semaphore.h"
#include "log.h"

#ifdef __APPLE__
#else
  #define _GNU_SOURCE
  #define _POSIX_C_SOURCE 199309L
#endif

#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {  \
        if (cond) {                                    \
            LOGe(msg);                                 \
            /* do not kill the entire process
               but only the involved thread */         \
            pthread_exit(NULL);                        \
        }                                              \
    } while(0)

#define ERROR_HELPER(ret, msg)         GENERIC_ERROR_HELPER((ret<0), errno, msg)
#define PTHREAD_ERROR_HELPER(ret, msg) GENERIC_ERROR_HELPER((ret!=0), ret, msg)

#define LOGi(info) log_info(info);
#define LOGe(info) log_error(info);
#define LOGd(info) log_debug(info);


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
#define DELETE_COMMAND      ":delete" // deletes a channel
#define STATS_COMMAND       ":stats"
#define HELP_COMMAND        ":help"


#endif
