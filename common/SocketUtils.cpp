#include "SocketUtils.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

std::string GetIFaceIP(std::string const &iface)
{
    char buf[NI_MAXHOST] = {};
    ifaddrs *ifaddr = nullptr;

    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface.compare(ifa->ifa_name) == 0)
            {
                getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST);
                break;
            }
        }

        freeifaddrs(ifaddr);
    }
    return buf;
}

bool SetNonBlocking(Socket fd)
{
    const auto flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        return false;
    }
    if (flags & O_NONBLOCK)
    {
        return true;
    }

    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

bool SetNoDelay(Socket fd)
{
    i32 one = 1;
    return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) != -1);
}

bool SetSOTimestamp(Socket fd)
{
    i32 one = 1;
    return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &one, sizeof(one)) != -1);
}

bool SetTTL(Socket fd, i32 ttl)
{
    return (setsockopt(fd, IPPROTO_TCP, IP_TTL, &ttl, sizeof(ttl)) != -1);
}

bool SetMcastTTL(Socket fd, i32 ttl)
{
    return (setsockopt(fd, IPPROTO_TCP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) != -1);
}

bool Join(Socket fd, std::string const &ip)
{
    ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(ip.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 1);
}

Socket CreateSocket(QuickLogger &logger, std::string const &t_ip, std::string const &iface, i32 port, bool isUdp,
                    bool isListening, bool needsSOTimestamp)
{
    const auto ip = t_ip.empty() ? GetIFaceIP(iface) : t_ip;
    logger.Log("Creating socket with params: t_ip = \"", t_ip, "\"; iface = \"", iface, "\"; port = ", port,
               "; isUdp = ", isUdp ? "TRUE" : "FALSE", "; isListening = ", isListening ? "TRUE" : "FALSE",
               "; needsSOTimestamp = ", needsSOTimestamp ? "TRUE" : "FALSE", '\n');

    i32 flags = (isListening ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
    addrinfo hints = {};
    hints.ai_flags = flags;
    hints.ai_family = AF_INET;
    hints.ai_socktype = isUdp ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = isUdp ? IPPROTO_UDP : IPPROTO_TCP;

    addrinfo *result = {};
    auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (rc)
    {
        logger.Log("getaddrinfo() failed. error: ", gai_strerror(rc), " errno: ", strerror(errno), '\n');
        return -1;
    }

    Socket fdSocket = -1;
    i32 one = 1;
    for (addrinfo *rp = result; rp; rp = rp->ai_next)
    {
        fdSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fdSocket == -1)
        {
            logger.Log("socket() failed. errno: ", strerror(errno), '\n');
            return -1;
        }

        if (!isUdp && !SetNoDelay(fdSocket))
        {
            logger.Log("SetNoDelay() failed. errno: ", strerror(errno), '\n');
            return -1;
        }

        if (!isListening)
        {
            if (connect(fdSocket, rp->ai_addr, rp->ai_addrlen) == -1)
            {
                logger.Log("connect() failed. errno: ", strerror(errno), '\n');
                return -1;
            }
        }
        if (isListening)
        {
            if (setsockopt(fdSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != 0)
            {
                logger.Log("setsockopt() failed. errno: ", strerror(errno), '\n');
                return -1;
            }

            sockaddr_in addr = {};
            addr.sin_port = htons(port);
            addr.sin_family = AF_INET;
            addr.sin_addr = {htonl(INADDR_ANY)};
            if (bind(fdSocket, isUdp ? reinterpret_cast<sockaddr *>(&addr) : rp->ai_addr, sizeof(sockaddr)) != 0)
            {
                logger.Log("bind() failed. errno: ", strerror(errno), '\n');
                return -1;
            }
        }

        if (isListening && !isUdp)
        {
            if (listen(fdSocket, MAX_TCP_SERVER_BACKLOG) != 0)
            {
                logger.Log("listen() failed. errno: ", strerror(errno), '\n');
                return -1;
            }
        }

        if (!SetNonBlocking(fdSocket))
        {
            logger.Log("SetNonBlocking() failed. errno: ", strerror(errno), '\n');
            return -1;
        }

        if (needsSOTimestamp)
        {
            if (!SetSOTimestamp(fdSocket))
            {
                logger.Log("SetSOTimestamp() failed. errno: ", strerror(errno), '\n');
                return -1;
            }
        }
    }

    logger.Log("Successfully created socket ", fdSocket, '\n');

    return fdSocket;
}
