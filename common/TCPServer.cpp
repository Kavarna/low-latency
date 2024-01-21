#include "TCPServer.h"
#include "Check.h"
#include "STLUtils.h"
#include "SocketUtils.h"
#include "TCPSocket.h"
#include "Types.h"
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>

void DefaultRecvFinishedCallback()
{
}

bool TCPServer::AddSocketToEpoll(TCPSocket *tcpSocket)
{
    epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.ptr = reinterpret_cast<void *>(tcpSocket);

    return (epoll_ctl(efd, EPOLL_CTL_ADD, tcpSocket->socket, &ev) != -1);
}

bool TCPServer::RemoveSocketFromEpoll(TCPSocket *tcpSocket)
{
    return (epoll_ctl(efd, EPOLL_CTL_DEL, tcpSocket->socket, nullptr) != -1);
}

void TCPServer::DeleteSocket(TCPSocket *tcpSocket)
{
    RemoveSocketFromEpoll(tcpSocket);

    RemoveElementFromArray(receiveSockets, tcpSocket);
    RemoveElementFromArray(sendSockets, tcpSocket);
    // RemoveElementFromArray(disconnectedSockets, tcpSocket);
}

void TCPServer::Listen(std::string const &iface, i32 port)
{
    efd = epoll_create(1);
    CHECK_FATAL(efd >= 0, "Couldn't create an epoll error = ", strerror(errno));

    CHECK_FATAL(listenerSocket.Connect("", iface, port, true), "Listener socket failed to connect. Iface = ", iface,
                "; port = ", port, "; error = ", strerror(errno));

    AddSocketToEpoll(&listenerSocket);
}

void TCPServer::Poll()
{
    const i32 maxEvents = 1 + sendSockets.size() + receiveSockets.size();

    const i32 n = epoll_wait(efd, events, maxEvents, 0);
    bool haveNewCoonections = false;
    for (s32 i = 0; i < n; ++i)
    {
        epoll_event &event = events[i];
        auto socket = reinterpret_cast<TCPSocket *>(event.data.ptr);

        if (event.events & EPOLLIN)
        {
            logger.Log("EPOLLIN socket = ", socket->socket, '\n');
            if (socket == &listenerSocket)
            {
                logger.Log("New connection on socket = ", listenerSocket.socket, '\n');
                haveNewCoonections = true;
                continue;
            }

            if (std::find(receiveSockets.begin(), receiveSockets.end(), socket) == receiveSockets.end())
            {
                receiveSockets.push_back(socket);
            }
        }

        if (event.events & EPOLLOUT)
        {
            logger.Log("EPOLLOUT socket = ", socket->socket, '\n');
            if (std::find(sendSockets.begin(), sendSockets.end(), socket) == sendSockets.end())
            {
                sendSockets.push_back(socket);
            }
        }

        if (event.events & (EPOLLERR | EPOLLHUP))
        {
            logger.Log("EPOLLERR socket = ", socket->socket, '\n');

            if (std::find(receiveSockets.begin(), receiveSockets.end(), socket) == receiveSockets.end())
            {
                receiveSockets.push_back(socket);
            }
        }
    }

    while (haveNewCoonections)
    {
        sockaddr_storage addr;
        socklen_t addrLen = sizeof(addr);
        Socket newSocket = accept(listenerSocket.socket, reinterpret_cast<sockaddr *>(&addr), &addrLen);
        if (newSocket == -1)
            break;

        CHECK_FATAL(SetNonBlocking(newSocket) && SetNoDelay(newSocket), "Failed to set attributes for the new socket");
        logger.Log("Accepted new socket = ", newSocket, '\n');

        auto newTCPSocket = new TCPSocket(logger);
        newTCPSocket->socket = newSocket;
        newTCPSocket->recvCallback = recvCallback;

        CHECK_FATAL(AddSocketToEpoll(newTCPSocket), "Failed to add new socket to epoll list");

        logger.Log("Socket = ", listenerSocket.socket, " has new connection\n");

        receiveSockets.push_back(newTCPSocket);
    }
}

void TCPServer::RecvAndSend()
{
    auto recv = false;
    for (auto socket : receiveSockets)
    {
        recv |= socket->RecvAndSend();
    }

    if (recv)
    {
        recvFinishedCallback();
    }

    for (auto socket : sendSockets)
    {
        socket->RecvAndSend();
    }
}
