
#include "Logger.h"
#include "TCPSocket.h"
#include "TimeUtils.h"
#include "server/TCPServer.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <ratio>
#include <thread>

TEST(Basic, SimpleTest)
{
    QuickLogger logger("server.txt");

    auto tcpServerRecvCallback = [&](TCPSocket *socket, Nanos time) {
        logger.Log("tcpServerRecvCallback(): socket = ", socket->socket, "; length = ", socket->nextRecvIndex,
                   "; time = ", time, '\n');
        std::string reply = "TCPServer received msg: " + std::string(socket->recvBuffer.data(), socket->nextRecvIndex);
        socket->nextRecvIndex = 0;

        socket->Send(reply.data(), reply.size());
    };

    auto tcpServerRecvFinishedCallback = [&]() { logger.Log("tcpServerRecvFinishedCallback()\n"); };

    auto tcpClientRecvCallback = [&](TCPSocket *socket, Nanos time) {
        std::string recvMsg = std::string(socket->recvBuffer.data(), socket->nextRecvIndex);
        socket->nextRecvIndex = 0;
        logger.Log("tcpClientRecvCallback(): socket = ", socket->socket, "; length = ", recvMsg.size(),
                   "; msg = ", recvMsg, '\n');
    };

    const std::string iface = "lo";
    const std::string ip = "127.0.0.1";
    const int port = 6969;

    logger.Log("Creating server on iface = ", iface, " and port = ", port, "\n");
    TCPServer server(logger);
    server.recvCallback = tcpServerRecvCallback;
    server.recvFinishedCallback = tcpServerRecvFinishedCallback;
    server.Listen(iface, port);

    std::vector<TCPSocket *> clients(5);
    for (s32 i = 0; i < 5; ++i)
    {
        logger.Log("~~~~~~ Client number ", i, "\n");
        clients[i] = new TCPSocket(logger);
        clients[i]->recvCallback = tcpClientRecvCallback;

        logger.Log("Connecting TCPClient ", i, " to ip = ", ip, "; iface = ", iface, "; port = ", port, '\n');
        clients[i]->Connect(ip, iface, port, false);
        server.Poll();
        logger.Log("~~~~~~ Client number ", i, "\n");
    }

    for (u32 itr = 0; itr < 5; ++itr)
    {
        logger.Log("~~~~~~ Iter number ", itr, "\n");
        for (size_t i = 0; i < clients.size(); ++i)
        {
            logger.Log("~~~~~~ Client number ", i, "\n");
            const std::string clientMsg =
                "CLIENT-[" + std::to_string(i) + "] : Sending " + std::to_string(itr * 100 + i);

            logger.Log("Sending TCPClient[", i, "] ", clientMsg, "\n");
            clients[i]->Send(clientMsg.data(), clientMsg.size());

            clients[i]->RecvAndSend();

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            server.Poll();
            server.RecvAndSend();
            logger.Log("~~~~~~ Client number ", i, "\n");
        }
        logger.Log("~~~~~~ Iter number ", itr, "\n");
    }
}
