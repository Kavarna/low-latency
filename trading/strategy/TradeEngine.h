#pragma once

#include "Logger.h"
#include "MarketUpdate.h"
#include "TimeUtils.h"
#include "Types.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/strategy/FeatureEngine.h"
#include "trading/strategy/LiquidityTaker.h"
#include "trading/strategy/MarketMaker.h"
#include "trading/strategy/MarketOrder.h"
#include "trading/strategy/MarketOrderBook.h"
#include "trading/strategy/OrderManager.h"
#include "trading/strategy/PositionKeeper.h"
#include "trading/strategy/RiskManager.h"
#include <functional>
namespace Trading
{
class TradeEngine
{
public:
    TradeEngine(ClientId clientId, AlgorithmType, TradeEngineConfigHashMap &tickerConfig,
                Exchange::MEClientRequestQueue *clientRequests, Exchange::MEClientResponseQueue *clientResponses,
                Exchange::MEMarketUpdateQueue *marketUpdates);
    ~TradeEngine();

    TradeEngine() = delete;
    TradeEngine(const TradeEngine &) = delete;
    TradeEngine(const TradeEngine &&) = delete;
    TradeEngine &operator=(const TradeEngine &) = delete;
    TradeEngine &operator=(const TradeEngine &&) = delete;

    void Start();
    void Stop();

    void SendClientRequest(Exchange::MEClientRequest *clientRequest)
    {
        mLogger.Log("Sending request: ", clientRequest->ToString(), "\n");

        auto *nextWrite = mRequestsQueue->GetNextWriteTo();
        *nextWrite = std::move(*clientRequest);
        mRequestsQueue->UpdateWriteIndex();
    }

    void OnOrderBookUpdate(TickerId tickerId, Price price, Side side, MarketOrderBook *book);
    void OnTradeUpdate(Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book);
    void OnOrderUpdate(Exchange::MEClientResponse *);

    void InitLastEventTime()
    {
        mLastEventTime = GetCurrentNanos();
    }

    i32 SilentSeconds()
    {
        return (GetCurrentNanos() - mLastEventTime) / NANOS_TO_SECS;
    }

public:
    std::function<void(TickerId, Price, Side, MarketOrderBook *)> mOnOrderBookUpdate;
    std::function<void(Exchange::MEMarketUpdate *, MarketOrderBook *)> mOnTradeUpdate;
    std::function<void(Exchange::MEClientResponse *)> mOnOrderUpdate;

private:
    void Run();

private:
    ClientId mClientId;
    MarketOrderBookHashMap mTickerOrderBook;

    Exchange::MEClientRequestQueue *mRequestsQueue;
    Exchange::MEClientResponseQueue *mResponsesQueue;
    Exchange::MEMarketUpdateQueue *mMarketUpdates;

    QuickLogger mLogger;

    volatile bool mShouldStop = false;
    std::unique_ptr<std::thread> mRunningThread;

    Nanos mLastEventTime = 0;

    FeatureEngine mFeatureEngine;

    PositionKeeper mPositionKeeper;

    OrderManager mOrderManager;

    RiskManager mRiskManager;

    MarketMaker *mMarketMakerAlgorithm;
    LiquidityTaker *mLiquidityTakerAlgorithm;
};
} // namespace Trading