#define SOCKET_NAME "socket"
#define BACKLOG 5

#include <iostream>

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <stdio.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static const int BUF_SIZE = 4096;

void ext(const char *msg, int socket) {
    std::cerr << msg << strerror(errno) << "\n";
    close(socket);
    exit(EXIT_FAILURE);
}

int accept(int sock_fd) {
    int fd = accept(sock_fd, nullptr, nullptr);
    if (fd == -1) {
        ext("accept() failed: ", sock_fd);
    }
    return fd;
}

bool send(int fd, char buffer[], ssize_t len) {
    std::cout << "sending...\n";
    ssize_t left = len, was_sent = 0;

    int tries = 5;

    while (left > 0 && tries > 0) {
        ssize_t sent = write(fd, buffer + was_sent, left);
        if (sent == -1) {
            return false;
        }
        if (sent == 0) {
            --tries;
        }

        was_sent += sent;
        left -= sent;
    }
    return was_sent == len;
}

ssize_t process_request(int fd) {
   char buffer[BUF_SIZE] ;
   memset(buffer, '\0', BUF_SIZE);
   ssize_t size = 0;

   while (true) {
       ssize_t got = read(fd, buffer, BUF_SIZE);
       if (got == 0) break;
       if (got == -1 || !send(fd, buffer, got)) {
            return -1;
       }
       size += got;
   }
   return size;
}

int main(int argc, char* argv[]) {

    if (unlink(SOCKET_NAME) == -1) {
        std::cerr << "unlink() failed: " << strerror(errno) << "\n Let's try anyway!\n";
    };

    struct sockaddr_un sock_domain;
    memset(&sock_domain, 0, sizeof(sock_domain));
    sock_domain.sun_family = AF_UNIX;
    strncpy(sock_domain.sun_path, SOCKET_NAME, sizeof(sock_domain.sun_path) - 1);

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        ext("socket() failed: ", socket_fd);
    }

    if (bind(socket_fd, reinterpret_cast<sockaddr *>(&sock_domain), sizeof(sockaddr_un)) == -1) {
        ext("bind() failed: ", socket_fd);
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        ext("listen() failed: ", socket_fd);
    }

    std::cout << "Ready to serve!\n";
    for (int req = 0; req < BACKLOG; ++req) {
        int client_fd = accept(socket_fd);
        process_request(client_fd);
        close(client_fd);
    }

    if (unlink(SOCKET_NAME) == -1) {
        std::cerr << "unlink() failed: " << strerror(errno) << "\n";
    };

    close(socket_fd);
    return 0;
}
