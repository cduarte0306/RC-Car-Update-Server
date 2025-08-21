#ifndef UDP_SOCKET_HPP
#define UDP_SOCKET_HPP

#include <cstddef>
#include <unistd.h>

#include "sockets.hpp"


namespace Network {

class TCPSocket : public Sockets {
public:
    TCPSocket(int sPort, int dPort=-1);
    ~TCPSocket();

    void transmissionThreadHandler(void) override;

private:
    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int MAX_CONNECTIONS = 10;
    static constexpr int TIMEOUT = 5000;
    int socket_ = -1;

    bool threadCanRun = true;
    struct sockaddr_in lastClientAddress;
private:
};

}

#endif