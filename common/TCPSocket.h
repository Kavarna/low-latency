#pragma once

#include "Logger.h"
#include "SocketUtils.h"
#include "TimeUtils.h"
#include <functional>
#include <netinet/in.h>
#include <string>

class TCPSocket;
void DefaultRecvCallback(TCPSocket *socket, Nanos time);

class TCPSocket
{
public:
    std::size_t TCPBufferSize = 64 * 1024 * 1024;

    TCPSocket(QuickLogger &logger) : logger(logger)
    {
        sendBuffer.resize(TCPBufferSize);
        recvBuffer.resize(TCPBufferSize);
    }

    TCPSocket() = delete;
    TCPSocket(const TCPSocket &) = delete;
    TCPSocket(const TCPSocket &&) = delete;
    TCPSocket &operator=(const TCPSocket &) = delete;
    TCPSocket &operator=(const TCPSocket &&) = delete;

    Socket Connect(std::string const &ip, std::string const &iface, i32 port, bool isListening);
    void Send(void const *data, size_t len);
    bool RecvAndSend();

    void Destroy()
    {
        if (socket != -1)
        {
            close(socket);
            socket = -1;
        }
    }

    ~TCPSocket()
    {
        Destroy();
    }

public:
    Socket socket = -1;

    std::vector<char> sendBuffer;
    size_t nextSendIndex = 0;
    std::vector<char> recvBuffer;
    size_t nextRecvIndex = 0;

    sockaddr_in inAddr;

    std::function<void(TCPSocket *, Nanos time)> recvCallback = DefaultRecvCallback;
    std::string timeStr;

    QuickLogger &logger;
};
