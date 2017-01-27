/**
 * Created by spallas on 27/jan/2017
 */

/*******************************************************************************
 * Wrapper function that sends to a different process the file descriptor
 * of a socket. The other process must have opened the on_thisfd unix domain
 * socket.
 * @return the number of sent bytes.
 */
int send_sockfd(int on_thisfd, int thisfd) {
    int ret;

    char something = 'c';  // not relevant to the real message
    struct msghdr msg;
    struct iovec  iov;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];  // just one descriptor

    iov.iov_base = &something;
    iov.iov_len = sizeof(something);

    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;
    msg.msg_flags   = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = CMSG_LEN(sizeof(int));

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;             // here's the core of the mehanism
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *((int*) CMSG_DATA(cmsg)) = thisfd;  // the content of the cmsg (header of
                                         // the msg message) is the fd to send
    ret = sendmsg(on_thisfd, &msg, 0);
    ERROR_HELPER(ret, "Descriptor comunication failed");

    return ret;
}

/*******************************************************************************
 * Wrapper function that receives the file descriptor of a socket.
 * The other process must have sent the descriptor over the on_thisfd
 * unix domain socket.
 * @param on_thisfd: channel where to receive the socket descriptor
 * @param newfd: where to put received socket descriptor
 * @return length of received data (0 on no data, sizeof(int) on success )
 */
 int recv_sockfd(int on_thisfd, int *newfd) {
    int len;

    char buf[1];
    struct iovec iov;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    msg.msg_name    = 0;
    msg.msg_namelen = 0;
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;
    msg.msg_flags   = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    len = recvmsg(socket, &msg, 0);
    ERROR_HELPER(len, "Could not receive file descriptor");
    if(len > 0) {
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        //move the file descriptor from the header to newfd
        memmove(newfd, CMSG_DATA(cmsg), sizeof(int));
    }

    return len;
 }
