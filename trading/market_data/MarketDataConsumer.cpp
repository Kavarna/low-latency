#include "MarketDataConsumer.h"
#include "Check.h"
#include "MCastSocket.h"
#include "MarketUpdate.h"
#include "ThreadUtils.h"
#include <cstring>

namespace Trading
{
MarketDataConsumer::MarketDataConsumer(ClientId clientId, Exchange::MEMarketUpdateQueue *marketUpdates,
                                       const std::string &iface, const std::string &snapshotIp, i32 snapshotPort,
                                       const std::string &incrementalIp, i32 incrementalPort)
    : mLogger("trading_market_data_consumer_" + std::to_string(clientId) + ".log"), mMarketUpdates(marketUpdates),
      mIncrementalSocket(&mLogger), mSnapshotSocket(&mLogger), mIFace(iface), mSnapshotIp(snapshotIp),
      mSnapshotPort(snapshotPort)
{
    auto recvCallback = [this](MCastSocket *socket) { RecvCallback(socket); };

    mIncrementalSocket.recvCallback = recvCallback;
    CHECK_FATAL(mIncrementalSocket.Init(incrementalIp, iface, incrementalPort, true),
                "Couldn't open incremental socket")

    CHECK_FATAL(mIncrementalSocket.Join(incrementalIp), "Couldn't join with the incremental socket")

    mSnapshotSocket.recvCallback = recvCallback;
}

MarketDataConsumer::~MarketDataConsumer()
{
    Stop();
}

void MarketDataConsumer::StartSnaphotSync()
{
    mSnapshotQueuedMessages.clear();
    mIncrementalQueuedMessages.clear();

    CHECK_FATAL(mSnapshotSocket.Init(mSnapshotIp, mIFace, mSnapshotPort, true), "Cannot create snapshot socket");

    CHECK_FATAL(mSnapshotSocket.Join(mSnapshotIp), "Cannot join with snapshot socket");
}

void MarketDataConsumer::CheckSnapshotSync()
{
    if (mSnapshotQueuedMessages.empty())
    {
        return;
    }

    const auto &firstSnapshotMsg = mSnapshotQueuedMessages.begin()->second;
    if (firstSnapshotMsg.type != Exchange::MarketUpdateType::SNAPSHOT_START)
    {
        mLogger.Log("First market update in the snapshot is not SNAPSHOT START. Trying again\n");
        mSnapshotQueuedMessages.clear();
        return;
    }

    std::vector<Exchange::MEMarketUpdate> finalEvents;

    bool isSnapshotComplete = false;
    u64 nextSequenceNumber = 1;
    for (auto &snapshotIter : mSnapshotQueuedMessages)
    {
        if (snapshotIter.first != nextSequenceNumber)
        {
            isSnapshotComplete = true;
            mLogger.Log("Found gap in the snapshot queue messages. Expected ", nextSequenceNumber, " but found ",
                        snapshotIter.first, "\n");
            break;
        }

        if (snapshotIter.second.type != Exchange::MarketUpdateType::SNAPSHOT_START &&
            snapshotIter.second.type != Exchange::MarketUpdateType::SNAPSHOT_END)
        {
            finalEvents.push_back(snapshotIter.second);
        }

        ++nextSequenceNumber;
    }

    auto &lastSnapshotMessage = mSnapshotQueuedMessages.rbegin()->second;
    if (lastSnapshotMessage.type != Exchange::MarketUpdateType::SNAPSHOT_END)
    {
        mLogger.Log("Did not find the snapshot end market update\n");
        isSnapshotComplete = false;
    }

    if (!isSnapshotComplete)
    {
        mLogger.Log("Stopping current snapshot sync as the snapshot is not complete\n");
        mSnapshotQueuedMessages.clear();
        return;
    }

    mLogger.Log("The snapshot seems complete, so will start recovery\n");

    auto haveCompleteIncremental = true;
    nextSequenceNumber = lastSnapshotMessage.orderId + 1;
    for (auto incrementalIt : mIncrementalQueuedMessages)
    {
        if (incrementalIt.first < mNextExpectedSequenceNumber)
        {
            continue;
        }

        if (incrementalIt.first != nextSequenceNumber)
        {
            mLogger.Log("Found gap in incremental stream. Expected ", nextSequenceNumber, " but found ",
                        incrementalIt.first, "\n");
            haveCompleteIncremental = false;
            break;
        }

        if (incrementalIt.second.type != Exchange::MarketUpdateType::SNAPSHOT_START &&
            incrementalIt.second.type != Exchange::MarketUpdateType::SNAPSHOT_END)
        {
            finalEvents.push_back(incrementalIt.second);
        }
        nextSequenceNumber++;
    }

    if (!haveCompleteIncremental)
    {
        mLogger.Log("The incremental stream is not complete. Have to retry ;(\n");
        mSnapshotQueuedMessages.clear();
        mIncrementalQueuedMessages.clear();
        return;
    }

    for (auto &itr : finalEvents)
    {
        auto nextWrite = mMarketUpdates->GetNextWriteTo();
        *nextWrite = itr;
        mMarketUpdates->UpdateWriteIndex();
    }

    mInRecovery = false;
    mSnapshotQueuedMessages.clear();
    mIncrementalQueuedMessages.clear();
    mNextExpectedSequenceNumber = nextSequenceNumber;

    mLogger.Log("The recovery is complete\n");

    mSnapshotSocket.Leave();
}

void MarketDataConsumer::QueueMessage(bool isSnapshot, Exchange::MPDMarketUpdate &marketUpdate)
{
    if (isSnapshot)
    {
        if (mSnapshotQueuedMessages.find(marketUpdate.sequenceNumber) != mSnapshotQueuedMessages.end())
        {
            mLogger.Log("Received a snapshot market update multipel times: ", marketUpdate.ToString(), "\n");
            mSnapshotQueuedMessages.clear();
        }
        mSnapshotQueuedMessages[marketUpdate.sequenceNumber] = marketUpdate.marketUpdate;
    }
    else
    {
        mIncrementalQueuedMessages[marketUpdate.sequenceNumber] = marketUpdate.marketUpdate;
    }

    mLogger.Log("Snapshot size: ", mSnapshotQueuedMessages.size(),
                "; Incremental size: ", mIncrementalQueuedMessages.size(), "\n");
    CheckSnapshotSync();
}

void MarketDataConsumer::RecvCallback(MCastSocket *socket)
{
    bool isSnapshot = socket->socket == mSnapshotSocket.socket;
    if (isSnapshot && !mInRecovery) [[unlikely]]
    {
        socket->nextRecvDataIndex = 0;
        mLogger.Log("Received data from the snapshot socket while not in recovery mode! \n");
        return;
    }

    if (socket->nextRecvDataIndex >= sizeof(Exchange::MPDMarketUpdate))
    {
        u32 i = 0;
        for (; i + sizeof(Exchange::MPDMarketUpdate) < socket->nextRecvDataIndex;
             i += sizeof(Exchange::MPDMarketUpdate))
        {
            auto marketUpdate = reinterpret_cast<Exchange::MPDMarketUpdate *>(socket->inboundData.data() + i);

            mLogger.Log("Received market update: ", marketUpdate->ToString(), " on ",
                        isSnapshot ? "snapshot" : "incremental", " socket\n");

            const bool alreadyInRecovery = mInRecovery;
            mInRecovery = alreadyInRecovery || marketUpdate->sequenceNumber != mNextExpectedSequenceNumber;

            if (mInRecovery) [[unlikely]]
            {
                if (!alreadyInRecovery) [[unlikely]]
                {
                    mLogger.Log("Found some packet drops ;( Sequence number expected ", mNextExpectedSequenceNumber,
                                " but found ", marketUpdate->sequenceNumber, "\n");
                    StartSnaphotSync();
                }

                QueueMessage(isSnapshot, *marketUpdate);
            }
            else if (!isSnapshot)
            {
                ++mNextExpectedSequenceNumber;

                auto nextWrite = mMarketUpdates->GetNextWriteTo();
                *nextWrite = std::move(marketUpdate->marketUpdate);
                mMarketUpdates->UpdateWriteIndex();
            }
        }

        memcpy(socket->inboundData.data(), socket->inboundData.data() + i, socket->nextRecvDataIndex - i);
        socket->nextRecvDataIndex -= i;
    }
}

void MarketDataConsumer::Start()
{
    mShouldStop = false;
    mRunningThread = CreateAndStartThread(-1, "Trading/MarketDataConsumer", [this]() { Run(); });
    CHECK_FATAL(mRunningThread != nullptr, "Unable to start market data consumer");
}

void MarketDataConsumer::Run()
{
    while (!mShouldStop)
    {
        mIncrementalSocket.RecvAndSend();
        mSnapshotSocket.RecvAndSend();
    }
}

void MarketDataConsumer::Stop()
{
    mShouldStop = true;
    mRunningThread->join();
}

} // namespace Trading
