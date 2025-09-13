#include "tcp_socket.hpp"

#include <iostream>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <netinet/in.h> // For sockaddr_in, htons(), htonl()
#include <unistd.h>     // For close()
#include <string>
#include <arpa/inet.h>  // For inet_ntoa()

#include <thread>
#include <cstring>

#include <iostream>


namespace Network {
TCPSocket::TCPSocket(int sPort) {
    this->sport_ = sPort;
}


TCPSocket::~TCPSocket() {
    this->threadCanRun = false;
    this->transmissionThread.join();
}


void TCPSocket::transmissionThreadHandler(void) {
    int connfd = -1;
    struct sockaddr_in servaddr, cli;
    socklen_t len;

    uint8_t buffer[512];
    std::memset(buffer, 0, sizeof(buffer));

    this->socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (this->socket_ < 0) {
        perror("socket");
        return;
    }

    // Allow quick reuse of the address
    int opt = 1;
    setsockopt(this->socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    servaddr.sin_port = htons(this->sport_); // Choose a port number
    if (!this->ifaceAddr.empty()) {
        inet_pton(AF_INET, this->ifaceAddr.c_str(), &servaddr.sin_addr);
    }

    if (bind(this->socket_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        close(this->socket_);
        return;
    }

    while (this->threadCanRun) {
        // Now server is ready to listen
        if ((listen(this->socket_, 5)) != 0) {
            perror("listen");
            close(this->socket_);
            return;
        }

        len = sizeof(cli);
        connfd = accept(this->socket_, (struct sockaddr*)&cli, &len);
        if (connfd < 0) {
            perror("accept");
            // continue accepting other connections
            continue;
        }

        std::cout << "INFO: Connection accepted\r\n";

        // We are connected! read from connfd
        while (this->threadCanRun) {
            ssize_t bytesRead = recv(connfd, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    // client closed connection
                    break;
                }
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // Non-fatal, continue
                    continue;
                }
                perror("recv");
                break;
            }

            // Fire signal here
            onDataReceived(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(bytesRead));
        }

        close(connfd);
        std::cout << "INFO: Connection closed\r\n";
    }
}    
};

