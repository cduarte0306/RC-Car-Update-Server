#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <thread>
#include <string>
#include <mutex>

#include "network_interface/tcp_socket.hpp"
#include "network_ipc.h"


class Updater {
public:
    Updater();

    ~Updater() {
        
    }

    void joinThread(void);
private:
    enum {
        INITIATE_UPDATE,
        READ_UPDATE_STATUS
    };

    static constexpr int         webServerPort       = 5000;
    static constexpr int         mainAppServerPort   = 5001;
    static constexpr const char* updateFileUpdateLoc = "/home/images/";

    static int fd;
    static char buf[256];
    static int endStatus;
    static ipc_message* status;

    Network::TCPSocket* serverWebApp= nullptr;
    Network::TCPSocket* serverMainApp = nullptr;
    std::thread mainThread;

    static int writeImage(char **p, int *size);
    static int getUpdateProgress(ipc_message *msg);
    static int updateEnd(RECOVERY_STATUS status);
    
    void processRequest(const uint8_t* pData, size_t length);
    void mainProcess();
};

#endif