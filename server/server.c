/**
 * Created by spallas on 26/jan/2017
 */

#include "server.h"

#define SERVER_PORT 8500
#define MAX_PENDING_REQ 3

#define SET_BIT(mask, n)   do { mask |= 1 << n; } while(0)
#define CLEAR_BIT(mask, n) do { mask &= ~(1 << n); } while(0)

ch_t* channels[MAX_CHANNELS];
int num_channels;

int ch_bitmask;
int last_free_ch = 1;

// send to user when joining a new channel
char ch_names[MAX_CHANNELS][CHNAME_SIZE];

// users threads, limited by no. of channels and no. of users per channels
pthread_t user_threads[MAX_CHANNELS*MAX_CH_USERS];

int add_user_mutex;    // correspond to semaphore array
int add_owner_mutex;
int init_channel_mutex;
int write_mutex;
int fill_sem;
int empty_sem;

sigset_t mask;

/**
 * Check if the channel in posotion ch_indx is free.
 */
int isFree(int ch_indx) {
    if(ch_indx < 0 || ch_indx >= MAX_CHANNELS) return 0;
    return (ch_bitmask >> ch_indx) & 1;
}

/**
 * Puts the user in the channel number ch_indx.
 * The number of users in the channel grows.
 * @param user: the user to insert in the channel
 * @param ch_indx: the index of the chennel where to put the new user
 * @return 0 on success, 1 if max number of user for the channel reached
 */
int add_user(user_t* user, int ch_indx) {
    if(ch_indx >= MAX_CHANNELS) return -1;
    if(channels[ch_indx]->num_users >= MAX_CH_USERS) return -1;
    int i;
    mutex_lock(add_user_mutex, ch_indx);
    for (i=0; i<MAX_CH_USERS; i++) {
        if(channels[ch_indx]->ch_users[i] == NULL) {
            channels[ch_indx]->ch_users[i] = user;
            channels[ch_indx]->num_users++;
            mutex_unlock(add_user_mutex, ch_indx);
            return 0;
        }
    }
    ERROR_HELPER(-1, "Inconsistency in num_users");
    return -1;
}




int add_owner(user_t* owner, int ch_indx) {
    mutex_lock(add_owner_mutex, ch_indx);
    strncpy(channels[ch_indx]->ch_owner, owner->nickname, NICKNAME_SIZE);
    mutex_unlock(add_owner_mutex, ch_indx);
    return 0;
}


/**
 * Add a message to the queue of messages that were sent to the ch_indx
 * channel.
 * @param message: the message to be enqueued
 * @param ch_indx: the index in the global array of channels
 *                 where to add message.
 */
void enqueue(msg_t* message, int ch_indx) {
    sem_wait(empty_sem, ch_indx);
    mutex_lock(write_mutex, ch_indx);

    int write_index = channels[ch_indx]->write_index;
    channels[ch_indx]->ch_queue[write_index] = message;
    channels[ch_indx]->write_index = (write_index + 1) % QUEUE_SIZE;

    mutex_unlock(write_mutex, ch_indx);
    sem_post(fill_sem, ch_indx);
}

/**
 * Get a message from the queue of messages that were sent to the ch_indx
 * channel.
 * @param message: the message to be dequeued
 * @param ch_indx: the index in the global array of channels
 *                 where to get the last message.
 */
msg_t* dequeue(int ch_indx) {
    msg_t* msg;
    sem_wait(fill_sem, ch_indx);

    int read_index =  (channels[ch_indx]->read_index);
    msg = (channels[ch_indx])->ch_queue[read_index];
    channels[ch_indx]->read_index = (read_index+1)%QUEUE_SIZE;

    sem_post(empty_sem, ch_indx);
    return msg;
}

/**
 * Get a message from the current queue and send it to all users in a channel;
 * Function executed by thread ....
 * @param args: pointer that casted to an int contains the channel index
 */
void* broadcast_routine(void* args) {
    int ch_indx= (int) args;
    LOGd("About to broadcast messages from broadcast_routine");
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    // terminate thread as soon as requested
    while(1){
        msg_t* msg = dequeue(ch_indx);
        for(int i=0; i<MAX_CH_USERS; i++){
            if(channels[ch_indx]->ch_users[i] == NULL) continue;
            if(strcmp(channels[ch_indx]->ch_users[i]->nickname, msg->nickname)==0)
                continue;
            int total_size = NICKNAME_SIZE + MSG_SIZE + 4;
            char* buff = malloc(total_size);
            strncpy(buff, msg->nickname, NICKNAME_SIZE);
            strncat(buff, ": ", 2);
            strncat(buff, msg->data, MSG_SIZE);
            send_stream(channels[ch_indx]->ch_users[i]->socket, buff, total_size);
            free(buff);
        }
        free(msg);
    }
    pthread_exit(NULL);
}

