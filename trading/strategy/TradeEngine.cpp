#include "TradeEngine.h"
#include "MarketUpdate.h"
#include "TimeUtils.h"
#include "Types.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/strategy/LiquidityTaker.h"
#include "trading/strategy/MarketMaker.h"
#include "trading/strategy/MarketOrderBook.h"
#include "trading/strategy/OrderManager.h"
#include "trading/strategy/RiskManager.h"
#include <chrono>
#include <ratio>
#include <thread>

void DefaultAlgorithmOnOrderBookUpdate(TickerId, Price, Side, Trading::MarketOrderBook *)
{
}

void DefaultAlgorithmOnTradeUpdate(Exchange::MEMarketUpdate *, Trading::MarketOrderBook *)
{
}

void DefaultAlgorithmOnOrderUpdate(Exchange::MEClientResponse *response)
{
}

namespace Trading
{
TradeEngine::TradeEngine(ClientId clientId, AlgorithmType algorithmType, TradeEngineConfigHashMap &tickerConfig,
                         Exchange::MEClientRequestQueue *clientRequests,
                         Exchange::MEClientResponseQueue *clientResponses, Exchange::MEMarketUpdateQueue *marketUpdates)
    : mClientId(clientId), mRequestsQueue(clientRequests), mResponsesQueue(clientResponses),
      mMarketUpdates(marketUpdates), mLogger("trading_engine_" + std::to_string(clientId) + ".log"),
      mFeatureEngine(&mLogger), mPositionKeeper(&mLogger), mOrderManager(&mLogger, this, mRiskManager),
      mRiskManager(&mPositionKeeper, tickerConfig)
{
    for (u32 i = 0; i < mTickerOrderBook.size(); ++i)
    {
        mTickerOrderBook[i] = new MarketOrderBook(i, &mLogger);
        mTickerOrderBook[i]->SetTradeEngine(this);
    }

    mOnTradeUpdate = DefaultAlgorithmOnTradeUpdate;
    mOnOrderBookUpdate = DefaultAlgorithmOnOrderBookUpdate;
    mOnOrderUpdate = DefaultAlgorithmOnOrderUpdate;

    if (algorithmType == AlgorithmType::MAKER)
    {
        mMarketMakerAlgorithm = new MarketMaker(&mLogger, this, &mFeatureEngine, &mOrderManager, tickerConfig);
    }
    else if (algorithmType == AlgorithmType::TAKER)
    {
        mLiquidityTakerAlgorithm = new LiquidityTaker(&mLogger, this, &mFeatureEngine, &mOrderManager, tickerConfig);
    }
}

TradeEngine::~TradeEngine()
{
    delete mMarketMakerAlgorithm;
    mMarketMakerAlgorithm = nullptr;

    delete mLiquidityTakerAlgorithm;
    mLiquidityTakerAlgorithm = nullptr;

    for (u32 i = 0; i < mTickerOrderBook.size(); ++i)
    {
        delete mTickerOrderBook[i];
        mTickerOrderBook[i] = nullptr;
    }

    Stop();
}

void TradeEngine::Start()
{
    mShouldStop = false;
    mRunningThread = CreateAndStartThread(-1, "Trading/TradeEngine", [this]() { Run(); });
    CHECK_FATAL(mRunningThread != nullptr, "Unable to start the market data publisher");
}

void TradeEngine::OnOrderBookUpdate(TickerId tickerId, Price price, Side side, MarketOrderBook *book)
{
    auto &bbo = book->GetBestBidOffer();

    mPositionKeeper.UpdateBestBidOffer(tickerId, &bbo);

    mFeatureEngine.OnOrderBookUpdate(tickerId, price, side, book);

    mOnOrderBookUpdate(tickerId, price, side, book);
}

void TradeEngine::OnTradeUpdate(Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book)
{
    mFeatureEngine.OnTradeUpdate(marketUpdate, book);

    mOnTradeUpdate(marketUpdate, book);
}

void TradeEngine::OnOrderUpdate(Exchange::MEClientResponse *clientResponse)
{
    if (clientResponse->type == Exchange::ClientResponseType::FILLED)
    {
        mPositionKeeper.AddFill(clientResponse);
    }

    mOnOrderUpdate(clientResponse);
}

void TradeEngine::Run()
{
    while (!mShouldStop)
    {
        for (auto clientResponse = mResponsesQueue->GetNextRead(); clientResponse != nullptr;
             mResponsesQueue->GetNextRead())
        {
            OnOrderUpdate(clientResponse);

            mResponsesQueue->UpdateReadIndex();

            mLastEventTime = GetCurrentNanos();
        }

        for (auto marketUpdate = mMarketUpdates->GetNextRead(); marketUpdate != nullptr;
             marketUpdate = mMarketUpdates->GetNextRead())
        {
            mTickerOrderBook[marketUpdate->tickerId]->OnMarketUpdate(marketUpdate);

            mMarketUpdates->UpdateReadIndex();

            mLastEventTime = GetCurrentNanos();
        }
    }
}

void TradeEngine::Stop()
{
    while (mResponsesQueue->GetSize() != 0 || mMarketUpdates->GetSize() != 0)
    {
        mLogger.Log("Stop had been requested but there are still requests to process... Waiting...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    mShouldStop = true;
    mRunningThread->join();

    mLogger.Log("Positions at end: ", mPositionKeeper.ToString(), "\n");
}

} // namespace Trading
