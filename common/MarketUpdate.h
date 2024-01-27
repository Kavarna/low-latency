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
    TRADE = 4,
    CLEAR = 5,
    SNAPSHOT_START = 6,
    SNAPSHOT_END = 7
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
    case MarketUpdateType::CLEAR:
        return "CLEAR";
    case MarketUpdateType::SNAPSHOT_START:
        return "SNAPSHOT_START";
    case MarketUpdateType::SNAPSHOT_END:
        return "SNAPSHOT_END";
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

    inline auto ToString(u32 indent = 0) const -> std::string
    {
        std::string indentString(indent, '\t');
        std::stringstream ss;

        ss << indentString << "MEMarketUpdate {\n";
        ss << indentString << "\ttype : " << MarketUpdateTypeToString(type) << '\n';
        ss << indentString << "\torderId : " << OrderIdToString(orderId) << '\n';
        ss << indentString << "\ttickerId : " << TickerIdToString(tickerId) << '\n';
        ss << indentString << "\tside : " << SideToString(side) << '\n';
        ss << indentString << "\tprice : " << PriceToString(price) << '\n';
        ss << indentString << "\tpriority : " << PriorityToString(priority) << '\n';
        ss << indentString << "\tquantity : " << QuantityToString(quantity) << '\n';
        ss << indentString << "}";

        return ss.str();
    }
};

struct MPDMarketUpdate
{
    u64 sequenceNumber = 0;
    MEMarketUpdate marketUpdate{};

    inline auto ToString() const -> std::string
    {
        std::stringstream ss;

        ss << "MPDMarketUpdate {\n";
        ss << "\tsequenceNumber : " << sequenceNumber << '\n';
        ss << "\tmarketUpdate : " << marketUpdate.ToString(1) << "\n";
        ss << "}";

        return ss.str();
    }
};

#pragma pack(pop)

using MEMarketUpdateQueue = SafeQueue<MEMarketUpdate>;
using MPDMarketUpdateQueue = SafeQueue<MPDMarketUpdate>;

} // namespace Exchange