/**
 * Initialize new channel, allocate memory and set initial indexes
 * @param channel_name: the name of the channel given by the creator
 */
int init_channel(char* channel_name) {
    if(num_channels == MAX_CHANNELS) return -1;

    mutex_lock(init_channel_mutex, 0);
    int i; // index of a free channel
    if(isFree(last_free_ch)){
        i = last_free_ch;
    } else {
        for (i = 0; i < MAX_CHANNELS; i++)
            if(channels[i] == NULL) break; // free channel slot found
    }
    if(i == MAX_CHANNELS)
        ERROR_HELPER(-1, "Incosistency in num_channels variable");
    // create the channel
    channels[i] = calloc(1, sizeof(ch_t));
    if(channels[i] == NULL)
        ERROR_HELPER(-1, "init_channel(): Error allocating memory");
    sprintf(channels[i]->ch_name, "%s", channel_name);
    channels[i]->num_users   = 0;
    channels[i]->read_index  = 0;
    channels[i]->write_index = 0;
    num_channels++;
    SET_BIT(ch_bitmask, i);
    // unlock semaphore before creating thread and exiting the function
    mutex_unlock(init_channel_mutex, 0);

    // insert the name in ch_names
    sprintf(ch_names[i], "%s", channel_name);
    LOGi("New channel created");
    int err = pthread_create(&(channels[i]->broadcast_thread),
                    NULL, broadcast_routine, (void*) i);
    err |= pthread_detach(channels[i]->broadcast_thread);
    PTHREAD_ERROR_HELPER(err,"Error creating thread in init_channel()");
    return i;
}


int remove_user(user_t user, int ch_indx) {
    int i;
    msg_t* alert_msg = malloc(sizeof(msg_t));
    sprintf(alert_msg->nickname, "%s", user.nickname);
    sprintf(alert_msg->data, "%s", "I'm leaving the channel. Bye folks!|");
    enqueue(alert_msg, ch_indx);
    for (i = 0; i<MAX_CH_USERS; i++) {
        if(channels[ch_indx]->ch_users[i] == NULL) continue;
        if(strcmp(user.nickname, channels[ch_indx]->ch_users[i]->nickname)==0) {
            channels[ch_indx]->ch_users[i] = NULL;
            LOGd("Removed user");
            LOGd(user.nickname);
            return 0;
        }
    }
    LOGe(user.nickname);
    LOGe("User not found");
    return -1;
}

int delete_channel(int ch_indx) {
    msg_t* alert_msg = malloc(sizeof(msg_t));
    sprintf(alert_msg->nickname, "%s", channels[ch_indx]->ch_owner);
    sprintf(alert_msg->data, "%s",
            "Sorry, I deleted this channel! Send :leave to leave the channel|");
    if(channels[ch_indx] != NULL) {
        enqueue(alert_msg, ch_indx);
        sleep(1);
        pthread_cancel(channels[ch_indx]->broadcast_thread);
        mutex_lock(init_channel_mutex, 0);
        free(channels[ch_indx]);
        channels[ch_indx] = NULL;
        CLEAR_BIT(ch_bitmask, ch_indx);
        last_free_ch = ch_indx;
        num_channels--;
        mutex_unlock(init_channel_mutex, 0);
        LOGd("Channel deleted!");
    }
    return 0;
}

/**
 * Get the channel index in the array given his name
 * @param name: the name of the channel to find
 * @return : the index of the channel, -1 if not found
 */
int find_ch_byname(char* name) {
    int i;
    for (i = 0; i < MAX_CHANNELS; i++) {
        if(channels[i] == NULL) continue;
        if(strcmp(channels[i]->ch_name, name) == 0) return i;
    }
    return -1;
}

/**
 * Receive a message from a user and enqueue it in the given channel message
 * queue.
 * @param user : sender
 * @param ch_indx : the channel the user is talking to
 */
int dialogue(user_t* user, int ch_indx, int is_owner) {
    char leave_msg[COMMAND_SIZE];
    sprintf(leave_msg, "%s%c", LEAVE_COMMAND, MSG_DELIMITER_CHAR);
    char delete_msg[COMMAND_SIZE];
    sprintf(delete_msg, "%s%c", DELETE_COMMAND, MSG_DELIMITER_CHAR);
    while(1) {
        msg_t* message = malloc(sizeof(msg_t));
        sprintf(message->nickname, "%s", user->nickname);
        int ret = recv_stream(user->socket, message->data, MSG_SIZE);
        if(strcmp(message->data, leave_msg) == 0  || ret == 0) {
            remove_user(*user, ch_indx);
            break;
        } else if (strcmp(message->data, delete_msg) == 0 || ret == 0) {
            // delete ch_indx channel only if this is the creator
            if(is_owner){
                LOGd("About to delete a channel...");
                delete_channel(ch_indx);
                break;
            }
        }
        if(strcmp(message->data, "") == 0) break;
        enqueue(message, ch_indx);

    }
    return 0;
}

