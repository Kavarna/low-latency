#pragma once

#include "Logger.h"
#include "MCastSocket.h"
#include "MarketUpdate.h"
#include "Types.h"

#include <map>

namespace Trading
{

class MarketDataConsumer
{
public:
    MarketDataConsumer(ClientId clientId, Exchange::MEMarketUpdateQueue *marketUpdates, const std::string &iface,
                       const std::string &snapshotIp, i32 snapshotPort, const std::string &incrementalIp,
                       i32 incrementalPort);
    ~MarketDataConsumer();

    MarketDataConsumer() = delete;
    MarketDataConsumer(const MarketDataConsumer &) = delete;
    MarketDataConsumer(const MarketDataConsumer &&) = delete;
    MarketDataConsumer &operator=(const MarketDataConsumer &) = delete;
    MarketDataConsumer &operator=(const MarketDataConsumer &&) = delete;

    void Start();
    void Stop();

private:
    void RecvCallback(MCastSocket *socket);

    void Run();

    void StartSnaphotSync();

    void QueueMessage(bool isSnapshot, Exchange::MPDMarketUpdate &marketUpdate);

    void CheckSnapshotSync();

private:
    QuickLogger mLogger;

    Exchange::MEMarketUpdateQueue *mMarketUpdates;

    u64 mNextExpectedSequenceNumber = 1;

    MCastSocket mIncrementalSocket, mSnapshotSocket;

    std::string mIFace;

    bool mInRecovery = false;

    std::string mSnapshotIp;
    i32 mSnapshotPort;

    volatile bool mShouldStop = false;

    std::unique_ptr<std::thread> mRunningThread;

    using QueuedMarketUpdates = std::map<u64, Exchange::MEMarketUpdate>;
    QueuedMarketUpdates mSnapshotQueuedMessages, mIncrementalQueuedMessages;
};

} // namespace Trading
