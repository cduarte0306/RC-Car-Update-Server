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
    // Signal thread to stop and close listening socket to unblock accept()
    this->threadCanRun = false;
    if (this->socket_ != -1) {
        close(this->socket_);
        this->socket_ = -1;
    }

    if (this->transmissionThread.joinable()) {
        this->transmissionThread.join();
    }
}


void TCPSocket::transmissionThreadHandler(void) {
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

    // Start listening once
    if ((listen(this->socket_, 5)) != 0) {
        perror("listen");
        close(this->socket_);
        return;
    }

    while (this->threadCanRun) {
        len = sizeof(cli);
        connfd = accept(this->socket_, (struct sockaddr*)&cli, &len);
        if (connfd < 0) {
            // If accept was interrupted because we closed the listening socket during shutdown,
            // break the loop and exit gracefully.
            if (errno == EBADF || errno == EINVAL) {
                break;
            }

            // If we got interrupted by signal, continue.
            if (errno == EINTR) {
                continue;
            }

            perror("accept");
            // continue accepting other connections
            continue;
        }

        std::cout << "INFO: Connection accepted\r\n";

        // We are connected! read from connfd
        // Set a receive timeout so that recv() doesn't block forever and the thread can stop responsively
        struct timeval tv;
        tv.tv_sec = 1;  // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

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

                // If recv returned -1 due to timeout, loop back to check threadCanRun
                if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
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


bool TCPSocket::transmit(const uint8_t* pBuf, size_t length) {
    if (!pBuf) {
        return false;
    }

    int ret = send(this->connfd, pBuf, length, 0);
    if (ret < 0) {
        std::cerr << "Error sending data" << std::endl;
    }
    
    return true;
}

};

