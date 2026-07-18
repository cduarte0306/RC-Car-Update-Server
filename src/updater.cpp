#include "updater.hpp"
#include "network_interface/TcpServer.hpp"
#include <boost/signals2.hpp> // Added for boost signals
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <queue>

#include "utils/logger.hpp"

#include "network_ipc.h"


int  Updater::fd = -1;
char Updater::buf[256] = { 0 };
int  Updater::endStatus = EXIT_SUCCESS;
ipc_message Updater::updateStatus;

std::mutex updateMutex;
std::queue<ipc_message> messageQueue;


Updater::Updater() : mMessageBuffer(256) {
    // this->serverWebApp  = new Network::TcpServer(this->webServerPort);
    // this->serverMainApp = new Network::TcpServer(this->mainAppServerPort);

    // this->serverWebApp->start("lo");
    // this->serverMainApp->start("lo");

    // Connect the reception signals
    // this->serverWebApp->onDataReceived.connect(
    //     boost::bind(&Updater::processRequest, this, boost::placeholders::_1, boost::placeholders::_2)
    // );

    // this->serverMainApp->onDataReceived.connect(
    //     boost::bind(&Updater::processRequest, this, boost::placeholders::_1, boost::placeholders::_2)
    // );
    mProxy = new Proxy(mMessageBuffer);
}


Updater::~Updater() {
    delete mProxy;
}


void Updater::processRequest(const uint8_t* pData, size_t length) {
    if ((!pData) && (pData[length - 1] != 0)) {
        // Log error
        Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Invalid message received\r\n");
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
        Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, e.what());
        return;
    }

    nlohmann::json reply;

    // Parse the JSON request and generate a reponse
    const int sPort = requestJ["port"].get<int>();  // Extract the source port
    const uint8_t command = requestJ["command"].get<uint8_t>();
    int ret;

    reply["status"]        = false;
    reply["update_status"] = -1;
    reply["message"]       = "";

    switch (command) {
        case Updater::INITIATE_UPDATE: {
            std::string updateFileUpdateLoc = requestJ["file_path"].get<std::string>();
            Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "File path: %s\r\n", updateFileUpdateLoc.c_str());

            while(!messageQueue.empty()) {
                messageQueue.pop();
            }

            // If the process is being started, then the file needs to be opened
            int ret = open(updateFileUpdateLoc.c_str(), O_RDONLY);
            if (ret >= 0) {
                Updater::fd = ret;

                struct swupdate_request req;
                swupdate_prepare_req(&req);
                req.source = SOURCE_WEBSERVER;
                req.dry_run = RUN_INSTALL;

                const char *tag = "update-triggered-from-webapi";
                strncpy(req.info, tag, sizeof(req.info)-1);
                
                ret = swupdate_async_start(&Updater::writeImage, &Updater::getUpdateProgress,
                                 &Updater::updateEnd, &req, sizeof(req));
                Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Initiating update\r\n");
                
                if (ret < 0) {
                    reply["status"] = false;    
                } else {
                    reply["status"] = true;
                }
            }

            break;
        }
        case Updater::READ_UPDATE_STATUS:{
            {
                std::lock_guard<std::mutex> lock(updateMutex);
                reply["status"   ] = true;

                if (!messageQueue.empty()) {
                    ipc_message msg = messageQueue.front();
                    reply["update_status"] = msg.data.status.current;
                    reply["message"] = std::string(msg.data.status.desc);
                    messageQueue.pop();
                }
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

    std::lock_guard<std::mutex> lock(updateMutex);
    // std::memcpy(&Updater::updateStatus, msg, sizeof(ipc_message));  
    if (strlen(msg->data.status.desc) > 0) {
        messageQueue.push(*msg);
    }

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