#define SOCKET_NAME "socket"

#include <iostream>

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

static const int BUF_SIZE = 4096;
static std::string msg = "Hello, world!";

void ext(const char *msg, int socket) {
    std::cerr << msg << strerror(errno) << "\n";
    close(socket);
    exit(EXIT_FAILURE);
}

bool send(int sock_fd, const std::string &msg) {
    ssize_t size = static_cast<ssize_t>(msg.size());
    ssize_t was_sent = 0, left = size;

    int tries = 5;

    while (left > 0 && tries > 0) {
        ssize_t sent = write(sock_fd, msg.data() + was_sent, left);
        if (sent == -1) {
            return false;
        }
        if (sent == 0) {
            --tries;
        }

        was_sent += sent;
        left -= sent;
    }
    return was_sent == size;
}

bool get_response(int sock_fd, size_t msg_len, std::string &response) {
    char buffer[BUF_SIZE];
    bzero(buffer, BUF_SIZE);

    ssize_t got = 0;
    int tries = 5;

    while (got != msg_len && tries > 0) {
        ssize_t r = read(sock_fd, buffer, BUF_SIZE);
        if (r == -1) return false;
        if (r == 0) --tries;

        for (ssize_t i = 0; i < r; ++i) {
            response += buffer[i];
         }
        got += r;

    }
    return msg_len == got;
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        msg = std::move(argv[1]);
    }

    struct sockaddr_un sock_domain;
    bzero(&sock_domain, sizeof(sock_domain));
    sock_domain.sun_family = AF_UNIX;
    strncpy(sock_domain.sun_path, SOCKET_NAME, sizeof (sock_domain.sun_path));

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        ext("socket() failed: ", sock_fd);
    }

    if (connect(sock_fd, reinterpret_cast<sockaddr *>(&sock_domain), sizeof(sock_domain)) == -1) {
        ext("connect() failed: ", sock_fd);
    }
    if (!send(sock_fd, msg)) {
        ext("send() failed:", sock_fd);
    }

    std::string response = "Response: \n";
    if (!get_response(sock_fd, msg.size(), response)) {
        ext("get_response() failed: ", sock_fd);
    }
    std::cout << response;

    close(sock_fd);
    return 0;
}
