/**
 * Created by spallas on 26/jan/2017
 */

#include "server.h"

#define SERVER_PORT 8500
#define MAX_PENDING_REQ 3

ch_t* channels[MAX_CHANNELS];
int num_channels;

int add_user_mutex;    // correspomd to semaphore array
int init_channel_mutex;
int write_mutex;
int fill_sem;
int empty_sem;

sigset_t mask;


// TODO: manage users leaving the channel
/**
 * Puts the user in the channel number ch_indx.
 * The number of users in the channel grows.
 * @param user: the user to insert in the channel
 * @param ch_indx: the index of the chennel where to put the new user
 * @return 0 on success, 1 if max number of user for the channel reached
 */
int add_user(user_t user, int ch_indx) {
    if(ch_indx >= MAX_CHANNELS) return -1;
    if(channels[ch_indx]->num_users >= MAX_CH_USERS) return -1;

    mutex_lock(add_user_mutex, ch_indx);
    int usr_indx = channels[ch_indx]->num_users;  //CS
    channels[ch_indx]->num_users++;               //CS
    mutex_unlock(add_user_mutex, ch_indx);

    channels[ch_indx]->ch_users[usr_indx] = malloc(sizeof(user_t));
    sprintf(channels[ch_indx]->ch_users[usr_indx]->nickname,"%s",user.nickname);
    channels[ch_indx]->ch_users[usr_indx]->socket   = user.socket;
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
    msg_t* msg = NULL;
    sem_wait(fill_sem, ch_indx);

    int read_index =  (channels[ch_indx]->read_index);
    msg = (channels[ch_indx])->ch_queue[read_index];
    channels[ch_indx]->read_index = (read_index+1)%QUEUE_SIZE;

    sem_post(empty_sem, ch_indx);
    return msg;
}

/**
 * Get a message from the current queue and send it to all users;
 * Function executed by thread ....
 * @param args: pointer that casted to an int contains the channel index
 */
