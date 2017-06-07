
#include "client.h"

#define SERVER_PORT 8500
#define SERVER_IP "127.0.0.1"
// use "139.59.131.72" to try with a real server
#define DEBUG 1

pthread_t threads[2];

void* send_msg(void*);
void* recv_msg(void*);
void smooth_exit(int unused1, siginfo_t *info, void *unused2);
void sigsegv_exit(int unused1, siginfo_t *info, void *unused2);
void handle_signal(int signal, void (*handler)(int, siginfo_t *, void *));

int sockfd;
sigset_t mask;
int sem;

int joining = 0;
int creating = 0;

int main(int argc, char const *argv[]) {
    log_init_name("./client/.client_log.txt");
    int err = 0;

    //  ignoring SIGPIPE signal
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        ERROR_HELPER(-1, "Error ignoring SIGPIPE");

    handle_signal(SIGTERM,smooth_exit);
    handle_signal(SIGINT, smooth_exit);
    handle_signal(SIGQUIT,smooth_exit);
    handle_signal(SIGSEGV,sigsegv_exit);

    unsigned short server_port = htons(SERVER_PORT);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sockfd, "socket()");

    struct sockaddr_in server_addr = {0};
    int addr_size = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = server_port;
    inet_aton(SERVER_IP, &server_addr.sin_addr);

    err = connect(sockfd, (struct sockaddr*) &server_addr, addr_size);
    ERROR_HELPER(err, "Error in connect()");

    char nickname[NICKNAME_SIZE];
    printf("Insert your nickname: ");
    readln(nickname, NICKNAME_SIZE);
    nickname[strlen(nickname)-1] = '\0';
    char command[COMMAND_SIZE];
    printf("Do you want to join or create a channel?\n");
    printf("(use :join or :create)\n> ");
    while(1){
        readln(command, COMMAND_SIZE);
        command[strlen(command)-1] = '\0'; // readln adds '\'
        if(strcmp(command, JOIN_COMMAND) == 0) {
            joining = 1;
            break;
        } else if(strcmp(command, CREATE_COMMAND) == 0){
            creating = 1;
            break;
        } else {
            printf("Please enter a valid command (:join or :create)\n> ");
            continue;
        }
    }

    send_packet(sockfd, nickname, NICKNAME_SIZE);
    char channel[CHNAME_SIZE];
    if(joining) {
        send_packet(sockfd, JOIN_COMMAND, COMMAND_SIZE);
        printf("Which channel do you want to join?\n");
        char ch_name[CHNAME_SIZE];
        recv_packet(sockfd, ch_name, CHNAME_SIZE);
        while(strcmp(ch_name, "END") != 0) {
            printf("- %s\n", ch_name);
            memset(ch_name, 0, CHNAME_SIZE);
            recv_packet(sockfd, ch_name, CHNAME_SIZE);
        }
    } else if(creating) {
        send_packet(sockfd, CREATE_COMMAND, COMMAND_SIZE);
        printf("What is the name of your new channel?\n");
    }
    printf("> ");
    readln(channel, CHNAME_SIZE);
    channel[strlen(channel)-1] = '\0';
    (joining ? printf("Joined!!\n\n") : printf("Created!!\n\n"));
    send_packet(sockfd, channel, CHNAME_SIZE);


    err = pthread_create(&threads[1], NULL, recv_msg, (void*) sockfd);
    err |= pthread_create(&threads[0], NULL, send_msg, (void*) sockfd);
    PTHREAD_ERROR_HELPER(err, "Error creating communication threads");
    err = pthread_join(threads[0], NULL);
    err |= pthread_join(threads[1], NULL);
    PTHREAD_ERROR_HELPER(err, "Error joining communication threads");

    err = close(sockfd);
    ERROR_HELPER(err, "Could not close socket in client:main");
    printf("\nBye!\n");
    exit(EXIT_SUCCESS);
}


