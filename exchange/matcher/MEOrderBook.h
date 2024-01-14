#pragma once

#include "Limits.h"
#include "Logger.h"
#include "MemoryPool.h"
#include "Types.h"
#include "exchange/market_data/MarketUpdate.h"
#include "exchange/matcher/MEOrder.h"
#include "exchange/order_server/ClientResponse.h"

namespace Exchange
{

class MatchingEngine;

class MEOrderBook
{
public:
    MEOrderBook(TickerId tickerId, QuickLogger *logger, MatchingEngine *matchingEngine);
    ~MEOrderBook();

    OrderId GenerateNewMarketOrderId()
    {
        return mNextOrderId++;
    }

    MEOrdersAtPrice *GetOrdersAtPrice(Price price)
    {
        return mOrdersAtPrice[PriceToIndex(price)];
    }

    MEOrderBook() = delete;
    MEOrderBook(const MEOrderBook &) = delete;
    MEOrderBook(const MEOrderBook &&) = delete;
    MEOrderBook &operator=(const MEOrderBook &) = delete;
    MEOrderBook &operator=(const MEOrderBook &&) = delete;

private:
    void Add(ClientId clientId, OrderId clientOrderId, TickerId tickerId, Side side, Price price, Quantity qty);
    void Cancel(ClientId clientId, OrderId clientOrderId, TickerId tickerId);

    Quantity CheckForMatch(ClientId clientId, OrderId clientOrderId, TickerId tickerId, Side side, Price price,
                           Quantity qty, OrderId marketOrderId);

    void Match(ClientId clientId, TickerId tickerId, Side side, OrderId clientOrderId, OrderId newMartkerOrderId,
               MEOrder *order, Quantity &leavesQuantity);

    Priority GetNextPriority(Price price);

    void AddOrder(MEOrder *order);
    void RemoveOrder(MEOrder *order);

    void AddOrdersAtPrice(MEOrdersAtPrice *ordersAtPrice);
    void RemoveOrdersAtPrice(Side side, Price price);

    u32 PriceToIndex(Price price)
    {
        return price % ME_MAX_PRICE_LEVELS;
    }

private:
    MatchingEngine *mMatchingEngine;

    ClientOrderHashMap mClientIdToOrderId;

    MemoryPool<MEOrdersAtPrice> mOrdersAtPricePool;
    MEOrdersAtPrice *mBidsByPrice = nullptr;
    MEOrdersAtPrice *mAsksByPrice = nullptr;

    OrdersAtPriceHashMap mOrdersAtPrice;

    MemoryPool<MEOrder> mOrdersPool;

    TickerId mTickerId = TickerId_INVALID;

    MEClientResponse mClientResponse;
    MEMarketUpdate mMarketUpdate;

    OrderId mNextOrderId = 0;

    QuickLogger *mLogger;
};

using OrderBookHashMap = std::array<MEOrderBook *, ME_MAX_TICKERS>;

} // namespace Exchange