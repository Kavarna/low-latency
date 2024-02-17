#pragma once

#include "trading/strategy/FeatureEngine.h"
#include "trading/strategy/MarketOrderBook.h"
#include "trading/strategy/OrderManager.h"
#include "trading/strategy/RiskManager.h"
namespace Trading
{
class TradeEngine;
class LiquidityTaker
{
public:
    LiquidityTaker(QuickLogger *logger, TradeEngine *tradeEngine, FeatureEngine *featureEngine,
                   OrderManager *orderManager, TradeEngineConfigHashMap const &tickerConfig);

    LiquidityTaker() = delete;
    LiquidityTaker(const LiquidityTaker &) = delete;
    LiquidityTaker(const LiquidityTaker &&) = delete;
    LiquidityTaker &operator=(const LiquidityTaker &) = delete;
    LiquidityTaker &operator=(const LiquidityTaker &&) = delete;

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