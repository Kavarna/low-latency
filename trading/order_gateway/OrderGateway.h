#pragma once

#include "Logger.h"
#include "TCPSocket.h"
#include "ThreadUtils.h"
#include "TimeUtils.h"
#include "Types.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"

namespace Trading
{
class OrderGateway
{
public:
    OrderGateway(ClientId clientId, Exchange::MEClientRequestQueue *clientRequests,
                 Exchange::MEClientResponseQueue *clientResponses, std::string const &ip, std::string const &iface,
                 i32 port);
    ~OrderGateway();

    void Start()
    {
        mShouldStop = false;

        CHECK_FATAL(mSocket.Connect(mIp, mIFace, mPort, false) >= 0, "Unable to connect to ip: ", mIp,
                    " on iface: ", mIFace, "; port: ", mPort);

        mThread = CreateAndStartThread(-1, "Trading/OrderGateway", [this]() { Run(); });
        CHECK_FATAL(mThread != nullptr, "Unable to start market data consumer");
    }

    void Stop()
    {
        mShouldStop = true;

        mThread->join();
    }

    OrderGateway() = delete;
    OrderGateway(const OrderGateway &) = delete;
    OrderGateway(const OrderGateway &&) = delete;
    OrderGateway &operator=(const OrderGateway &) = delete;
    OrderGateway &operator=(const OrderGateway &&) = delete;

private:
    void Run();

    void RecvCallback(TCPSocket *socket, Nanos rxTime);

private:
    ClientId mClientId;

    std::string mIp;
    std::string mIFace;
    i32 mPort;

    QuickLogger mLogger;

    Exchange::MEClientRequestQueue *mRequests;
    Exchange::MEClientResponseQueue *mResponses;

    volatile bool mShouldStop;

    u64 mNextOutgoingSequenceNumber = 1;
    u64 mNextExpectedSequenceNumber = 1;

    TCPSocket mSocket;

    std::unique_ptr<std::thread> mThread;
};
} // namespace Trading