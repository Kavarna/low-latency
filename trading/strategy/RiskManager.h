#pragma once

#include "Limits.h"
#include "Logger.h"
#include "Types.h"
#include "trading/strategy/OrderManager.h"
#include "trading/strategy/PositionKeeper.h"
#include <sstream>
#include <string>

namespace Trading
{
struct RiskConfig
{
    Quantity maxOrderSize = 0;
    Quantity maxPositions = 0;
    f64 maxLoss = 0.0;

    auto ToString() const -> std::string
    {
        std::stringstream ss;
        ss << "RiskConfig {\n"
           << "maxOrderSize: " << QuantityToString(maxOrderSize) << "\n"
           << "maxPositions: " << QuantityToString(maxPositions) << "\n"
           << "maxLoss: " << maxLoss << "\n"
           << "}";
        return ss.str();
    }
};

struct TradeEngineConfig
{
    Quantity clip = 0; // the quantity of orders that the trade engine should send out
    f64 threshold = 0.0;
    RiskConfig riskConfig;

    auto ToString() const -> std::string
    {
        std::stringstream ss;

        ss << "TradeEngineConfig {\n"
           << "clip: " << QuantityToString(clip) << "\n"
           << "threshold: " << threshold << "\n"
           << "riskConfig: " << riskConfig.ToString() << "\n"
           << "}";

        return ss.str();
    }
};

using TradeEngineConfigHashMap = std::array<TradeEngineConfig, ME_MAX_TICKERS>;

enum class RiskCheckResult : i8
{
    INVALID = 0,
    ORDER_TOO_LARGE,
    POSITION_TOO_LARGE,
    LOSS_TOO_LARGE,
    ALLOWED
};

inline std::string RiskCheckResultToString(RiskCheckResult result)
{
    switch (result)
    {
    case Trading::RiskCheckResult::INVALID:
        return "INVALID";
    case Trading::RiskCheckResult::ORDER_TOO_LARGE:
        return "ORDER_TOO_LARGE";
    case Trading::RiskCheckResult::POSITION_TOO_LARGE:
        return "POSITION_TOO_LARGE";
    case Trading::RiskCheckResult::LOSS_TOO_LARGE:
        return "LOSS_TOO_LARGE";
    case Trading::RiskCheckResult::ALLOWED:
        return "ALLOWED";
    default:
        return "INVALID";
    }
}

struct RiskInfo
{
    const PositionInfo *positioninfo = nullptr;

    RiskConfig riskConfig;

    RiskCheckResult CheckPreTradeRisk(Side side, Quantity qty) const
    {
        if (qty > riskConfig.maxOrderSize) [[unlikely]]
            return RiskCheckResult::ORDER_TOO_LARGE;
        if (abs(positioninfo->position + SideToValue(side) * (i32)qty) > (i32)riskConfig.maxPositions) [[unlikely]]
            return RiskCheckResult::POSITION_TOO_LARGE;
        if (positioninfo->totalPnL < riskConfig.maxLoss)
            return RiskCheckResult::LOSS_TOO_LARGE;

        return RiskCheckResult::ALLOWED;
    }

    auto ToString() const -> std::string
    {
        std::stringstream ss;
        ss << "RiskInfo {\n"
           << "positionInfo: " << positioninfo->ToString() << "\n"
           << "riskConfig: " << riskConfig.ToString() << "\n"
           << "}";

        return ss.str();
    }
};

using TickerRiskInfoHashMap = std::array<RiskInfo, ME_MAX_TICKERS>;

class RiskManager
{
public:
    RiskManager(PositionKeeper *positionKeeper, TradeEngineConfigHashMap &tickerConfigs)
    {
        for (TickerId i = 0; i < ME_MAX_TICKERS; ++i)
        {
            mTickerRisk[i].positioninfo = positionKeeper->GetPositionInfo(i);
            mTickerRisk[i].riskConfig = tickerConfigs[i].riskConfig;
        }
    }

    auto CheckPreTradeRisk(TickerId tickerId, Side side, Quantity qty) const
    {
        return mTickerRisk[tickerId].CheckPreTradeRisk(side, qty);
    }

    ~RiskManager() = default;

private:
    TickerRiskInfoHashMap mTickerRisk;
};
} // namespace Trading