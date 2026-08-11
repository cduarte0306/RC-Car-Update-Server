#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <thread>
#include <string>
#include <mutex>
#include <atomic>
#include "nlohmann/json.hpp"

#include "lib/MessageLib.hpp"
#include "network_interface/TcpServer.hpp"
#include "network_ipc.h"
#include "progress_ipc.h"
#include "network_interface/Proxy.hpp"

class Updater {
public:
    Updater();

    ~Updater();

    void joinThread(void);

    /**
     * @brief Get the last known installation progress reported by swupdate
     *
     * @return unsigned int Installation percentage of the current step (0-100)
     */
    unsigned int getInstallPercentage(void) const {
        return this->installPercent.load();
    }

private:
    enum {
        INITIATE_UPDATE,
        READ_UPDATE_STATUS
    };

    static constexpr int         webServerPort       = 5000;
    static constexpr int         mainAppServerPort   = 5001;

    static int fd;
    static char buf[256];
    static int endStatus;
    static ipc_message updateStatus;

    Proxy* mProxy = nullptr;

    Network::TcpServer* serverWebApp  = nullptr;
    Network::TcpServer* serverMainApp = nullptr;

    std::thread progressThread;
    std::atomic<bool> progressThreadRunning{true};
    std::atomic<int> progressSocketFd{-1};
    std::atomic<unsigned int> installPercent{0};

    static int writeImage(char **p, int *size);
    static int getUpdateProgress(ipc_message *msg);
    static int updateEnd(RECOVERY_STATUS status);

    std::vector<char> processRequest(nlohmann::json& requestJson);
    void progressThreadHandler(void);
};

#endif