/**
 * Send a list of currently active channel names retrieving it from the ch_names
 * matrix.
 */
void send_ch_names(int sockfd) {
    int i;
    for (i=0; i<MAX_CHANNELS; i++) {
        if (strcmp(ch_names[i], "") == 0)
            continue;
        send_packet(sockfd, ch_names[i], CHNAME_SIZE);
    }
    char *thats_all = "END";
    send_packet(sockfd, thats_all, 32);
}
/**
 * Thread function....
 */
void* user_main(void* args) {

    user_t user_obj;
    user_t* user = &user_obj;
    user->socket = (int) args;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    int err;
    recv_packet(user->socket, user->nickname, NICKNAME_SIZE);
    LOGd("Received nickname");

    char command[COMMAND_SIZE];
    recv_packet(user->socket, command, COMMAND_SIZE);
    LOGd("Received command");

    if (strcmp(command, ":join") == 0) {
        // send list of active channels
        send_ch_names(user->socket);
        char channel_name[CHNAME_SIZE];
        recv_packet(user->socket, channel_name, CHNAME_SIZE);
        LOGd("Received channel name");
        LOGd(channel_name);
        int ch_indx = find_ch_byname(channel_name);
        if(ch_indx < 0) {
            // send channel not existent message and listen to new command
            char *error_message = "MyChannel: sorry your channel could not be found.|";
            send_stream(user->socket, error_message, strlen(error_message));
        } else {
            add_user(user, ch_indx);
            LOGd("New user joined");
            dialogue(user, ch_indx, 0);
        }

    } else if (strcmp(command, ":create") == 0) {
        char channel_name[CHNAME_SIZE];
        recv_packet(user->socket, channel_name, CHNAME_SIZE);
        LOGd("Received new channel name");
        LOGd(channel_name);
        int ch_indx = init_channel(channel_name);
        if(ch_indx < 0) {
            // send channel not existent message and listen to new command
            char *error_message = ERR_TOO_MANY_CH;
            send_stream(user->socket, error_message, strlen(error_message));
        } else {
            add_owner(user, ch_indx);
            add_user(user, ch_indx);
            LOGd("Added new user");
            dialogue(user, ch_indx, 1);
        }
    }

    err = close(user->socket);
    if(err<0) LOGe("Error closing socket in user_main");
    pthread_exit(NULL);
}

/**
 * Handles termination signals
 */
void smooth_exit(int unused1, siginfo_t *info, void *unused2) {
    char signal_caught_msg[32] = {0};
    sprintf(signal_caught_msg,
            "\nCaught signal: %s\n",
            strsignal(info->si_signo));
    // don't care about errors, info is not vital
    // Note that write() has been implemented as thread-safe and reentrant in
    // recent linux kernels.
    write(2, signal_caught_msg, strlen(signal_caught_msg));
    write(1, "\n", 1);
    // alert all connected clients
    msg_t* alert_msg = malloc(sizeof(msg_t));
    sprintf(alert_msg->nickname, "%s", "MyChannel");
    sprintf(alert_msg->data, "%s", "Sorry, Error occcurred in server|");
    for (int i=0; i<MAX_CHANNELS; i++) {
        if(channels[i] != NULL) {
            enqueue(alert_msg, i);
        }
    }
    sleep(1); // wait broadcast_threads to send the error message
    // Note: sleep affects only the calling thread
    // terminate all running threads
    for (int i=0; i<MAX_CHANNELS; i++) {
        if(channels[i] != NULL) {
            pthread_cancel(channels[i]->broadcast_thread);
        }
    }
    for (int i = 0; i < MAX_CHANNELS*MAX_CH_USERS; i++) {
        pthread_cancel(user_threads[i]);
    }

    // close all semaphores
    sem_close(add_user_mutex);
    sem_close(add_owner_mutex);
    sem_close(write_mutex);
    sem_close(fill_sem);
    sem_close(empty_sem);
    sem_close(init_channel_mutex);

    log_save();

    exit(EXIT_FAILURE);
}

/**
 * Handles SIGSEGV signal
 */
