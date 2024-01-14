#pragma once

#include "Limits.h"
#include "SafeQueue.h"
#include "Types.h"
#include <sstream>
#include <string>

namespace Exchange
{

enum class MarketUpdateType : u8
{
    INVALID = 0,
    ADD = 1,
    MODIFY = 2,
    CANCEL = 3,
    TRADE = 4
};

inline auto MarketUpdateTypeToString(MarketUpdateType type) -> std::string
{
    switch (type)
    {
    case MarketUpdateType::INVALID:
        return "INVALID";
    case MarketUpdateType::ADD:
        return "ADD";
    case MarketUpdateType::MODIFY:
        return "MODIFY";
    case MarketUpdateType::CANCEL:
        return "CANCEL";
    case MarketUpdateType::TRADE:
        return "TRADE";
    }
    return "UNKNOWN";
}

#pragma pack(push, 1)

struct MEMarketUpdate
{
    MarketUpdateType type = MarketUpdateType::INVALID;
    OrderId orderId = OrderId_INVALID;
    TickerId tickerId = TickerId_INVALID;
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    Priority priority = Priority_INVALID;
    Quantity quantity = Quantity_INVALID;

    inline auto ToString() const -> std::string
    {
        std::stringstream ss;

        ss << "MEMarketUpdate {\n";
        ss << "\ttype : " << MarketUpdateTypeToString(type) << '\n';
        ss << "\torderId : " << OrderIdToString(orderId) << '\n';
        ss << "\ttickerId : " << TickerIdToString(tickerId) << '\n';
        ss << "\tside : " << SideToString(side) << '\n';
        ss << "\tprice : " << PriceToString(price) << '\n';
        ss << "\tpriority : " << PriorityToString(priority) << '\n';
        ss << "\tquantity : " << QuantityToString(quantity) << '\n';
        ss << "}";

        return ss.str();
    }
};

#pragma pack(pop)

using MEMarketUpdateQueue = SafeQueue<MEMarketUpdate>;

} // namespace Exchange