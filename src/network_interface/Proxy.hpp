/**
 * @file Proxy.hpp This file contains the Proxy class definition
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#pragma once

#include <cstdint>
#include <string>
#include <map>

#include <boost/signals2.hpp> // Added for boost signals
#include <boost/thread.hpp> // Added for boost thread group

#include "TcpServer.hpp"
#include "lib/MessageLib.hpp"
#include <nlohmann/json.hpp>

typedef nlohmann::json ProxyMessages;

class Proxy {
public:
    Proxy(Msg::CircularBuffer<nlohmann::json>& buff);
    ~Proxy();

    int setProxyCallback(
        boost::signals2::signal<void(const uint8_t* data, size_t length)>::slot_type slot
    ) {
        // this->onDataReceived.connect(slot);
        return 0;
    }

    int sendMessage(ProxyMessages& msg);
private:
    enum {
        UpdaterRouteAddr = 0x01, /*!< Local application route address */
        WebAppRouteAddr  = 0x02, /*!< Web application route address */
        MainAppRouteAddr = 0x03  /*!< Main application route address */
    };

    struct ProxyMsgHdr {
        int srcAddr;   /*!< Source address */
        int destAddr;  /*!< Destination address */
        int len;
    };

    struct ProxyMessage {
        ProxyMsgHdr header;          /*!< Proxy message header */
        uint8_t    payload[512];    /*!< Proxy message payload */
    };

    /**
     * @brief Process incoming proxy requests
     *
     * @param data Reference to the data vector containing the request
     * @return int Returns 0 on success, -1 on failure
     */
    int processRequest(std::vector<char>& data);

    Msg::CircularBuffer<nlohmann::json>& messageBuffer;

    constexpr static uint16_t WEB_APP_PROXY_PORT = 8080; /*!< Web application proxy port */
    constexpr static uint16_t MAIN_APP_PROXY_PORT = 9090; /*!< Main application proxy port */

    // Boost thread
    boost::thread_group threadPool;

    Network::TcpServer* m_WebAppSocket = nullptr; /*!< Pointer to the web application socket */
    Network::TcpServer* m_MainAppSocket = nullptr; /*!< Pointer to the main application socket */
    std::map<std::string , int> proxyPorts; /*!< Map of proxy addresses to port numbers */
    boost::asio::io_context io_context;
};

#pragma endregion