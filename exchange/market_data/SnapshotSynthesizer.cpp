#include "SnapshotSynthesizer.h"
#include "Check.h"
#include "Limits.h"
#include "MCastSocket.h"
#include "ThreadUtils.h"
#include "TimeUtils.h"
#include "exchange/market_data/MarketUpdate.h"

namespace Exchange
{
SnapshotSynthesizer::SnapshotSynthesizer(MPDMarketUpdateQueue *marketUpdates, std::string const &iface,
                                         std::string const &snapshotIp, i32 snapshotPort)
    : mSnapshotQueue(marketUpdates), mLogger("exchange_snapshot_synthesizer.log"), mSnapshotSocket(&mLogger),
      mMarketUpdatesPool(ME_MAX_ORDER_IDS)
{
    CHECK_FATAL(mSnapshotSocket.Init(snapshotIp, iface, snapshotPort, false), "Couldn't initialize snapshot socket");
    for (auto &orders : mOrdersByTickers)
    {
        orders.fill(nullptr);
    }
}

SnapshotSynthesizer::~SnapshotSynthesizer()
{
    Stop();
}

void SnapshotSynthesizer::AddToSnapshot(MPDMarketUpdate *marketUpdateMPD)
{
    const auto marketUpdate = marketUpdateMPD->marketUpdate;
    auto orders = mOrdersByTickers[marketUpdate.tickerId];
    switch (marketUpdate.type)
    {
    case MarketUpdateType::ADD: {
        auto order = orders[marketUpdate.orderId];
        CHECK_FATAL(order == nullptr, "Received: ", marketUpdate.ToString(),
                    " but order already exists: ", order->ToString(), "\n");
        orders[marketUpdate.orderId] = mMarketUpdatesPool.Allocate(marketUpdate);

        break;
    }
    case MarketUpdateType::MODIFY: {
        auto order = orders[marketUpdate.orderId];
        CHECK_FATAL(order != nullptr, "Received: ", marketUpdate.ToString(), " but order does not exist\n");
        CHECK_FATAL(order->orderId == marketUpdate.orderId, "Expecting existing order to match new one");
        CHECK_FATAL(order->side == marketUpdate.side, "Expecting existing order to match new one");

        order->quantity = marketUpdate.quantity;
        order->price = marketUpdate.price;
        break;
    }
    case MarketUpdateType::CANCEL: {
        auto order = orders[marketUpdate.orderId];
        CHECK_FATAL(order != nullptr, "Received: ", marketUpdate.ToString(), " but order does not exist\n");
        CHECK_FATAL(order->orderId == marketUpdate.orderId, "Expecting existing order to match new one");
        CHECK_FATAL(order->side == marketUpdate.side, "Expecting existing order to match new one");

        mMarketUpdatesPool.Deallocate(order);
        orders[marketUpdate.orderId] = nullptr;

        break;
    }
    case MarketUpdateType::SNAPSHOT_START:
    case MarketUpdateType::CLEAR:
    case MarketUpdateType::SNAPSHOT_END:
    case MarketUpdateType::TRADE:
    case MarketUpdateType::INVALID:
        break;
    }

    CHECK_FATAL(marketUpdateMPD->sequenceNumber == mLastSequenceIncrementalNumber + 1,
                "Expected incremental sequence nums");
    mLastSequenceIncrementalNumber = marketUpdateMPD->sequenceNumber;
}

void SnapshotSynthesizer::Start()
{
    mShouldStop = false;
    mRunningThread = CreateAndStartThread(-1, "Exchange/SnapshotSyntesizer", [this] { Run(); });
    CHECK_FATAL(mRunningThread != nullptr, "Couldn't start snapshot syntesizer");
}

void SnapshotSynthesizer::Stop()
{
    mShouldStop = true;
    mRunningThread->join();
}

void SnapshotSynthesizer::Run()
{
    while (!mShouldStop)
    {
        while (mSnapshotQueue->GetSize() != 0)
        {
            auto marketUpdate = mSnapshotQueue->GetNextRead();
            mLogger.Log("Processing: ", marketUpdate->ToString(), "\n");

            AddToSnapshot(marketUpdate);

            mSnapshotQueue->UpdateReadIndex();
        }

        if (GetCurrentNanos() - mLastSnapshotTime > 60 * NANOS_TO_SECS)
        {
            mLastSnapshotTime = GetCurrentNanos();
            PublishSnapshot();
        }
    }
}

void SnapshotSynthesizer::PublishSnapshot()
{
    mLogger.Log("~~~~~~~~~~~~~~~~~~ START PUBLISHING SNAPSHOT ! ! ! ~~~~~~~~~~~~~~~~~~\n");
    u32 snapshotSize = 0;
    MPDMarketUpdate startMaketUpdate{};
    {
        startMaketUpdate.sequenceNumber = snapshotSize++;
        startMaketUpdate.marketUpdate.type = MarketUpdateType::SNAPSHOT_START;
    }
    mSnapshotSocket.Send(&startMaketUpdate, sizeof(MPDMarketUpdate));
    mLogger.Log("Sending start update: ", startMaketUpdate.ToString(), "\n");

    for (size_t tickerId = 0; tickerId < mOrdersByTickers.size(); ++tickerId)
    {
        const auto &orders = mOrdersByTickers[tickerId];

        MPDMarketUpdate clearMarketUpdate{};
        {
            clearMarketUpdate.marketUpdate.type = MarketUpdateType::CLEAR;
            clearMarketUpdate.marketUpdate.tickerId = tickerId;
            clearMarketUpdate.sequenceNumber = snapshotSize++;
        }

        mLogger.Log("Sending clear update: ", clearMarketUpdate.ToString(), "\n");

        for (const auto order : orders)
        {
            if (order)
            {
                MPDMarketUpdate marketUpdate{};
                {
                    marketUpdate.sequenceNumber = snapshotSize++;
                    marketUpdate.marketUpdate = *order;
                }

                mLogger.Log("Sending new update: ", clearMarketUpdate.ToString(), "\n");

                mSnapshotSocket.Send(&marketUpdate, sizeof(marketUpdate));
                mSnapshotSocket.RecvAndSend();
            }
        }
    }

    MPDMarketUpdate stopMarketUpdate{};
    {
        startMaketUpdate.sequenceNumber = snapshotSize++;
        startMaketUpdate.marketUpdate.type = MarketUpdateType::SNAPSHOT_END;
    }

    mSnapshotSocket.Send(&stopMarketUpdate, sizeof(stopMarketUpdate));
    mSnapshotSocket.RecvAndSend();
    mLogger.Log("Sending stop update: ", stopMarketUpdate.ToString(), "\n");

    mSnapshotSocket.RecvAndSend();

    mLogger.Log("~~~~~~~~~~~~~~~~~~ STOP  PUBLISHING SNAPSHOT ! ! ! ~~~~~~~~~~~~~~~~~~\n");

    mLogger.Log("Published in total ", snapshotSize, " orders\n");
}

} // namespace Exchange