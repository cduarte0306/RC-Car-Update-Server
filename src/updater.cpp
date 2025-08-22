#include "updater.hpp"
#include "network_interface/tcp_socket.hpp"
#include <boost/signals2.hpp> // Added for boost signals
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>
#include "network_ipc.h"


Updater::Updater() {
    this->serverWebApp  = new Network::TCPSocket(this->webServerPort);
    this->serverMainApp = new Network::TCPSocket(this->mainAppServerPort);

    this->serverWebApp->start("lo");
    this->serverMainApp->start("lo");

    // Connect the reception signals
    this->serverWebApp->onDataReceived.connect(
        boost::bind(&Updater::processRequest, this, boost::placeholders::_1, boost::placeholders::_2)
    );
}


void Updater::processRequest(const uint8_t* pData, size_t length) {
    if (!pData) {
        return;
    }

    // Convert to JSON
    std::string request = std::string(reinterpret_cast<const char*>(pData), length);
    nlohmann::json requestJ = nlohmann::json::parse(request);

    // Parse the JSON request and generate a reponse
    const int sPort = requestJ["port"].get<int>();  // Extract the source port
    const uint8_t command = requestJ["command"].get<uint8_t>();
    int ret;

    switch (command) {
        case Updater::INITIATE_UPDATE: {
            struct swupdate_request req;
            swupdate_async_start(&Updater::writeImage, &Updater::getUpdateProgress,
                                &Updater::updateEnd, &req, sizeof(req));
            break;
        }
        case Updater::READ_UPDATE_STATUS:{
            
            break;
        }
        default:
            break;
    }
}


/**WW
 * @brief Called to wait for the main thread to be killed
 * 
 */
void Updater::joinThread(void) {
    this->mainThread.join();

    delete this->serverWebApp;
    delete this->serverMainApp;
}

// ---- Minimal implementations to satisfy linker (placeholders) ----
int Updater::writeImage(char **buf, int *size) {
    // Placeholder: no image to write in this stub implementation
    (void)buf;
    (void)size;
    return 0; // success
}

int Updater::getUpdateProgress(ipc_message *msg) {
    // Placeholder: return 0% progress
    if (msg) {
        return -1;
    }
    return 0;
}

int Updater::updateEnd(RECOVERY_STATUS status) {
    // Placeholder: simply return the status as integer
    (void)status;
    return 0;
}