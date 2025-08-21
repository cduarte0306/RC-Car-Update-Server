#include "updater.hpp"
#include "network_interface/tcp_socket.hpp"
#include <boost/signals2.hpp> // Added for boost signals
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>


Updater::Updater() {
    this->serverWebApp  = new Network::TCPSocket(this->webServerPort);
    this->serverMainApp = new Network::TCPSocket(this->mainAppServerPort);

    // Connect the reception signals
    this->serverWebApp->onDataReceived.connect(
        boost::bind(&RcCar::processCommand, this, _1, _2)
    );

}


void Updater::processRequest(const uint8_t* pData, size_t length) {
    if (!pData) {
        return;
    }

    // Convert to JSON
    const std::string request = std::string(pData);
    nlohmann::json requestJ = nlohmann::json::parse(request);
}


/**
 * @brief Called to wait for the main thread to be killed
 * 
 */
void Updater::joinThread(void) {
    this->mainThread.join();

    delete this->serverWebApp;
    delete this->serverMainApp;
}