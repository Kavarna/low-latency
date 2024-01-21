#include "MarketDataPublisher.h"
#include "Check.h"
#include "MCastSocket.h"
#include "ThreadUtils.h"
#include "exchange/market_data/SnapshotSynthesizer.h"

namespace Exchange
{
MarketDataPublisher::MarketDataPublisher(MEMarketUpdateQueue *marketUpdateQueue, std::string const &iface,
                                         std::string const &snapshotIp, i32 snapshotPort,
                                         std::string const &incrementalIp, i32 incrementalPort)
    : mLogger("exchange_market_data_publisher.log"), mMulticastSocket(&mLogger), mMarketUpdateQueue(marketUpdateQueue),
      mSnapshotQueue(ME_MAX_MARKET_UPDATES), mShouldStop(true)
{
    CHECK_FATAL(mMulticastSocket.Init(incrementalIp, iface, incrementalPort, false),
                "Unable to initialize multicast socket");

    mSnapshotSynthesizer = new SnapshotSynthesizer(&mSnapshotQueue, iface, snapshotIp, snapshotPort);
}

MarketDataPublisher::~MarketDataPublisher()
{
    Stop();
}

void MarketDataPublisher::Start()
{
    mShouldStop = false;
    mRunningThread = CreateAndStartThread(-1, "Exchange/MarketDataPublisher", [this]() { Run(); });
    CHECK_FATAL(mRunningThread != nullptr, "Unable to start the market data publisher");

    mSnapshotSynthesizer->Start();
}

void MarketDataPublisher::Run()
{
    while (!mShouldStop)
    {
        while (mMarketUpdateQueue->GetSize() != 0)
        {
            auto marketUpdate = mMarketUpdateQueue->GetNextRead();

            mLogger.Log("Sending market update: ", marketUpdate->ToString(), "\n");

            /* Send the market update */
            mMulticastSocket.Send(&mNextSequenceNumber, sizeof(mNextSequenceNumber));
            mMulticastSocket.Send(&marketUpdate, sizeof(MEMarketUpdate));

            /* Update read index for the market update queue */
            mMarketUpdateQueue->UpdateReadIndex();

            /* Also save this to the snapshot queue */
            auto nextWrite = mSnapshotQueue.GetNextWriteTo();
            nextWrite->sequenceNumber = mNextSequenceNumber;
            nextWrite->marketUpdate = *marketUpdate;

            /* Update the write index */
            mSnapshotQueue.UpdateWriteIndex();

            mNextSequenceNumber++;
        }

        mMulticastSocket.RecvAndSend();
    }
}

void MarketDataPublisher::Stop()
{
    mShouldStop = true;
    mRunningThread->join();

    mSnapshotSynthesizer->Stop();
}
} // namespace Exchange
