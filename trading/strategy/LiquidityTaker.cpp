#include "LiquidityTaker.h"
#include "TradeEngine.h"
#include "Types.h"
#include "trading/strategy/FeatureEngine.h"
#include "trading/strategy/MarketOrderBook.h"

namespace Trading
{
LiquidityTaker::LiquidityTaker(QuickLogger *logger, TradeEngine *tradeEngine, FeatureEngine *featureEngine,
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

void LiquidityTaker::OnOrderBookUpdate(TickerId ticker, Price price, Side side, const MarketOrderBook *book)
{
    /* Do nothing */
    (void)ticker;
    (void)price;
    (void)side;
    (void)book;
}

void LiquidityTaker::OnOrderUpdate(Exchange::MEClientResponse *clientResponse)
{
    mOrderManager->OnOrderUpdate(clientResponse);
}

void LiquidityTaker::OnTradeUpdate(Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book)
{
    mLogger->Log("LiquidityTaker::OnTradeUpdate(marketUpdate: ", marketUpdate->ToString(), "; book\n");

    auto &bbo = book->GetBestBidOffer();
    auto aggresiveQuantityRatio = mFeatureEngine->GetAggresiveTradeQuantityRatio();

    if (bbo.bidPrice != Price_INVALID && bbo.askPrice != Price_INVALID && aggresiveQuantityRatio != Feature_INVALID)
        [[likely]]
    {
        mLogger->Log("LiquidityTaker::OnTradeUpdate(): Found aggresive quantity ratio");

        auto clip = mTickerConfig[marketUpdate->tickerId].clip;
        auto threshold = mTickerConfig[marketUpdate->tickerId].threshold;

        if (aggresiveQuantityRatio > threshold)
        {
            if (marketUpdate->side == Side::BUY)
                mOrderManager->MoveOrders(marketUpdate->tickerId, bbo.askPrice, Price_INVALID, clip);
            else
                mOrderManager->MoveOrders(marketUpdate->tickerId, Price_INVALID, bbo.bidPrice, clip);
        }
    }
}

} // namespace Trading