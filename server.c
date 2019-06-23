#define SOCKET_NAME "socket"
#define BACKLOG 5
#define true 1
#define false 0
#define MSGSIZE 1024

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

const int r_msgtype = 1, w_msgtype = 2;

typedef short bool;

typedef struct msgbuf {
    long mtype;
    char mtext[MSGSIZE];
}message_buffer;

void ext(const char *msg, int socket) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    close(socket);
    exit(EXIT_FAILURE);
}

int accept_connection(int sock_fd) {
    int fd = accept(sock_fd, NULL, NULL);
    if (fd == -1) {
        ext("accept() failed", sock_fd);
    }
    printf("Connection was accepted\n");
    return fd;
}

int send_msgq_fd(int sock_fd) {
    int msgq_fd = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msgq_fd == -1) {
        ext("msgget() failed", sock_fd);
    }

    unsigned char buffer[5];
    for (int i = 0; i < 4; ++i) {
        buffer[3 - i] = (msgq_fd >> (8 * i));
    }

    fprintf(stdout, "%s\n","Sending msg queue id...");

    ssize_t sent = write(sock_fd, buffer, 4);
    if (sent == -1) {
       return -1;
    }

    int tries = 3;
    while (sent == 0 && tries != 0) {
        sent = write(sock_fd, buffer, 4);
        --tries;
    }
    if (sent == 0) {
        return -1;
    }

    return (sent == 4) ? msgq_fd : -1;
}

bool send_message(int fd, const char *msg) {
    printf("Sending response...\n");
    ssize_t size = strlen(msg);

    message_buffer msg_buf;
    msg_buf.mtype = w_msgtype;

    strcpy(&(msg_buf.mtext), msg);

    if (msgsnd(fd, (void *) &msg_buf, sizeof(msg_buf.mtext), IPC_NOWAIT) == -1) {
        return false;
    }

    return true;
}

void process_request(int sock_fd) {
    message_buffer msg;
    memset(&msg, '\0', sizeof (msg));

   ssize_t size = 0;

   int msgq_fd = send_msgq_fd(sock_fd);
   if (msgq_fd == -1) {
       ext("Can't send message_queue_fd: ", sock_fd);
   }
   printf("Msg queue fd was sent.\n");

   msg.mtype = r_msgtype;

   while (true) {
       ssize_t recieved = msgrcv(msgq_fd, (void *) &msg, MSGSIZE, r_msgtype, IPC_NOWAIT);
       if (recieved != -1)
           break;
       if (errno != ENOMSG)
           ext("msgrcv() failed", sock_fd);
   }

   printf("Received message: %s\n", msg.mtext);

   if (!send_message(msgq_fd, msg.mtext)) {
       ext("msgsnd() failed", sock_fd);
   }
   printf("Message was sent\n");
   if (msgctl(msgq_fd, IPC_RMID, 0) != 0) {
       fprintf(stderr, "Can't remove message queue with id %d\n", msgq_fd);
   }
}

int main(int argc, char* argv[]) {
    if (unlink(SOCKET_NAME) == -1) {
       fprintf(stderr, "%s: %s\n%s\n", "unlink() failed", strerror(errno), "Let's try anyway!");
    };

    struct sockaddr_un sock_domain;
    memset(&sock_domain, 0, sizeof(sock_domain));
    sock_domain.sun_family = AF_UNIX;
    strncpy(sock_domain.sun_path, SOCKET_NAME, sizeof(sock_domain.sun_path) - 1);

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        ext("socket() failed", socket_fd);
    }

    if (bind(socket_fd, (struct sockaddr *)(&sock_domain), sizeof(struct sockaddr_un)) == -1) {
        ext("bind() failed", socket_fd);
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        ext("listen() failed", socket_fd);
    }

    printf("%s\n", "Ready to serve!");

    for (int req = 0; req < BACKLOG; ++req) {
        int client_fd = accept_connection(socket_fd);
        process_request(client_fd);
        close(client_fd);
    }

    if (unlink(SOCKET_NAME) == -1) {
         fprintf(stderr, "%s: %s\n", "unlink() failed", strerror(errno));
    };

    close(socket_fd);
    return 0;
}
