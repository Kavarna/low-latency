#pragma once

#include "Logger.h"
#include "MCastSocket.h"
#include "MarketUpdate.h"
#include "Types.h"
#include "exchange/market_data/SnapshotSynthesizer.h"
namespace Exchange
{
class MarketDataPublisher
{
public:
    MarketDataPublisher(MEMarketUpdateQueue *marketUpdateQueue, std::string const &iface, std::string const &snapshotIp,
                        i32 snapshotPort, std::string const &incrementalIp, i32 incrementalPort);
    ~MarketDataPublisher();

    MarketDataPublisher() = delete;
    MarketDataPublisher(const MarketDataPublisher &) = delete;
    MarketDataPublisher(const MarketDataPublisher &&) = delete;
    MarketDataPublisher &operator=(const MarketDataPublisher &) = delete;
    MarketDataPublisher &operator=(const MarketDataPublisher &&) = delete;

    void Start();
    void Stop();

private:
    void Run();

private:
    u64 mNextSequenceNumber = 1;
    QuickLogger mLogger;

    MCastSocket mMulticastSocket;

    MEMarketUpdateQueue *mMarketUpdateQueue;

    MPDMarketUpdateQueue mSnapshotQueue;

    std::unique_ptr<std::thread> mRunningThread;

    SnapshotSynthesizer *mSnapshotSynthesizer = nullptr;

    volatile bool mShouldStop = false;
};
} // namespace Exchange