void* send_msg(void* args) {
    sleep(1);
    int sockfd = (int) args;
    char message[MSG_SIZE] = {0};
    int err;
    char leave_msg[COMMAND_SIZE];
    sprintf(leave_msg, "%s%c", LEAVE_COMMAND, MSG_DELIMITER_CHAR);
    char delete_msg[COMMAND_SIZE];
    sprintf(delete_msg, "%s%c", DELETE_COMMAND, MSG_DELIMITER_CHAR);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int delete = 0;
    while(1) {
        readln(message, MSG_SIZE);
        if(strcmp(message, "") == 0) continue;
        if((strcmp(message, delete_msg) == 0
            || strcmp(message, leave_msg) == 0) && creating)
        {
            printf("Leaving the channel...\n");
            printf("Telling MyChannel to delete the channel...\n");
            delete = 1;
            pthread_cancel(threads[1]);
            break;
        } else if(strcmp(message, leave_msg) == 0) {
            printf("Leaving the channel...\n");
            pthread_cancel(threads[1]);
            break;
        }
        err = send_stream(sockfd, message, MSG_SIZE);
        if(err == EPIPE) {
            printf("MyChannel servers are issuing problems...\n");
            fflush(stdout);
            pthread_exit(NULL);
        }
        LOGd("Sent message to server...");
        memset(message, 0, MSG_SIZE);
    }
    if(delete) send_stream(sockfd, delete_msg, COMMAND_SIZE);
    else send_stream(sockfd, leave_msg, COMMAND_SIZE);

    LOGd("Sent message to server...");
    pthread_exit(NULL);
}


void* recv_msg(void* args) {
    int sockfd = (int) args;
    char message[MSG_SIZE];

    while(1) {
        int ret = recv_stream(sockfd, message, MSG_SIZE);
        if(ret == 0) {
            printf("MyChannel servers are issuing problems...\n");
            printf("Press enter to quit > ");
            fflush(stdout);
            pthread_cancel(threads[0]);
            pthread_exit(NULL);
        }
        LOGd("Received message from server: ");
        message[strlen(message)-1] = '\0';
        printf("%s\n", message);
        memset(message, 0, MSG_SIZE);
    }

    pthread_exit(NULL);
}


void handle_signal(int signal, void (*handler)(int, siginfo_t *, void *)){
    // use sigaction
    struct sigaction act;
    sigset_t block_mask;

    sigemptyset (&block_mask);
    /* Block other terminal-generated signals while handler runs. */
    sigaddset (&block_mask, SIGINT);
    sigaddset (&block_mask, SIGQUIT);
    sigaddset (&block_mask, SIGTERM);
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    act.sa_mask  = block_mask;
    int err = sigaction(signal, &act, NULL); // assign handler to signal
    ERROR_HELPER(err, "Error in handle_signal: sigaction()");
}

/**
 * Executed when one of the following signals occur: SIGTERM, SIGINT, SIGQUIT
 * SIGHUP, SIGILL. It first stops the sending and listening threads using the
 * library function pthread_cancel() which sends a terminating message to the
 * threads; by default the message will be delivered in the cancellation point
 * of the thread, which in this case are the calls to send() and recv().
 * See pthreads(7) for info about cancellation functions.
 */
void smooth_exit(int unused1, siginfo_t *info, void *unused2) {
    pthread_cancel(threads[0]);
    pthread_cancel(threads[1]);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    char leave_msg[COMMAND_SIZE];
    sprintf(leave_msg, "%s%c", LEAVE_COMMAND, MSG_DELIMITER_CHAR);
    send_stream(sockfd, leave_msg, MSG_SIZE);
    int err = close(sockfd);
    ERROR_HELPER(err, "Could not close socket in client:smooth_exit");
    write(1, "\nBye! C you soon!\n", 18);
    log_save();
    exit(0);
}


void sigsegv_exit(int unused1, siginfo_t *info, void *unused2) {
    unsigned int address;
    address = *(unsigned int*) &(info->si_addr);
    char msg[64];
    sprintf(msg, "segfault occurred (address is %x)\n", address);
    LOGe(msg);
}
