#include "log.h"

int logfd;

void log_init() {
    logfd = open(FILE_NAME, O_WRONLY | O_APPEND | O_CREAT, 0600);
    if(logfd<0 && errno == EEXIST) return;
    ERROR_HELPER(logfd, "Unable to open log file");
    close(2);
    int err = dup(logfd);
    ERROR_HELPER(err, "Unable to redirect stderr in log_init()");
    //	getting current time and date
    time_t mytime;
    mytime = time(NULL);

    char* date = ctime(&mytime);
    fprintf(stderr, "\n%s%s\n", date, "Mychannel log started");
}


void log_init_name(char *filename) {
    logfd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0600);
    if(logfd<0 && errno == EEXIST) return;
    ERROR_HELPER(logfd, "Unable to open log file");
    close(2);
    int err = dup(logfd);
    ERROR_HELPER(err, "Unable to redirect stderr in log_init()");
    //	getting current time and date
    time_t mytime;
    mytime = time(NULL);

    char* date = ctime(&mytime);
    fprintf(stderr, "\n%s%s\n", date, "Mychannel log started");
}


void log_debug(char* info) {
#ifdef DEBUG
    sigset_t set;
    int err;
	err = sigemptyset(&set);
    err |= sigfillset(&set);
	err |= sigprocmask(SIG_BLOCK, &set, NULL);

    fprintf(stderr, "[DEBUG]: %s\n", info);

    err |= sigprocmask(SIG_UNBLOCK, &set, NULL);
    if(err != 0) fprintf(stderr, "%s: %s\n", "Logger error", strerror(errno));
#else
    return;
#endif
}


void log_error(char* info) {
    sigset_t set;
    int err;
	err = sigemptyset(&set);
    err |= sigfillset(&set);
	err |= sigprocmask(SIG_BLOCK, &set, NULL);

    fprintf(stderr, "[ERROR]: %s\n", info);

    err |= sigprocmask(SIG_UNBLOCK, &set, NULL);
    if(err != 0) fprintf(stderr, "%s: %s\n", "Logger error", strerror(errno));
}


void log_info(char* info) {
    sigset_t set;
    int err;
	err = sigemptyset(&set);
    err |= sigfillset(&set);
	err |= sigprocmask(SIG_BLOCK, &set, NULL);

    fprintf(stdout, "[INFO]: %s\n", info);

    err |= sigprocmask(SIG_UNBLOCK, &set, NULL);
    if(err != 0) fprintf(stderr, "%s: %s\n", "Logger error", strerror(errno));
}


void log_save() {
    fsync(logfd);
    close(logfd);
}
