#pragma once

#include "Logger.h"
#include "TCPSocket.h"
#include <sys/epoll.h>

void DefaultRecvFinishedCallback();

class TCPServer
{
public:
    TCPServer(QuickLogger &logger) : listenerSocket(logger), logger(logger)
    {
        recvCallback = DefaultRecvCallback;
        recvFinishedCallback = DefaultRecvFinishedCallback;
    }

    TCPServer() = delete;
    TCPServer(const TCPServer &) = delete;
    TCPServer(const TCPServer &&) = delete;
    TCPServer &operator=(const TCPServer &) = delete;
    TCPServer &operator=(const TCPServer &&) = delete;

    void Listen(std::string const &iface, int port);

    bool AddSocketToEpoll(TCPSocket *tcpSocket);
    bool RemoveSocketFromEpoll(TCPSocket *tcpSocket);

    void DeleteSocket(TCPSocket *tcpSocket);

    void Poll();

    void RecvAndSend();

    void Destroy()
    {
        if (efd != -1)
        {
            close(efd);
            efd = -1;
        }

        listenerSocket.Destroy();
    }

    int efd = -1;
    TCPSocket listenerSocket;
    epoll_event events[1024];

    std::vector<TCPSocket *> receiveSockets, sendSockets;

    std::function<void(TCPSocket *, Nanos)> recvCallback;
    std::function<void()> recvFinishedCallback = DefaultRecvFinishedCallback;

    std::string timeStr;

    QuickLogger &logger;
};
