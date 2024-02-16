#pragma once

#include "Logger.h"
#include "Types.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/strategy/MarketOrderBook.h"
#include "trading/strategy/OMOrder.h"

namespace Trading
{

class TradeEngine;
class RiskManager;

class OrderManager
{
public:
    OrderManager(QuickLogger *logger, TradeEngine *tradeEngine, RiskManager &riskManager)
        : mTradeEngine(tradeEngine), mRiskManager(riskManager), mLogger(logger)
    {
    }

    void OnOrderUpdate(Exchange::MEClientResponse *clientResponse);

    void NewOrder(OMOrder *order, TickerId tickerId, Price price, Side side, Quantity quantity);
    void CancelOrder(OMOrder *order);

    void MoveOrders(TickerId tickerId, Price bidPrice, Price askPrice, Quantity tradeSize)
    {
        auto bidOrder = &mTickerSideOrder[tickerId][SideToIndex(Side::BUY)];
        MoveOrder(bidOrder, tickerId, bidPrice, Side::BUY, tradeSize);

        auto askOrder = &mTickerSideOrder[tickerId][SideToIndex(Side::SELL)];
        MoveOrder(askOrder, tickerId, askPrice, Side::SELL, tradeSize);
    }

    auto &GetOMOrderSideHashMap(TickerId tickerId) const
    {
        return mTickerSideOrder[tickerId];
    }

private:
    void MoveOrder(OMOrder *order, TickerId tickerId, Price price, Side side, Quantity qty);

private:
    TradeEngine *mTradeEngine = nullptr;
    RiskManager &mRiskManager;
    QuickLogger *mLogger = nullptr;
    OMOrderTickerSideHashMap mTickerSideOrder;
    OrderId mNextOrderId = 1;
};

} // namespace Trading