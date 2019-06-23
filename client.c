#define SOCKET_NAME "socket"
#define true 1
#define false 0
#define MSGSIZE 1024

#include <sys/msg.h>

#include <netinet/in.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static char *msg = "Hello, world!\0";

typedef short bool;

const int r_msgtype = 2, w_msgtype = 1;

typedef struct msgbuf {
    long mtype;
    char mtext[MSGSIZE];
}message_buffer;


void ext(const char *msg, int socket) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    close(socket);
    exit(EXIT_FAILURE);
}

bool send_message(int fd, const char *msg) {
    printf("Sending request...\n");
    size_t size = strlen(msg);

    if (size > MSGSIZE) {
        fprintf(stderr, "Message is too long\n");
        return false;
    }

    message_buffer msg_buf;
    msg_buf.mtype = w_msgtype;

    strcpy(&(msg_buf.mtext), msg);
    if (msgsnd(fd, (void *) &msg_buf, sizeof(msg_buf.mtext), IPC_NOWAIT) == -1) {
        return false;
    }

    return true;
}

bool recieve_message(int fd, size_t size, char *buffer) {
    message_buffer msg;
    buffer[size] = '\0';

    while (true) {
       ssize_t recieved = msgrcv(fd, (void *) &msg, MSGSIZE, r_msgtype, IPC_NOWAIT);
       if (recieved != -1) {
           strcpy(buffer, &(msg.mtext));
           break;
       }
       if (errno != ENOMSG)
           return false;
    }
    printf(buffer);
    return true;
}

int get_fd(int sock_fd) {
    unsigned char buf[5];
    buf[4] = '\0';
    if (read(sock_fd, buf, 4) == -1) {
        return -1;
    }

    int fd = 0;
    for (int i = 0; i < 4; ++i) {
        int temp = buf[3 - i];
        fd |= (temp << i * 8);
    }

    return fd;
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        msg = argv[1];
    }

    struct sockaddr_un sock_domain;
    bzero(&sock_domain, sizeof(sock_domain));
    sock_domain.sun_family = AF_UNIX;
    strncpy(sock_domain.sun_path, SOCKET_NAME, sizeof (sock_domain.sun_path));

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        ext("socket() failed", sock_fd);
    }

    if (connect(sock_fd, (struct sockaddr *)(&sock_domain), sizeof(sock_domain)) == -1) {
        ext("connect() failed", sock_fd);
    }

    int fd = get_fd(sock_fd);

    if (fd == -1) {
        ext("can't retrieve message queue fd", fd);
    }
    printf("Msg queue id was recieved.\n");

    if (!send_message(fd, msg)) {
        ext("msgsnd() failed", sock_fd);
    }

    char buffer[strlen(msg) + 1];
    if (!recieve_message(fd, strlen(msg), buffer)) {
        ext("recieve_message() failed", sock_fd);
    }

    printf("\nResponse:\n%s\n", buffer);
    close(sock_fd);

    return 0;
}
