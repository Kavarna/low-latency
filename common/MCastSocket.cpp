#include "MCastSocket.h"
#include "common/Check.h"
#include "common/SocketUtils.h"
#include <cstddef>
#include <cstring>
#include <sys/socket.h>

bool MCastSocket::Init(std::string const &ip, std::string const &iface, i32 port, bool isListening)
{
    socket = CreateSocket(*logger, ip, iface, port, true, isListening, false);
    return socket != -1;
}

bool MCastSocket::Join(std::string const &ip)
{
    return ::Join(socket, ip);
}

void MCastSocket::Leave()
{
    close(socket);
    socket = -1;
}

void MCastSocket::Send(const void *data, size_t len)
{
    memcpy(outboundData.data() + nextSendDataIndex, data, len);
    nextSendDataIndex += len;
    CHECK_FATAL(nextSendDataIndex < BUFFER_SIZE, "MCast socket buffer filled");
}

bool MCastSocket::RecvAndSend()
{
    auto const readSize =
        recv(socket, inboundData.data() + nextRecvDataIndex, BUFFER_SIZE - nextRecvDataIndex, MSG_DONTWAIT);
    if (readSize > 0)
    {
        logger->Log("Read socket = ", socket, "; length = ", readSize, "\n");
        nextRecvDataIndex += readSize;
        recvCallback(this);
    }

    if (nextSendDataIndex > 0)
    {
        size_t n = send(socket, outboundData.data(), nextSendDataIndex, MSG_DONTWAIT | MSG_NOSIGNAL);
        logger->Log("Send socket = ", socket, "; length = ", n, "\n");
    }
    nextSendDataIndex = 0;

    return readSize > 0;
}
