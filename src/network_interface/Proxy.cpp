/**
 * @file Proxy.cpp This file contains the Proxy class implementation. The proxy
 * handles the creation and management of proxy ports for network communication between
 * different application components.
 * @author Carlos Duarte (carlosduarte.molina97@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Proxy.hpp"
#include "nlohmann/json.hpp"
#include "utils/logger.hpp"
#include <vector>
#include <iostream>

Proxy::Proxy()
{
    m_MainAppSocket = new Network::TcpServer(io_context, "lo", "lo", MAIN_APP_PROXY_PORT, 0);
    m_WebAppSocket  = new Network::TcpServer(io_context, "lo", "lo", WEB_APP_PROXY_PORT,  0);

    m_MainAppSocket->setOnConnectionEstablished
    (
        [this]() { Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Main app connection established\r\n"); }
    );
    m_WebAppSocket->setOnConnectionEstablished
    (
        [this]() { Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Web app connection established\r\n"); }
    );

    m_MainAppSocket->startReceive([this](std::vector<char>& data) { onSocketData(data, m_MainAppRxAccum); });
    m_WebAppSocket->startReceive([this](std::vector<char>& data) { onSocketData(data, m_WebAppRxAccum); });

    threadPool.create_thread([this]() { io_context.run(); });
}


Proxy::~Proxy()
{
    if (m_MainAppSocket)
    {
        delete m_MainAppSocket;
        m_MainAppSocket = nullptr;
    }
    if (m_WebAppSocket)
    {
        delete m_WebAppSocket;
        m_WebAppSocket = nullptr;
    }
    threadPool.join_all();
}

void Proxy::onSocketData(std::vector<char>& data, std::vector<char>& accumBuffer)
{
    accumBuffer.insert(accumBuffer.end(), data.begin(), data.end());

    // TCP is a byte stream: a single read can hold a partial message, exactly
    // one message, or several messages concatenated together. Only hand
    // processRequest() a slice that is exactly one framed message, and hold
    // back anything incomplete for the next read.
    while (accumBuffer.size() >= sizeof(ProxyMsgHdr))
    {
        const ProxyMsgHdr* header = reinterpret_cast<const ProxyMsgHdr*>(accumBuffer.data());
        if (header->len < 0 || static_cast<size_t>(header->len) > MaxPayloadLen)
        {
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR,
                "Proxy: invalid message length %d, discarding %zu buffered byte(s)\r\n",
                header->len, accumBuffer.size());
            accumBuffer.clear();
            return;
        }

        const size_t totalMsgLen = sizeof(ProxyMsgHdr) + static_cast<size_t>(header->len);
        if (accumBuffer.size() < totalMsgLen)
        {
            break; // Rest of the message hasn't arrived yet.
        }

        std::vector<char> message(accumBuffer.begin(), accumBuffer.begin() + totalMsgLen);
        processRequest(message);

        accumBuffer.erase(accumBuffer.begin(), accumBuffer.begin() + totalMsgLen);
    }
}

int Proxy::processRequest(std::vector<char>& data)
{
    if (data.size() < sizeof(ProxyMsgHdr))
    {
        return -1;
    }

    const ProxyMsgHdr* header = reinterpret_cast<const ProxyMsgHdr*>(data.data());
    const uint8_t* payload = reinterpret_cast<const uint8_t*>(data.data() + sizeof(ProxyMsgHdr));
    if (header->len < 0 || sizeof(ProxyMsgHdr) + static_cast<size_t>(header->len) > data.size())
    {
        Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Proxy: message length %d inconsistent with buffer size %zu\r\n", header->len, data.size());
        return -1;
    }
    size_t payloadLength = static_cast<size_t>(header->len);
    std::vector<char> replyVec;

    using json = nlohmann::json;
    
    // Route the request based on the destination address in the header
    switch (header->destAddr)
    {
        case UpdaterRouteAddr:
        {
            if (onDataReceived)
            {
                try
                {
                    nlohmann::json requestJson = nlohmann::json::parse(payload, payload + payloadLength);
                    replyVec = onDataReceived(requestJson);
                }
                catch (const nlohmann::json::parse_error& e)
                {
                    json j =
                    {
                        {"status", false}
                    };
                    std::copy(j.dump().begin(), j.dump().end(), std::back_inserter(replyVec));
                    Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "JSON parse error: %s\r\n", e.what());
                    return -1; // JSON parse error
                }
            }
            else
            {
                Logger::getLoggerInst()->log(Logger::LOG_LVL_WARN, "No callback set for UpdaterRouteAddr\r\n");
            }
            break;
        }
        case WebAppRouteAddr:
            // Handle web app route
            m_WebAppSocket->transmit(payload, payloadLength);
            break;
        case MainAppRouteAddr:
            // Handle main app route
            m_MainAppSocket->transmit(payload, payloadLength);
            break;
        default:
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Unknown route address: %i\r\n", header->destAddr);
            return -1; // Unknown route
    }

    // If a reply was generated, send it back to the source
    if (!replyVec.empty())
    {
        // Build reply
        int reqSrc = header->srcAddr;
        std::vector<char> fullReply(sizeof(ProxyMsgHdr) + replyVec.size());
        ProxyMsgHdr* replyHeader = reinterpret_cast<ProxyMsgHdr*>(fullReply.data());
        replyHeader->srcAddr  = UpdaterRouteAddr;
        replyHeader->destAddr = header->srcAddr;
        replyHeader->len = static_cast<int>(replyVec.size());
        std::copy(replyVec.begin(), replyVec.end(), fullReply.begin() + sizeof(ProxyMsgHdr));

        if (reqSrc == WebAppRouteAddr)
        {
            m_WebAppSocket->transmit(reinterpret_cast<const uint8_t*>(fullReply.data()), fullReply.size());
        }
        else if (reqSrc == MainAppRouteAddr)
        {
            m_MainAppSocket->transmit(reinterpret_cast<const uint8_t*>(fullReply.data()), fullReply.size());
        }
        else
        {
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Unknown source address: %i\r\n", header->srcAddr);
            return -1; // Unknown source
        }
    }
    return 0;
}