void sigsegv_exit(int unused1, siginfo_t *info, void *unused2) {
    unsigned int address;
    address = *(unsigned int*) &(info->si_addr);
    char msg[64];
    sprintf(msg, "segfault occurred (address is %x)\n", address);
    LOGe(msg);
    smooth_exit(unused1, info, unused2);
}


void handle_signal(int signal, void (*handler)(int, siginfo_t *, void *)) {
    // use sigaction
    struct sigaction act;
    sigset_t block_mask;

    sigemptyset (&block_mask);
    /* Block other terminal-generated signals while handler runs. */
    sigaddset (&block_mask, SIGINT);
    sigaddset (&block_mask, SIGQUIT);
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    act.sa_mask  = block_mask;
    int err = sigaction(signal, &act, NULL); // assign handler to signal
    ERROR_HELPER(err, "Error in handle_signal: sigaction()");
}


int main(int argc, char const *argv[]) {

    log_init();
    LOGi("Server started");
    printf("process: %d\n", getpid());

    int err = 0;
    // mask is a global variable
    err |= sigemptyset(&mask);
    err |= sigfillset(&mask);
    err |= sigdelset(&mask, SIGTERM);
    err |= sigdelset(&mask, SIGINT);
    err |= sigdelset(&mask, SIGQUIT);
    err |= sigdelset(&mask, SIGHUP);
    err |= sigdelset(&mask, SIGPIPE);
    err |= sigdelset(&mask, SIGSEGV);
    err |= pthread_sigmask(SIG_BLOCK, &mask, NULL);

    if(err != 0) {
        fprintf(stderr, "%s: %s\n",
                "Error in signal set initialization",  strerror(errno));
        exit(EXIT_FAILURE);
    }

    //  ignoring SIGPIPE signal
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        ERROR_HELPER(-1, "Error ignoring SIGPIPE");

    handle_signal(SIGTERM,smooth_exit);
    handle_signal(SIGINT, smooth_exit);
    handle_signal(SIGQUIT,smooth_exit);
    handle_signal(SIGHUP, smooth_exit);
    handle_signal(SIGILL, smooth_exit);
    handle_signal(SIGSEGV,sigsegv_exit);

    // initialize semaphores
    add_user_mutex     = mutex_init(MAX_CHANNELS);
    add_owner_mutex    = mutex_init(MAX_CHANNELS);
    init_channel_mutex = mutex_init(1);

    write_mutex = mutex_init(MAX_CHANNELS);
    fill_sem    = sem_init(0, MAX_CHANNELS);
    empty_sem   = sem_init(QUEUE_SIZE, MAX_CHANNELS);

    // the server listens on port 8000
    unsigned short server_port = htons(SERVER_PORT);

    // initialize the listening socket, use default protocol
    int server_desc = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    err = setsockopt(server_desc, SOL_SOCKET,
                     SO_REUSEADDR, &enable, sizeof(int));
    ERROR_HELPER(err, "setsockopt(SO_REUSEADDR) failed");

    // If the
    struct timeval tv;
    tv.tv_sec = 30;  /* 30 Secs Timeout */
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors
    err = setsockopt(server_desc, SOL_SOCKET, SO_RCVTIMEO,
                     (const char*)&tv, sizeof(struct timeval));
    ERROR_HELPER(err, "server.c -> setsockopt()");

    // initialize the server address with port defined above
    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = server_port;

    // relate the address just created to the server listening socket
    err = bind(server_desc,(struct sockaddr*) &server_addr,sizeof(server_addr));
    ERROR_HELPER(err, "bind()");
    // mark the server socket as listening: it will only issue accept/close
    err = listen(server_desc, MAX_PENDING_REQ);
    ERROR_HELPER(err, "listen()");
    // declare a variable to hold client address and its length
    // these will be filled by accept
    struct sockaddr_in* client_addr = malloc(sizeof(struct sockaddr_in));
    unsigned int client_addr_len = 0;

    // declare variable for descriptor returned by accept
    int client_desc;
    int ti = 0;
    // in this infinite loop accept new connections
    // upon a connection launch a thread to interpret what the client wants to
    // do between creating a channel and joining a channel
    while(1) {

        LOGi("Accepting new connections...");
        client_desc = accept(server_desc,
                             (struct sockaddr*) client_addr,
                             &client_addr_len);
        if(client_desc < 0) continue;
        err = pthread_create(&user_threads[ti], NULL, user_main,
                             (void*) client_desc);
        err|= pthread_detach(user_threads[ti]);
        PTHREAD_ERROR_HELPER(err, "Error creating threads in main loop");
        client_addr = calloc(1,sizeof(struct sockaddr_in));
        ti++;
     }

     exit(EXIT_SUCCESS);
 }
