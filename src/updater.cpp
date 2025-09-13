#include "updater.hpp"
#include "network_interface/tcp_socket.hpp"
#include <boost/signals2.hpp> // Added for boost signals
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "network_ipc.h"


int  Updater::fd = -1;
char Updater::buf[256] = { 0 };
int  Updater::endStatus = EXIT_SUCCESS;
ipc_message* Updater::status = nullptr;

std::mutex updateMutex;


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
    if ((!pData) && (pData[length - 1] != 0)) {
        // Log error
        std::cout << "ERROR: Invalid message received\r\n";
        return;
    }

    // Convert to JSON
    std::string request = std::string(reinterpret_cast<const char*>(pData), length);
    nlohmann::json requestJ;
    try
    {
        requestJ = nlohmann::json::parse(request);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return;
    }
    
    nlohmann::json reply;

    // Parse the JSON request and generate a reponse
    const int sPort = requestJ["port"].get<int>();  // Extract the source port
    const uint8_t command = requestJ["command"].get<uint8_t>();
    int ret;

    switch (command) {
        case Updater::INITIATE_UPDATE: {
            // If the process is being started, then the file needs to be opened
            int ret = open(updateFileUpdateLoc, O_RDONLY);
            if (ret >= 0) {
                struct swupdate_request req;
                ret = 0;//swupdate_async_start(&Updater::writeImage, &Updater::getUpdateProgress,
                        //           &Updater::updateEnd, &req, sizeof(req));
                std::cout << "INFO: Initiating update\r\n";
                
                if (ret < 0) {
                    reply["status"] = false;    
                } else {
                    reply["status"] = true;
                }
            } else {
                // Report failure to log

                reply["status"] = false;
            }

            break;
        }
        case Updater::READ_UPDATE_STATUS:{
            bool canProcess = true;
            std::cout << "INFO: Reading status\r\n";
            {
                std::lock_guard<std::mutex> lock(updateMutex);
                if (Updater::status->magic != IPC_MAGIC) {
                    reply["status"] = false;
                    canProcess = false;
                }
            }

            if (canProcess) {
                reply["status"] = true;
            }
            break;
        }
        default:
            break;
    }

    std::string replyString = reply.dump();
    
    // Find out which socket to reply to
    if(sPort == this->webServerPort) {
        this->serverWebApp->transmit(reinterpret_cast<const uint8_t*>(replyString.data()), replyString.length());
    } else {  // Else reply to the main app
        this->serverMainApp->transmit(reinterpret_cast<const uint8_t*>(replyString.data()), replyString.length());
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


/**
 * @brief Callback for swupdate image write
 * 
 */
int Updater::writeImage(char **p, int *size) {
    int ret;
	ret = read(Updater::fd, Updater::buf, sizeof(Updater::buf));
	*p = Updater::buf;
	*size = ret;

	return ret;
}

int Updater::getUpdateProgress(ipc_message *msg) {
    // Placeholder: return 0% progress
    if (!msg) {
        return -1;
    }

    // std::lock_guard<std::mutex> lock(updateMutex);
    // status = msg;
    fprintf(stdout, "Status: %d message: %s\n",
			msg->data.status.current,
			strlen(msg->data.status.desc) > 0 ? msg->data.status.desc : "");


    return 0;
}


/**
 * @brief Update end handler
 * 
 * @param status 
 * @return int 
 */
int Updater::updateEnd(RECOVERY_STATUS status) {
    // Placeholder: simply return the status as integer
    Updater::endStatus = (status == SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
    
    if  (status == SUCCESS) {
		fprintf(stdout, "Executing post-update actions.\n");
		ipc_message msg;
		msg.data.procmsg.len = 0;
		if (ipc_postupdate(&msg) != 0 || msg.type != ACK) {
			fprintf(stderr, "Running post-update failed!\n");
			endStatus = EXIT_FAILURE;
		}

        int ret = close(Updater::fd);
        if (ret < 0) {
            // Log failure to close
        }
	}
    
    return 0;
}