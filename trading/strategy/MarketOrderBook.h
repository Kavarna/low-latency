#pragma once

#include "Limits.h"
#include "Logger.h"
#include "MemoryPool.h"
#include "Types.h"
#include "common/MarketUpdate.h"
#include "trading/strategy/MarketOrder.h"

namespace Trading
{
class TradeEngine;

class MarketOrderBook
{
public:
    MarketOrderBook(TickerId tickerId, QuickLogger *logger);
    ~MarketOrderBook();

    MarketOrderBook() = delete;
    MarketOrderBook(const MarketOrderBook &) = delete;
    MarketOrderBook(const MarketOrderBook &&) = delete;
    MarketOrderBook &operator=(const MarketOrderBook &) = delete;
    MarketOrderBook &operator=(const MarketOrderBook &&) = delete;

    void SetTradeEngine(TradeEngine *tradeEngine)
    {
        mTradeEngine = tradeEngine;
    }

    void OnMarketUpdate(Exchange::MEMarketUpdate *marketUpdate);

    BestBidOffer const &GetBestBidOffer() const
    {
        return mBestBidOffer;
    }

private:
    void AddOrder(MarketOrder *order);
    void RemoveOrder(MarketOrder *order);

    void AddOrdersAtPrice(MarketOrdersAtPrice *ordersAtPrice);
    void RemoveOrdersAtPrice(Side side, Price price);

    void UpdateBestBidOffer(bool bidUpdated, bool askUpdated);

    u32 PriceToIndex(Price price)
    {
        return price % ME_MAX_PRICE_LEVELS;
    }

    MarketOrdersAtPrice *GetOrdersAtPrice(Price price)
    {
        return mPriceOrdersAtPrice[PriceToIndex(price)];
    }

private:
    TickerId mTickerId;
    TradeEngine *mTradeEngine;

    OrderHashMap mOrderIdToOrder;

    MemoryPool<MarketOrdersAtPrice> mOrdersAtPricePool;
    MarketOrdersAtPrice *mBidsByPrice = nullptr;
    MarketOrdersAtPrice *mAsksByPrice = nullptr;

    MemoryPool<MarketOrder> mMartketOrdersPool;

    OrdersAtPriceHashMap mPriceOrdersAtPrice;

    BestBidOffer mBestBidOffer;

    QuickLogger *mLogger;
};

using MarketOrderBookHashMap = std::array<MarketOrderBook *, ME_MAX_TICKERS>;
} // namespace Trading