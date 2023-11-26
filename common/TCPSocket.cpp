#include "TCPSocket.h"
#include "SocketUtils.h"
#include "TimeUtils.h"
#include <asm-generic/socket.h>
#include <bits/types/struct_iovec.h>
#include <cstring>
#include <sys/socket.h>

void DefaultRecvCallback(TCPSocket *socket, Nanos time)
{
    socket->logger.Log("DefaultRecvCallback[Socket = ", socket->socket, "; nextRecvIndex = ", socket->nextRecvIndex,
                       "; time = ", time, "]\n");
}

Socket TCPSocket::Connect(std::string const &ip, std::string const &iface, int port, bool isListening)
{
    socket = CreateSocket(logger, ip, iface, port, false, isListening, true);

    inAddr.sin_addr.s_addr = INADDR_ANY;
    inAddr.sin_port = htons(port);
    inAddr.sin_family = AF_INET;

    return socket;
}

void TCPSocket::Send(void const *data, size_t len)
{
    if (len > 0)
    {
        memcpy(&sendBuffer[nextSendIndex], data, len);
        nextSendIndex += len;
    }
}

bool TCPSocket::RecvAndSend()
{
    char ctrl[CMSG_SPACE(sizeof(struct timeval))];
    auto cmsg = reinterpret_cast<struct cmsghdr *>(&ctrl);

    iovec iov;
    iov.iov_base = recvBuffer.data() + nextRecvIndex;
    iov.iov_len = TCPBufferSize - nextRecvIndex;

    msghdr msg;
    msg.msg_flags = 0;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = &inAddr;
    msg.msg_namelen = sizeof(inAddr);

    auto const readSize = recvmsg(socket, &msg, MSG_DONTWAIT);
    if (readSize > 0)
    {
        nextRecvIndex += readSize;

        Nanos kernelTime = 0;
        timeval kernelTimeValue;
        if (cmsg->cmsg_type == SCM_TIMESTAMP && cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_len == CMSG_LEN(sizeof(kernelTimeValue)))
        {
            memcpy(&kernelTimeValue, CMSG_DATA(cmsg), sizeof(kernelTimeValue));
            kernelTime = kernelTimeValue.tv_usec * NANOS_TO_MICROS + kernelTimeValue.tv_sec * NANOS_TO_SECS;
        }

        const auto userTime = GetCurrentNanos();

        logger.Log("Read socket = ", socket, "; len = ", nextRecvIndex, "; user time = ", userTime,
                   "; kernel time = ", kernelTime, "; diff = ", userTime - kernelTime, "\n");

        recvCallback(this, kernelTime);
    }

    if (nextSendIndex > 0)
    {
        int n = send(socket, sendBuffer.data(), nextSendIndex, MSG_DONTWAIT);
        logger.Log("Send socket ", socket, " returned ", n, "\n");
    }
    nextSendIndex = 0;

    return readSize > 0;
}
