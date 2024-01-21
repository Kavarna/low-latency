#pragma once

#include "Limits.h"
#include "Logger.h"
#include "MCastSocket.h"
#include "MemoryPool.h"
#include "TimeUtils.h"
#include "exchange/market_data/MarketUpdate.h"
#include <cstddef>
#include <thread>

namespace Exchange
{
class SnapshotSynthesizer
{
public:
    SnapshotSynthesizer(MPDMarketUpdateQueue *marketUpdates, std::string const &iface, std::string const &snapshotIp,
                        i32 snapshotPort);
    ~SnapshotSynthesizer();

    SnapshotSynthesizer() = delete;
    SnapshotSynthesizer(const SnapshotSynthesizer &) = delete;
    SnapshotSynthesizer(const SnapshotSynthesizer &&) = delete;
    SnapshotSynthesizer &operator=(const SnapshotSynthesizer &) = delete;
    SnapshotSynthesizer &operator=(const SnapshotSynthesizer &&) = delete;

    void Start();
    void Stop();

private:
    void Run();

    void AddToSnapshot(MPDMarketUpdate *marketUpdate);

    void PublishSnapshot();

private:
    MPDMarketUpdateQueue *mSnapshotQueue = nullptr;

    QuickLogger mLogger;

    volatile bool mShouldStop = true;

    MCastSocket mSnapshotSocket;

    std::array<std::array<MEMarketUpdate *, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> mOrdersByTickers;

    u64 mLastSequenceIncrementalNumber = 0;
    Nanos mLastSnapshotTime = 0;

    MemoryPool<MEMarketUpdate> mMarketUpdatesPool;

    std::unique_ptr<std::thread> mRunningThread;
};
} // namespace Exchange