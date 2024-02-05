#include "MatchingEngine.h"
#include "Check.h"
#include "Logger.h"
#include "ThreadUtils.h"
#include "exchange/matcher/MEOrderBook.h"
#include "exchange/order_server/ClientRequest.h"

namespace Exchange
{
MatchingEngine::MatchingEngine(MEClientRequestQueue *clientRequests, MEClientResponseQueue *clientResponses,
                               MEMarketUpdateQueue *marketUpdate)
    : mClientRequests(clientRequests), mClientResponses(clientResponses), mMarketUpdate(marketUpdate),
      mLogger("matching_engine.log")
{
    for (u32 i = 0; i < mOrderBook.size(); ++i)
    {
        mOrderBook[i] = new MEOrderBook(i, &mLogger, this);
    }
}
MatchingEngine::~MatchingEngine()
{
    Stop();
}

void MatchingEngine::ProcessClientRequest(MEClientRequest *request)
{
    auto orderBook = mOrderBook[request->tickerId];
    switch (request->type)
    {
    case ClientRequestType::NEW:
        orderBook->Add(request->clientId, request->orderId, request->tickerId, request->side, request->price,
                       request->quantity);
        break;
    case ClientRequestType::CANCEL:
        orderBook->Cancel(request->clientId, request->orderId, request->tickerId);
        break;
    case ClientRequestType::INVALID:
        mLogger.Log("Invalid client request received\n");
        break;
    }
}

void MatchingEngine::SendClientResponse(MEClientResponse *response)
{
    auto *nextWrite = mClientResponses->GetNextWriteTo();
    *nextWrite = std::move(*response);
    mClientResponses->UpdateWriteIndex();
}

void MatchingEngine::SendMarketUpdate(MEMarketUpdate *marketUpdate)
{
    auto *nextWrite = mMarketUpdate->GetNextWriteTo();
    *nextWrite = std::move(*marketUpdate);
    mMarketUpdate->UpdateWriteIndex();
}

void MatchingEngine::Run()
{
    while (mRunning)
    {
        auto clientRequest = mClientRequests->GetNextRead();
        if (clientRequest)
        {
            mLogger.Log("Processing request ", clientRequest->ToString(), '\n');
            ProcessClientRequest(clientRequest);
            mClientRequests->UpdateReadIndex();
        }
    }
}

void MatchingEngine::Start()
{
    mRunning = true;
    mRunningThread = CreateAndStartThread(-1, "Matching Engine", [&]() { this->Run(); });
    CHECK_FATAL(mRunningThread != nullptr, "Unable to start matching engine thread");
}

void MatchingEngine::Stop()
{
    mRunning = false;
    if (mRunningThread && mRunningThread->joinable())
        mRunningThread->join();

    mRunningThread.reset();
}

} // namespace Exchange