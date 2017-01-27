/**
 * Created by spallas on 27/jan/2017
 */

#include "server.h"

/*******************************************************************************
 * Wrapper function that sends to a different process the file descriptor
 * of a socket. The other process must have opened the on_thisfd unix domain
 * socket.
 */
int send_sockfd(int on_thisfd, int thisfd);

/*******************************************************************************
 * Wrapper function that receives the file descriptor of a socket.
 * The other process must have sent the descriptor over the on_thisfd
 * unix domain socket.
 */
int recv_sockfd(int on_thisfd, int* newfd);
