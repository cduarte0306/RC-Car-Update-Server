#include "updater.hpp"
#include "network_interface/TcpServer.hpp"
#include <boost/signals2.hpp> // Added for boost signals
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <chrono>

#include "utils/logger.hpp"

#include "network_ipc.h"
#include "progress_ipc.h"


int  Updater::fd = -1;
char Updater::buf[256] = { 0 };
int  Updater::endStatus = EXIT_SUCCESS;
ipc_message Updater::updateStatus;

std::mutex updateMutex;
std::queue<ipc_message> messageQueue;


Updater::Updater()
{
    mProxy = new Proxy();
    mProxy->OnMessageReceived(std::bind(&Updater::processRequest, this, std::placeholders::_1));

    progressThread = std::thread(&Updater::progressThreadHandler, this);
}


Updater::~Updater()
{
    progressThreadRunning.store(false);

    // Unblock the progress thread if it is parked in a blocking read on the progress socket
    int fd = progressSocketFd.load();
    if (fd >= 0)
    {
        shutdown(fd, SHUT_RDWR);
    }

    if (progressThread.joinable())
    {
        progressThread.join();
    }

    delete mProxy;
}


/**
 * @brief Connects to swupdate's Progress socket and keeps installPercent up to date.
 *
 * The status callback wired up via swupdate_async_start() (getUpdateProgress) only carries
 * a state code and a text description; swupdate reports install percentage separately via
 * its Progress socket (progress_ipc.h), which this thread subscribes to independently of
 * whichever process actually triggered the update.
 */
void Updater::progressThreadHandler(void)
{
    struct progress_msg msg;
    int connfd = -1;

    while (progressThreadRunning.load())
    {
        if (connfd < 0)
        {
            connfd = progress_ipc_connect(false);
            progressSocketFd.store(connfd);

            if (connfd < 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        int ret = progress_ipc_receive(&connfd, &msg);
        progressSocketFd.store(connfd);

        if (ret <= 0)
        {
            continue;
        }

        installPercent.store(msg.cur_percent);
    }

    if (connfd >= 0)
    {
        close(connfd);
    }
}


std::vector<char> Updater::processRequest(nlohmann::json& requestJson)
{
    nlohmann::json reply;

    // Parse the JSON request and generate a reponse
    const uint8_t command = requestJson["command"].get<uint8_t>();
    int ret;

    reply["status"]        = false;
    reply["update_status"] = -1;
    reply["message"]       = "";
    reply["percentage"]    = 0;

    switch (command)
    {
        case Updater::INITIATE_UPDATE:
        {
            std::string updateFileUpdateLoc = requestJson["file_path"].get<std::string>();
            Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "File path: %s\r\n", updateFileUpdateLoc.c_str());

            while(!messageQueue.empty())
            {
                messageQueue.pop();
            }

            // If the process is being started, then the file needs to be opened
            int ret = open(updateFileUpdateLoc.c_str(), O_RDONLY);
            if (ret >= 0)
            {
                Updater::fd = ret;
                this->installPercent.store(0);
                struct swupdate_request req;
                swupdate_prepare_req(&req);
                req.source = SOURCE_WEBSERVER;
                req.dry_run = RUN_INSTALL;

                const char *tag = "update-triggered-from-webapi";
                strncpy(req.info, tag, sizeof(req.info)-1);
                
                ret = swupdate_async_start(&Updater::writeImage, &Updater::getUpdateProgress,
                                 &Updater::updateEnd, &req, sizeof(req));
                Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Initiating update\r\n");
                
                if (ret < 0)
                {
                    reply["status"] = false;    
                }
                else
                {
                    reply["status"] = true;
                }
            }

            break;
        }
        case Updater::READ_UPDATE_STATUS:
        {
            {
                std::lock_guard<std::mutex> lock(updateMutex);
                reply["status"   ] = true;
                reply["percentage"] = this->getInstallPercentage();

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

    std::vector<char> replyVec(reply.dump().length());
    std::string replyStr = reply.dump();
    std::copy(replyStr.begin(), replyStr.end(), replyVec.begin());
    return replyVec;
}


/**WW
 * @brief Called to wait for the main thread to be killed
 * 
 */
void Updater::joinThread(void)
{
    progressThread.join();
    delete this->serverWebApp;
    delete this->serverMainApp;
}


/**
 * @brief Callback for swupdate image write
 * 
 */
int Updater::writeImage(char **p, int *size)
{
    int ret;
	ret = read(Updater::fd, Updater::buf, sizeof(Updater::buf));
	*p = Updater::buf;
	*size = ret;

	return ret;
}

int Updater::getUpdateProgress(ipc_message *msg)
{
    // Placeholder: return 0% progress
    if (!msg)
    {
        return -1;
    }

    std::lock_guard<std::mutex> lock(updateMutex);
    // std::memcpy(&Updater::updateStatus, msg, sizeof(ipc_message));  
    if (strlen(msg->data.status.desc) > 0)
    {
        messageQueue.push(*msg);
    }

    Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Status: %d message: %s\r\n",
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
int Updater::updateEnd(RECOVERY_STATUS status)
{
    // Placeholder: simply return the status as integer
    Updater::endStatus = (status == SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;
    
    if  (status == SUCCESS)
    {
        Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Executing post-update actions.");        
		ipc_message msg;
		msg.data.procmsg.len = 0;
		if (ipc_postupdate(&msg) != 0 || msg.type != ACK)
        {
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Running post-update failed!");
			endStatus = EXIT_FAILURE;
		}

        int ret = close(Updater::fd);
        if (ret < 0)
        {
            // Log failure to close
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Failed to close update file descriptor.");
        }
	}
    
    return 0;
}