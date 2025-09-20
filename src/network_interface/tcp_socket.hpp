#ifndef TCP_SOCKET_HPP
#define TCP_SOCKET_HPP

#include <cstddef>
#include <unistd.h>
#include <mutex>

#include "sockets.hpp"


namespace Network {

class TCPSocket : public Sockets {
public:
    TCPSocket(int sPort);
    ~TCPSocket();

    void transmissionThreadHandler(void) override;
    bool transmit(const uint8_t* pBuf, size_t length) override;
    int start(const char* interface) override {
        if (!interface) return -1;
        
        auto ifaceOpt = this->findInterface(interface);
        if (ifaceOpt == std::nullopt) {
            return -1;
        }

        ifaceAddr = ifaceOpt.value();
        // Ensure the flag is set before launching the thread so the thread sees the correct state immediately
        this->threadCanRun = true;
        this->transmissionThread = std::thread(&TCPSocket::transmissionThreadHandler, this);
        return 0;
    }
private:
    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int MAX_CONNECTIONS = 10;
    static constexpr int TIMEOUT = 5000;

    std::string ifaceAddr  = "";
    int socket_ = -1;
    int connfd = -1;

    // Use the `threadCanRun` flag from the base `Sockets` class (do not shadow it here).
    struct sockaddr_in lastClientAddress;
private:
        std::mutex swupdateProgressMutex;
};

}

#endif