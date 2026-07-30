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

Proxy::Proxy(Msg::CircularBuffer<nlohmann::json>& buff)
    : messageBuffer(buff)
{
    m_MainAppSocket = new Network::TcpServer(io_context, "lo", "lo", MAIN_APP_PROXY_PORT, 0);
    m_WebAppSocket  = new Network::TcpServer(io_context, "lo", "lo", WEB_APP_PROXY_PORT,  0);

    m_MainAppSocket->setOnConnectionEstablished(
        [this]() { Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Main app connection established\r\n"); }
    );
    m_WebAppSocket->setOnConnectionEstablished(
        [this]() { Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "Web app connection established\r\n"); }
    );

    m_MainAppSocket->startReceive(std::bind(&Proxy::processRequest, this, std::placeholders::_1));
    m_WebAppSocket->startReceive(std::bind(&Proxy::processRequest, this, std::placeholders::_1));

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

int Proxy::processRequest(std::vector<char>& data)
{
    if (data.size() < sizeof(ProxyMsgHdr))
    {
        return -1;
    }

    const ProxyMsgHdr* header = reinterpret_cast<const ProxyMsgHdr*>(data.data());
    const uint8_t* payload = reinterpret_cast<const uint8_t*>(data.data() + sizeof(ProxyMsgHdr));
    size_t payloadLength = data.size() - sizeof(ProxyMsgHdr);

    using json = nlohmann::json;
    
    // Route the request based on the destination address in the header
    switch (header->destAddr)
    {
        case UpdaterRouteAddr:
        {
            // Handle local route
            json msg;

            try
            {
                msg = json::parse(payload, payload + payloadLength);
            }
            catch(const std::exception& e)
            {
                Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, e.what());
                return -1; // Return error if JSON parsing fails
            }

            msg["source"] = header->srcAddr;
            messageBuffer.push(msg);
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
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Unknown route address: %s", std::to_string(header->destAddr).c_str());
            return -1; // Unknown route
    }
    return 0;
}

int Proxy::sendMessage(ProxyMessages& msg)
{
    int source = -1;
    try
    {
        if (msg.contains("source"))
        {
            source = msg["source"].get<int>();
        }
        else
        {
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Source not specified in message");
            return -1; // Source not specified
        }

        msg.erase("source");

        // Route
        ProxyMsgHdr header;
        header.srcAddr = source;
        header.destAddr = source; // Assuming the destination is the same as the source for routing
        std::string serialized = msg.dump();
        std::vector<char> data(sizeof(ProxyMsgHdr) + serialized.size());
        std::memcpy(data.data(), &header, sizeof(ProxyMsgHdr));
        std::memcpy(data.data() + sizeof(ProxyMsgHdr), serialized.data(), serialized.size());
        
        if (source == WebAppRouteAddr)
        {
            m_WebAppSocket->transmit(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        }
        else if (source == MainAppRouteAddr)
        {
            m_MainAppSocket->transmit(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        }
        else
        {
            Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, "Unknown source address: %s", std::to_string(source).c_str());
            return -1; // Unknown source
        }
    }
    catch(const std::exception& e)
    {
        Logger::getLoggerInst()->log(Logger::LOG_LVL_ERROR, e.what());
        return -1; // Return error if JSON parsing fails
    }

    return 0;
}