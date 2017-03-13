
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

int main(int argc, char const *argv[]) {

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
    err |= sigdelset(&mask, SIGUSR1);
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

    sem = mutex_init(2);

    char nickname[NICKNAME_SIZE];
    printf("Insert your nickname: ");
    readln(nickname, NICKNAME_SIZE);
    nickname[strlen(nickname)-1] = '\0';
    char command[COMMAND_SIZE];
    printf("Do you want to join or create a channel?\n");
    printf("(use :join or :create)\n> ");
    int join = 0;
    int create = 0;
    while(1){
        readln(command, COMMAND_SIZE);
        command[strlen(command)-1] = '\0'; // readln adds '\'
        if(strcmp(command, JOIN_COMMAND) == 0) {
            join = 1;
            break;
        } else if(strcmp(command, CREATE_COMMAND) == 0){
            create = 1;
            break;
        } else {
            printf("Please enter a valid command (:join or :create)\n> ");
            continue;
        }
    }
    char channel[CHNAME_SIZE];
    if(join)
        printf("Which channel do you want to join?\n> ");
    else if(create)
        printf("What is the name of your new channel?\n> ");
    readln(channel, CHNAME_SIZE);
    channel[strlen(channel)-1] = '\0';

    unsigned short server_port = htons(SERVER_PORT);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(sockfd, "socket()");

    struct sockaddr_in server_addr = {0};
    int addr_size = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = server_port;
    inet_aton(SERVER_IP, &server_addr.sin_addr);

    err = connect(sockfd, (struct sockaddr*) &server_addr, addr_size);
    ERROR_HELPER(err, "connect()");

    send_packet(sockfd, nickname, NICKNAME_SIZE);
    if (join) {
        send_packet(sockfd, JOIN_COMMAND, COMMAND_SIZE);
    } else if (create) {
        send_packet(sockfd, CREATE_COMMAND, COMMAND_SIZE);
    }
    send_packet(sockfd, channel, CHNAME_SIZE);

    pthread_create(&threads[0], NULL, send_msg, (void*) sockfd);
    pthread_create(&threads[1], NULL, recv_msg, (void*) sockfd);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    exit(EXIT_SUCCESS);
}


void* send_msg(void* args) {
    int sockfd = (int) args;
    char message[MSG_SIZE];

    while(1) {
        readln(message, MSG_SIZE);
        if(message == "") continue;
        send_stream(sockfd, message, MSG_SIZE);
        LOGd("Sent message to server...");
        memset(message, 0, MSG_SIZE);
    }
    pthread_exit(NULL);
}


void* recv_msg(void* args) {
    int sockfd = (int) args;
    char message[MSG_SIZE];

    while(1) {
        recv_stream(sockfd, message, MSG_SIZE);
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
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
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
    void* err0, err1;
    pthread_join(threads[0], &err0);
    pthread_join(threads[1], &err1);
    if (err0 == PTHREAD_CANCELED && err1 == PTHREAD_CANCELED) {
        char* leave_msg = ":leave|";
        send_stream(sockfd, leave_msg, MSG_SIZE);
        int err = close(sockfd);
        printf("\nBye! C you soon!\n");
        exit(0);
    } else {
        printf("This should not be called...\n");
    }
}


void sigsegv_exit(int unused1, siginfo_t *info, void *unused2) {
    pthread_cancel(threads[0]);
    pthread_cancel(threads[1]);
    void* err0, err1;
    pthread_join(threads[0], &err0);
    pthread_join(threads[1], &err1);
    if (err0 == PTHREAD_CANCELED && err1 == PTHREAD_CANCELED) {
        char* leave_msg = ":leave|";
        send_stream(sockfd, leave_msg, MSG_SIZE);
        int err = close(sockfd);
        printf("Bye! C you soon!\n");
        exit(0);
    } else {
        printf("This should not be called...\n");
    }
