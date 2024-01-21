#pragma once

#include "Limits.h"
#include "Logger.h"
#include "TCPSocket.h"
#include "TimeUtils.h"
#include "common/TCPServer.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include "exchange/order_server/FIFOSequencer.h"
#include <array>
#include <string>

namespace Exchange
{

class OrderServer
{
public:
    OrderServer(MEClientRequestQueue *clientRequests, MEClientResponseQueue *clientResponses, std::string const &iface,
                i32 port);
    ~OrderServer();

    OrderServer() = delete;
    OrderServer(const OrderServer &) = delete;
    OrderServer(const OrderServer &&) = delete;
    OrderServer &operator=(const OrderServer &) = delete;
    OrderServer &operator=(const OrderServer &&) = delete;

    void Start();
    void Stop();

private:
    void RecvCallback(TCPSocket *socket, Nanos rxTime);
    void RecvFinishCallback();

    void Run();

private:
    std::string mIFace;
    i32 mPort = 0;

    MEClientResponseQueue *mClientResponses = nullptr;

    std::array<TCPSocket *, ME_MAX_NUM_CLIENTS> mClientIdToSocket;

    std::array<u64, ME_MAX_NUM_CLIENTS> mClientIdToNextResponseSequenceNumber;
    std::array<u64, ME_MAX_NUM_CLIENTS> mClientIdToNextRequestSequenceNumber;

    volatile bool mShouldStop = false;

    std::unique_ptr<std::thread> mRunningThread;

    QuickLogger mLogger;

    TCPServer mTCPServer;
    FIFOSequencer mSequencer;
};

} // namespace Exchange