void* broadcast_routine(void* args) {
    int ch_indx= (int) args;
    LOGi("About to broadcast messages from broadcast_routine");
    while(1){
        msg_t* msg = dequeue(ch_indx);
        for(int i=0; i<MAX_CH_USERS; i++){

            if(channels[ch_indx]->ch_users[i] == NULL) continue;
            if(strcmp(channels[ch_indx]->ch_users[i]->nickname, msg->nickname)==0)
                continue;
            //printf("%d\n", channels[ch_indx]->ch_users[i]->socket);
            int total_size = NICKNAME_SIZE + MSG_SIZE + 4;
            char buff[NICKNAME_SIZE + MSG_SIZE + 4] = {0};
            sprintf(buff, "%s: %s", msg->nickname, msg->data);
            send_stream(channels[ch_indx]->ch_users[i]->socket, buff, total_size);
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
    int i = num_channels;
    num_channels++;
    mutex_unlock(init_channel_mutex, 0);

    channels[i] = malloc(sizeof(ch_t));
    sprintf(channels[i]->ch_name, "%s", channel_name);
    channels[i]->num_users   = 0;
    channels[i]->read_index  = 0;
    channels[i]->write_index = 0;
    pthread_t thread;
    pthread_create(&thread, NULL, broadcast_routine, (void*) i);
    pthread_detach(thread);
    return i;
}

/**
 * Get the channel index in the array given his name
 * @param name: the name of the channel to find
 * @return : the index of the channel, -1 if not found
 */
int find_ch_byname(char* name) {
    int i;
    for (i = 0; i < MAX_CHANNELS; i++) {
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
int dialogue(user_t* user, int ch_indx) {
    while(1) {
        msg_t* message = malloc(sizeof(msg_t));
        sprintf(message->nickname, "%s", user->nickname);
        recv_stream(user->socket, message->data, MSG_SIZE);
        // check if message contains commands like leave
        // temporary
        if(message->data == NULL) break;
        enqueue(message, ch_indx);

    }
    return 0;
}

/**
 * Thread function....
 */
void* user_main(void* args) {

    user_t user;
    user.socket = (int) args;

    recv_packet(user.socket, user.nickname, NICKNAME_SIZE);
    LOGi("Received nickname");

    char command[COMMAND_SIZE];
    recv_packet(user.socket, command, COMMAND_SIZE);
    LOGi("Received command");

    if (strcmp(command, ":join") == 0) {
        char channel_name[CHNAME_SIZE];
        recv_packet(user.socket, channel_name, CHNAME_SIZE);
        LOGi("Received channel name");
        LOGi(channel_name);
        int ch_indx = find_ch_byname(channel_name);
        add_user(user, ch_indx);
        LOGi("New user joined");
        dialogue(&user, ch_indx);
    } else if (strcmp(command, ":create") == 0) {
        char channel_name[CHNAME_SIZE];
        recv_packet(user.socket, channel_name, CHNAME_SIZE);
        LOGi("Received new channel name");
        LOGi(channel_name);
        int ch_indx = init_channel(channel_name);
        add_user(user, ch_indx);
        LOGi("Added new user");
        dialogue(&user, ch_indx);
    }
    pthread_exit(NULL);
}

/**
 * Handles terminating signals
 */
void smooth_exit(int unused1, siginfo_t *info, void *unused2) {
    char signal_caught_msg[32];
    sprintf(signal_caught_msg,
            "Caught signal: sig%s",
            sys_signame[info->si_signo]);
    printf("\n");
    LOGi(signal_caught_msg);
    // alert all connected clients
    for (int i=0; i<MAX_CHANNELS; i++) {
        if(channels[i] != NULL) {
            for (int j=0; j<MAX_CH_USERS; j++) {
                if(channels[i]->ch_users[j] != NULL){
                    int total_size = NICKNAME_SIZE + MSG_SIZE + 4;
                    char buff[NICKNAME_SIZE + MSG_SIZE + 4] = {0};
                    sprintf(buff, "%s: %s",
                            "server message",
                            "Sorry, Error occcurred in server");
                    send_stream(channels[i]->ch_users[j]->socket,
                                buff, total_size);
                }
            }
        }
    }
    // free all structures
    for (int i=0; i<MAX_CHANNELS; i++) {
        if(channels[i] != NULL) {
            for (int j=0; j<MAX_CH_USERS; j++) {
                if(channels[i]->ch_users[j] != NULL)
                    free(channels[i]->ch_users[j]);
            }
            for (int j=0; j<QUEUE_SIZE; j++) {
                if(channels[i]->ch_queue[j] != NULL)
                    free(channels[i]->ch_queue[j]);
            }
            free(channels[i]);
        }
    }
    // close all descriptors

    // close all semaphores
    sem_close(add_user_mutex);
    sem_close(write_mutex);
    sem_close(fill_sem);
    sem_close(empty_sem);
    sem_close(init_channel_mutex);
    // and?

    exit(EXIT_FAILURE);
}

/**
 * Handles SIGSEGV signal
 */
void sigsegv_exit(int unused1, siginfo_t *info, void *unused2) {
    // alert all connected clients
    // free all structures
    // close all descriptors
    // log info about memory
    // and?
}


void handle_signal(int signal, void (*handler)(int, siginfo_t *, void *)) {
    // use sigaction
    struct sigaction act;
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    int err = sigaction(signal, &act, NULL); // assign handler to signal
    ERROR_HELPER(err, "Error in handle_signal: sigaction()");
}


int main(int argc, char const *argv[]) {

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
    init_channel_mutex = mutex_init(1);
    write_mutex = mutex_init(MAX_CHANNELS);
    fill_sem    = sem_init(0, MAX_CHANNELS);
    empty_sem   = sem_init(QUEUE_SIZE, MAX_CHANNELS);

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
        LOGi("Accepting new connections...");
        client_desc = accept(server_desc,
                             (struct sockaddr*) &client_addr,
                             &client_addr_len);

        pthread_t user_thread;
        pthread_create(&user_thread, NULL, user_main, (void*) client_desc);
        pthread_detach(user_thread);
     }

     exit(EXIT_SUCCESS);
 }
