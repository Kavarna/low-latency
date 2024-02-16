#pragma once

#include "Limits.h"
#include "Types.h"
#include <sstream>
#include <string>
namespace Trading
{
enum class OMOrderState : i8
{
    INVALID = 0,
    PENDING_NEW = 1,
    LIVE = 2,
    PENDING_CANCEL = 3,
    DEAD = 4,
};

inline auto OMOrderStateToString(OMOrderState state) -> std::string
{
    switch (state)
    {
    case Trading::OMOrderState::INVALID:
        return "INVALID";
    case Trading::OMOrderState::PENDING_NEW:
        return "PENDING_NEW";
    case Trading::OMOrderState::LIVE:
        return "LIVE";
    case Trading::OMOrderState::PENDING_CANCEL:
        return "PENDING_CANCEL";
    case Trading::OMOrderState::DEAD:
        return "DEAD";
    default:
        return "UNKNOWN";
    }
}

struct OMOrder
{
    TickerId tickerId = TickerId_INVALID;
    OrderId orderId = OrderId_INVALID;
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    Quantity quantity = Quantity_INVALID;
    OMOrderState state = OMOrderState::INVALID;

    auto ToString() -> std::string
    {
        std::stringstream ss;
        ss << "OMOrder {\n"
           << "tickerId: " << TickerIdToString(tickerId) << '\n'
           << "orderId: " << OrderIdToString(orderId) << '\n'
           << "side: " << SideToString(side) << '\n'
           << "price: " << PriceToString(price) << '\n'
           << "quantity: " << QuantityToString(quantity) << '\n'
           << "state: " << OMOrderStateToString(state) << '\n'
           << "}";

        return ss.str();
    }
};

using OMOrderSideHashMap = std::array<OMOrder, SideToIndex(Side::MAX) + 1>;
using OMOrderTickerSideHashMap = std::array<OMOrderSideHashMap, ME_MAX_TICKERS>;

} // namespace Trading