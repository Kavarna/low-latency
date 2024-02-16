#pragma once

#include "Check.h"
#include "Limits.h"
#include "Logger.h"
#include "Types.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/strategy/MarketOrder.h"

#include <array>
#include <sstream>
#include <string>

namespace Trading
{

struct PositionInfo
{
    i32 position = 0;
    f64 realizedPnL = 0.0;
    f64 unrealizedPnL = 0.0;
    f64 totalPnL = 0.0;
    std::array<f64, SideToIndex(Side::MAX)> openVWAP; /* Volume Weighted Average price for open positions */
    Quantity volume;
    BestBidOffer const *bbo = nullptr;

    auto ToString() const -> std::string
    {
        std::stringstream ss;
        ss << "PositionInfo {\n";
        ss << "\tposition: " << position << "\n";
        ss << "\trealizedPnL: " << realizedPnL << "\n";
        ss << "\tunrealizedPnL: " << unrealizedPnL << "\n";
        ss << "\ttotalPnL: " << totalPnL << "\n";
        ss << "\tvolume: " << QuantityToString(volume) << "\n";
        ss << "\tvwaps: [ " << (position ? (openVWAP[SideToIndex(Side::BUY)] / std::abs(position)) : 0) << " x "
           << (position ? (openVWAP[SideToIndex(Side::SELL)] / std::abs(position)) : 0) << "\n";
        ss << "\tbbo: " << bbo->ToString() << "\n";
        ss << "}";

        return ss.str();
    }

    void AddFill(Exchange::MEClientResponse *clientResponse, QuickLogger *logger)
    {
        const auto oldPosition = position;
        auto sideIndex = SideToIndex(clientResponse->side);
        auto invertSideIndex = SideToIndex(clientResponse->side == Side::BUY ? Side::SELL : Side::BUY);
        auto sideValue = SideToValue(clientResponse->side);

        position += clientResponse->executed_quantity * sideValue;
        volume += clientResponse->executed_quantity;

        if (oldPosition * sideValue >= 0)
        {
            // new buy after buy / new sell after sell
            openVWAP[sideIndex] += clientResponse->price * clientResponse->executed_quantity;
        }
        else
        {
            // we must close some positions

            // Get the price from the old VWAP
            auto oldPrice = openVWAP[invertSideIndex] / std::abs(oldPosition);

            // Update the VWAP after updating the position
            openVWAP[invertSideIndex] = oldPrice * std::abs(position);

            // Calculate the Profit based on the old price
            realizedPnL = std::min((i32)clientResponse->executed_quantity, std::abs(oldPosition)) *
                          (oldPrice - clientResponse->price) * SideToValue(clientResponse->side);

            if (position * oldPosition < 0)
            {
                // The position has been flipped
                openVWAP[sideIndex] = clientResponse->price * std::abs(position);
                openVWAP[invertSideIndex] = 0;
            }
        }

        if (position == 0)
        {
            openVWAP[SideToIndex(Side::BUY)] = 0;
            openVWAP[SideToIndex(Side::SELL)] = 0;
            unrealizedPnL = 0;
        }
        else
        {
            // We still have an open position => recalculate unrealized pnl

            if (position > 0)
            {
                // (currentPrice - openVWAP / position) * position
                unrealizedPnL = (clientResponse->price - openVWAP[SideToIndex(Side::BUY)] / std::abs(position)) *
                                std::abs(position);
            }
            else
            {
                // (openVWAP / position - currentPrice) * position
                unrealizedPnL = (openVWAP[SideToIndex(Side::BUY)] / std::abs(position) - clientResponse->price) *
                                std::abs(position);
            }
        }

        totalPnL = unrealizedPnL + realizedPnL;

        logger->Log("PositionInfo::AddFill(", clientResponse->ToString(), ")\n");
    }

    void UpdateBestBidOffer(BestBidOffer *bbo, QuickLogger *logger)
    {
        this->bbo = bbo;

        if (position && bbo->bidPrice != Price_INVALID && bbo->askPrice != Price_INVALID)
        {
            auto midPrice = (bbo->bidPrice + bbo->askPrice) * 0.5;
            if (position > 0)
            {
                unrealizedPnL = (midPrice - openVWAP[SideToIndex(Side::BUY)] / std::abs(position)) * std::abs(position);
            }
            else
            {
                unrealizedPnL = (openVWAP[SideToIndex(Side::BUY)] / std::abs(position) - midPrice) * std::abs(position);
            }

            auto oldTotalPnl = totalPnL;
            totalPnL = unrealizedPnL + realizedPnL;

            if (totalPnL != oldTotalPnl)
            {
                logger->Log("PositionInfo::UpdateBestBidOffer(", bbo->ToString(),
                            ") results in a new total pnl: ", ToString(), "\n");
            }
        }
    }
};

class PositionKeeper
{
public:
    PositionKeeper(QuickLogger *logger) : mLogger(logger){};

    PositionKeeper() = delete;
    PositionKeeper(const PositionKeeper &) = delete;
    PositionKeeper(const PositionKeeper &&) = delete;
    PositionKeeper &operator=(const PositionKeeper &) = delete;
    PositionKeeper &operator=(const PositionKeeper &&) = delete;

    void AddFill(Exchange::MEClientResponse *clientResponse)
    {
        mTickerPositions[clientResponse->tickerId].AddFill(clientResponse, mLogger);
    }

    void UpdateBestBidOffer(TickerId tickerId, BestBidOffer *bbo)
    {
        mTickerPositions[tickerId].UpdateBestBidOffer(bbo, mLogger);
    }

    PositionInfo const *GetPositionInfo(TickerId ticker) const
    {
        return &mTickerPositions[ticker];
    }

    auto ToString() -> std::string
    {
        double totalPnL = 0.0;
        Quantity totalQuantity = 0.0;

        std::stringstream ss;
        for (u32 i = 0; i < mTickerPositions.size(); ++i)
        {
            ss << "TickerId: " << TickerIdToString(i) << " " << mTickerPositions[i].ToString() << "\n";

            totalPnL = mTickerPositions[i].totalPnL;
            totalQuantity = mTickerPositions[i].volume;
        }

        ss << "Total PnL: " << totalPnL << "; Total Quantity: " << totalQuantity << "\n";

        return ss.str();
    }

private:
    QuickLogger *mLogger = nullptr;

    std::array<PositionInfo, ME_MAX_TICKERS> mTickerPositions;
};

} // namespace Trading