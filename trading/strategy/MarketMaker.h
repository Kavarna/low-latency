#pragma once

#include "Logger.h"
#include "MarketUpdate.h"
#include "Types.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/strategy/FeatureEngine.h"
#include "trading/strategy/MarketOrderBook.h"
#include "trading/strategy/OrderManager.h"
#include "trading/strategy/RiskManager.h"
namespace Trading
{
class TradeEngine;
class MarketMaker
{
public:
    MarketMaker(QuickLogger *logger, TradeEngine *tradeEngine, FeatureEngine *featureEngine, OrderManager *orderManager,
                TradeEngineConfigHashMap const &tickerConfig);

    MarketMaker() = delete;
    MarketMaker(const MarketMaker &) = delete;
    MarketMaker(const MarketMaker &&) = delete;
    MarketMaker &operator=(const MarketMaker &) = delete;
    MarketMaker &operator=(const MarketMaker &&) = delete;

    void OnOrderBookUpdate(TickerId ticker, Price price, Side side, const MarketOrderBook *book);
    void OnOrderUpdate(Exchange::MEClientResponse *clientResponse);
    void OnTradeUpdate(Exchange::MEMarketUpdate *marketUpdate, MarketOrderBook *book);

private:
    FeatureEngine *mFeatureEngine = nullptr;
    OrderManager *mOrderManager = nullptr;

    QuickLogger *mLogger;
    TradeEngineConfigHashMap mTickerConfig;
};
} // namespace Trading