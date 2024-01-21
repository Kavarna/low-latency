#pragma once

#include "common/Logger.h"
#include "common/SocketUtils.h"
#include <functional>

struct MCastSocket
{
    static constexpr const u32 BUFFER_SIZE = 64 * 1024 * 1024;
    MCastSocket(QuickLogger *logger) : logger(logger)
    {
        outboundData.resize(BUFFER_SIZE);
        inboundData.resize(BUFFER_SIZE);
    }

    bool Init(std::string const &ip, std::string const &iface, i32 port, bool isListening);
    bool Join(std::string const &ip);
    void Leave();
    void Send(const void *data, size_t len);
    bool RecvAndSend();

    QuickLogger *logger;
    Socket socket;

    std::vector<char> outboundData;
    size_t nextSendDataIndex = 0;

    std::vector<char> inboundData;
    size_t nextRecvDataIndex = 0;

    std::function<void(MCastSocket *)> recvCallback = nullptr;
};