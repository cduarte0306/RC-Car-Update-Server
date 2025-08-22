#ifndef UDP_SOCKET_HPP
#define UDP_SOCKET_HPP

#include <cstddef>
#include <unistd.h>

#include "sockets.hpp"


namespace Network {

class TCPSocket : public Sockets {
public:
    TCPSocket(int sPort);
    ~TCPSocket();

    void transmissionThreadHandler(void) override;
    int start(const char* interface) override {
        if (!interface) return -1;
        
        auto ifaceOpt = this->findInterface(interface);
        if (ifaceOpt == std::nullopt) {
            return -1;
        }

        ifaceAddr = ifaceOpt.value();
        this->transmissionThread = std::thread(&TCPSocket::transmissionThreadHandler, this);
        // Start the data transmission thread
        this->threadCanRun = true;
        return 0;
    }
private:
    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int MAX_CONNECTIONS = 10;
    static constexpr int TIMEOUT = 5000;

    std::string ifaceAddr  = "";
    int socket_ = -1;

    bool threadCanRun = true;
    struct sockaddr_in lastClientAddress;
private:
};

}

#endif