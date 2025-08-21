#include "tcp_socket.hpp"

#include <iostream>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <netinet/in.h> // For sockaddr_in, htons(), htonl()
#include <unistd.h>     // For close()
#include <string>
#include <arpa/inet.h>  // For inet_ntoa()

#include <thread>
#include <cstring>


namespace Network {
TCPSocket::TCPSocket(int sPort, int dPort) {
    this->sport_ = sPort;
    this->dport_ = dPort;

    // Start the data transmission thread
    this->threadCanRun = true;
    this->transmissionThread = std::thread(&TCPSocket::transmissionThreadHandler, this);
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

    sockaddr_in serverAdservaddrdress;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    servaddr.sin_port = htons(this->sport_); // Choose a port number (e.g., 8080)

    if (bind(this->socket_, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        throw(0);
    }
    
    while(true) {
         // Now server is ready to listen and verification 
        if ((listen(this->socket_, 5)) != 0) { 
            printf("Listen failed...\n"); 
            exit(0); 
        }
        
        connfd = accept(this->socket_, (struct sockaddr*)&cli, &len); 
        if (connfd < 0) { 
            printf("server accept failed...\n"); 
            exit(0); 
        }
        
        // We are connected!
        while (true) {
            ssize_t bytesRead = recv(this->socket_, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // Timeout occurred, just continue the loop
                    continue;
                }
            }

            // Fire signal here
            onDataReceived(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(bytesRead));
        }
        
    }
}    
};

