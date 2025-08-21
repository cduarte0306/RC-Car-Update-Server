#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <thread>
#include "network_interface/tcp_socket.hpp"


class Updater {
public:
    Updater();

    ~Updater() {
        
    }

    void joinThread(void);
private:
    static constexpr int webServerPort     = 5000;
    static constexpr int mainAppServerPort = 5001;

    Network::TCPSocket* serverWebApp= nullptr;
    Network::TCPSocket* serverMainApp = nullptr;
    std::thread mainThread;
    
    void mainProcess();
};

#endif