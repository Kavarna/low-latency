#pragma once

#include "Check.h"
#include "Limits.h"
#include "Logger.h"
#include "TimeUtils.h"
#include "exchange/order_server/ClientRequest.h"

namespace Exchange
{
class FIFOSequencer
{
public:
    FIFOSequencer(MEClientRequestQueue *clientRequests, QuickLogger *logger)
        : mLogger(logger), mClientRequests(clientRequests)
    {
    }
    ~FIFOSequencer()
    {
    }

    FIFOSequencer() = delete;
    FIFOSequencer(const FIFOSequencer &) = delete;
    FIFOSequencer(const FIFOSequencer &&) = delete;
    FIFOSequencer &operator=(const FIFOSequencer &) = delete;
    FIFOSequencer &operator=(const FIFOSequencer &&) = delete;

    void AddClientRequest(Nanos recvTime, MEClientRequest &request)
    {
        CHECK_FATAL(mPendingSize < ME_MAX_PENDING_REQUESTS, "Too many pending requests");
        mPendingRequests[mPendingSize++] = {recvTime, request};
    }

    void SequenceAndPublish()
    {
        if (mPendingSize == 0) [[unlikely]]
        {
            return;
        }

        mLogger->Log("Reordering ", mPendingSize, " requests\n");

        std::sort(mPendingRequests.begin(), mPendingRequests.begin() + mPendingSize);

        for (u32 i = 0; i < mPendingSize; ++i)
        {
            auto clientRequest = mPendingRequests[i];

            mLogger->Log("Writing request ", clientRequest.clientRequest.ToString(),
                         " to  FIFO (recv time = ", clientRequest.recvTime, ")\n");
            auto nextWrite = mClientRequests->GetNextWriteTo();
            *nextWrite = std::move(clientRequest.clientRequest);
            mClientRequests->UpdateWriteIndex();
        }

        mPendingSize = 0;
    }

private:
    QuickLogger *mLogger;
    MEClientRequestQueue *mClientRequests;

    struct RecvTimeClientRequest
    {
        Nanos recvTime;
        MEClientRequest clientRequest;

        bool operator<(const RecvTimeClientRequest &rhs) const
        {
            return recvTime < rhs.recvTime;
        }
    };

    std::array<RecvTimeClientRequest, ME_MAX_PENDING_REQUESTS> mPendingRequests;
    u32 mPendingSize;
};
} // namespace Exchange