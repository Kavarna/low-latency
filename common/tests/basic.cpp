#include "SafeQueue.h"
#include "SocketUtils.h"
#include "common/Check.h"
#include "common/Logger.h"
#include "common/MemoryPool.h"
#include "common/TCPServer.h"
#include "common/ThreadUtils.h"
#include "common/TimeUtils.h"

#include <fstream>
#include <gtest/gtest.h>

#include <string>

void MyFunction(int firstArgument)
{
    SHOWINFO(firstArgument);
    while (firstArgument-- > 0)
        ;
}

TEST(Basic, ThreadExample)
{
    auto thread1 = CreateAndStartThread(1, "MyFunctionAffinity", MyFunction, 100);
    auto thread2 = CreateAndStartThread(-1, "MyFunctionNoAffinity", MyFunction, 25);

    thread1->join();
    thread2->join();
}

TEST(Basic, MemoryPoolExample)
{
    struct MyStruct
    {
        int d[3];

        MyStruct() = default;
        MyStruct(int a, int b, int c)
        {
            d[0] = a;
            d[1] = b;
            d[2] = c;
        }
    };

    MemoryPool<f64> primPool(50);
    MemoryPool<MyStruct> structPool(50);

    for (u32 i = 0; i < 50; ++i)
    {
        double *elem1 = primPool.Allocate((f64)i * 3);
        MyStruct *elem2 = structPool.Allocate(i * 3, i * 3 + 1, i * 3 + 2);

        SHOWINFO("elem1: ", *elem1, " allocated at: ", elem1);
        SHOWINFO("elem2: {", elem2->d[0], ", ", elem2->d[1], ", ", elem2->d[2], "} allocated at: ", elem2);

        if (i % 5 == 0)
        {
            SHOWINFO("Deallocating elem1: ", *elem1, " from: ", elem1);
            SHOWINFO("Deallocating elem2: {", elem2->d[0], ", ", elem2->d[1], ", ", elem2->d[2], "} from: ", elem2);

            primPool.Deallocate(elem1);
            structPool.Deallocate(elem2);
        }
    }
}

TEST(Basic, SafeQueueExample)
{
    struct MyStruct
    {
        f64 values[3];
    };
    SafeQueue<MyStruct> lfss(50);
    std::atomic<bool> shouldStop = false;
    auto consumeFunction = [&] {
        while (!shouldStop || lfss.GetSize() != 0)
        {
            if (lfss.GetSize() != 0)
            {
                MyStruct const *ms = lfss.GetNextRead();
                SHOWINFO("Reading from the queue values: {", ms->values[0], ", ", ms->values[1], ", ", ms->values[2],
                         "}")
                lfss.UpdateReadIndex();
            }
        }
    };

    auto ct = CreateAndStartThread(-1, "Consumer", consumeFunction);
    for (u32 i = 0; i < 50; ++i)
    {
        MyStruct *ms = lfss.GetNextWriteTo();

        ms->values[0] = i * 3 + 0;
        ms->values[1] = i * 3 + 1;
        ms->values[2] = i * 3 + 2;

        SHOWINFO("Writing to the queue values: {", ms->values[0], ", ", ms->values[1], ", ", ms->values[2], "}")
        lfss.UpdateWriteIndex();
    }
    shouldStop = true;
    ct->join();
}

TEST(Basic, Time)
{
    std::string currentTime;
    GetCurrentTimeStr(currentTime);
    SHOWINFO("Current time as string: ", currentTime);
    SHOWINFO(GetCurrentNanos());
}

TEST(Basic, Logger)
{
    {
        QuickLogger logger("output.txt");
        logger.Log("My logger is fancy ", 5, ' ', 'd', '\n');
    }
    {
        std::string expectedString = "My logger is fancy 5 d\n";
        std::string actualString;
        actualString.resize(expectedString.size());
        std::ifstream fin("output.txt");
        EXPECT_TRUE(fin.is_open());
        fin.read(actualString.data(), actualString.size());
        EXPECT_TRUE(fin.good());
        EXPECT_EQ(expectedString, actualString);
    }
}

TEST(Basic, SocketUtils)
{
    SHOWINFO(GetIFaceIP("ens160"));
}

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
    const i32 port = 6969;

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
