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

#include <functional>
#include <cstdint>
#include <string>
#include <map>

#include <boost/signals2.hpp> // Added for boost signals
#include <boost/thread.hpp> // Added for boost thread group

#include "TcpServer.hpp"
#include "lib/MessageLib.hpp"
#include <nlohmann/json.hpp>

class Proxy
{
public:
    struct ProxyMsgHdr
    {
        int srcAddr;   /*!< Source address */
        int destAddr;  /*!< Destination address */
        int len;
    };

    Proxy();
    ~Proxy();

    int setProxyCallback( boost::signals2::signal<void(const uint8_t* data, size_t length)>::slot_type slot )
    {
        // this->onDataReceived.connect(slot);
        return 0;
    }

    int OnMessageReceived(std::function<std::vector<char>(nlohmann::json&)> callback)
    {
        onDataReceived = std::move(callback);
        return 0;
    }
private:
    enum {
        UpdaterRouteAddr = 0x01, /*!< Local application route address */
        WebAppRouteAddr  = 0x02, /*!< Web application route address */
        MainAppRouteAddr = 0x03  /*!< Main application route address */
    };

    struct ProxyMessage
    {
        ProxyMsgHdr header;          /*!< Proxy message header */
        uint8_t    payload[512];    /*!< Proxy message payload */
    };

    constexpr static size_t MaxPayloadLen = sizeof(ProxyMessage::payload); /*!< Sanity bound for a single message's payload length */

    /**
     * @brief Buffer raw bytes from a socket and dispatch exactly one call to
     * processRequest() per complete framed message (header.len prefixed).
     * TCP is a byte stream, so a single socket read can contain a partial
     * message, one message, or several concatenated messages - this
     * reassembles message boundaries before they're parsed as JSON.
     *
     * @param data Raw bytes just received from the socket
     * @param accumBuffer Per-connection accumulation buffer for reassembly
     */
    void onSocketData(std::vector<char>& data, std::vector<char>& accumBuffer);

    /**
     * @brief Process incoming proxy requests
     *
     * @param data Reference to the data vector containing exactly one framed request
     * @return int Returns 0 on success, -1 on failure
     */
    int processRequest(std::vector<char>& data);

    constexpr static uint16_t WEB_APP_PROXY_PORT  = 9091; /*!< Web application facing proxy port */
    constexpr static uint16_t MAIN_APP_PROXY_PORT = 9090; /*!< Main application facing proxy port */

    // Boost thread
    boost::thread_group threadPool;

    std::function<std::vector<char>(nlohmann::json&)> onDataReceived; /*!< Callback function for data received */

    Network::TcpServer* m_WebAppSocket = nullptr; /*!< Pointer to the web application socket */
    Network::TcpServer* m_MainAppSocket = nullptr; /*!< Pointer to the main application socket */
    std::map<std::string , int> proxyPorts; /*!< Map of proxy addresses to port numbers */
    boost::asio::io_context io_context;

    std::vector<char> m_MainAppRxAccum; /*!< Reassembly buffer for the main app socket */
    std::vector<char> m_WebAppRxAccum;  /*!< Reassembly buffer for the web app socket */
};

#pragma endregion