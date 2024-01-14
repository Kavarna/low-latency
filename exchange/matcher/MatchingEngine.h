#pragma once

#include "Logger.h"
#include "exchange/market_data/MarketUpdate.h"
#include "exchange/matcher/MEOrderBook.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include <thread>

namespace Exchange
{
class MatchingEngine
{
public:
    MatchingEngine(MEClientRequestQueue *clientRequests, MEClientResponseQueue *clientResponses,
                   MEMarketUpdateQueue *marketUpdate);

    ~MatchingEngine();

    void Start();
    void Stop();

    MatchingEngine() = delete;
    MatchingEngine(const MatchingEngine &) = delete;
    MatchingEngine(const MatchingEngine &&) = delete;
    MatchingEngine &operator=(const MatchingEngine &) = delete;
    MatchingEngine &operator=(const MatchingEngine &&) = delete;

public:
    void SendClientResponse(MEClientResponse *response);
    void SendMarketUpdate(MEMarketUpdate *marketUpdate);

private:
    void Run();

    void ProcessClientRequest(MEClientRequest *request);

private:
    OrderBookHashMap mOrderBook;
    MEClientRequestQueue *mClientRequests = nullptr;
    MEClientResponseQueue *mClientResponses = nullptr;
    MEMarketUpdateQueue *mMarketUpdate = nullptr;

    std::unique_ptr<std::thread> mRunningThread;
    volatile bool mRunning = false;

    QuickLogger mLogger;
};
} // namespace Exchange