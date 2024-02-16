#pragma once

#include "Logger.h"
#include "MarketUpdate.h"
#include "Types.h"
#include "trading/strategy/MarketOrderBook.h"
#include <limits>

namespace Trading
{
constexpr auto Feature_INVALID = std::numeric_limits<f64>::quiet_NaN();
class FeatureEngine
{
public:
    FeatureEngine(QuickLogger *logger);

    auto GetFairMarketPrice() const
    {
        return mMarketPrice;
    }

    auto GetAggresiveTradeQuantityRatio() const
    {
        return mAggresiveTradeQuantityRatio;
    }

    void OnOrderBookUpdate(TickerId ticker, Price price, Side side, MarketOrderBook *book)
    {
        auto bbo = book->GetBestBidOffer();
        if (bbo.bidPrice != Price_INVALID && bbo.askPrice != Price_INVALID) [[likely]]
        {
            mMarketPrice = (f64)(bbo.bidPrice * bbo.askQuantity + bbo.askPrice * bbo.bidQuantity) /
                           ((f64)bbo.bidQuantity + (f64)bbo.askQuantity);
        }

        mLogger->Log("FeatureEngine::OnOrderBookUpdate() -> new fair market price is ", mMarketPrice, "\n");
    }

    void OnTradeUpdate(Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book)
    {
        auto bbo = book->GetBestBidOffer();
        if (bbo.bidPrice != Price_INVALID && bbo.askPrice != Price_INVALID) [[likely]]
        {
            mAggresiveTradeQuantityRatio =
                f64(marketUpdate->quantity) / (marketUpdate->side == Side::BUY ? bbo.bidQuantity : bbo.askQuantity);
        }

        mLogger->Log("FeatureEngine::OnTradeUpdate() -> new aggressive trade quantity ratio is ",
                     mAggresiveTradeQuantityRatio, "\n");
    }

    FeatureEngine() = delete;
    FeatureEngine(const FeatureEngine &) = delete;
    FeatureEngine(const FeatureEngine &&) = delete;
    FeatureEngine &operator=(const FeatureEngine &) = delete;
    FeatureEngine &operator=(const FeatureEngine &&) = delete;

private:
    QuickLogger *mLogger;

    f64 mMarketPrice = Feature_INVALID;
    f64 mAggresiveTradeQuantityRatio = Feature_INVALID;
};
} // namespace Trading
