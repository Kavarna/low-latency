#include "MarketMaker.h"
#include "TradeEngine.h"
#include "Types.h"
#include "trading/strategy/FeatureEngine.h"
#include "trading/strategy/MarketOrderBook.h"

namespace Trading
{
MarketMaker::MarketMaker(QuickLogger *logger, TradeEngine *tradeEngine, FeatureEngine *featureEngine,
                         OrderManager *orderManager, TradeEngineConfigHashMap const &tickerConfig)
    : mFeatureEngine(featureEngine), mOrderManager(orderManager), mLogger(logger), mTickerConfig(tickerConfig)
{
    tradeEngine->mOnTradeUpdate = [this](Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book) {
        OnTradeUpdate(marketUpdate, book);
    };
    tradeEngine->mOnOrderBookUpdate = [this](TickerId tickerId, Price price, Side side, const MarketOrderBook *book) {
        OnOrderBookUpdate(tickerId, price, side, book);
    };
    tradeEngine->mOnOrderUpdate = [this](Exchange::MEClientResponse *clientResponse) { OnOrderUpdate(clientResponse); };
}

void MarketMaker::OnOrderBookUpdate(TickerId tickerId, Price price, Side side, const MarketOrderBook *book)
{
    mLogger->Log("MarketMaker::OnOrderBookUpdate(tickerId: ", tickerId, "; price: ", price,
                 "; side: ", SideToString(side), ")\n");

    const auto &bbo = book->GetBestBidOffer();
    const auto fairPrice = mFeatureEngine->GetFairMarketPrice();

    if (bbo.bidPrice != Price_INVALID && bbo.askPrice != Price_INVALID && fairPrice != Feature_INVALID) [[likely]]
    {
        mLogger->Log("MarketMaker::OnOrderBookUpdate() found fair price: ", fairPrice, "\n");

        auto clip = mTickerConfig[tickerId].clip;
        auto threshold = mTickerConfig[tickerId].threshold;

        const auto bidPrice = bbo.bidPrice - (fairPrice - bbo.bidPrice >= threshold ? 0 : 1);
        const auto askPrice = bbo.bidPrice + (bbo.askPrice + fairPrice >= threshold ? 0 : 1);

        mOrderManager->MoveOrders(tickerId, bidPrice, askPrice, clip);
    }
}

void MarketMaker::OnOrderUpdate(Exchange::MEClientResponse *clientResponse)
{
    mOrderManager->OnOrderUpdate(clientResponse);
}

void MarketMaker::OnTradeUpdate(Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book)
{
    /* Do nothing */
    (void)marketUpdate;
}

} // namespace Trading