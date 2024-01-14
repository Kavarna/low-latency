#pragma once

#include "Limits.h"
#include "Types.h"

#include <array>
#include <sstream>
#include <string>

namespace Exchange
{
struct MEOrder
{
    TickerId tickerId = TickerId_INVALID;
    ClientId clientId = ClientId_INVALID;
    OrderId clientOrderId = OrderId_INVALID;
    OrderId marketOrderId = OrderId_INVALID;
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    Quantity quantity = Quantity_INVALID;
    Priority priority = Priority_INVALID;

    MEOrder *nextOrder = nullptr;
    MEOrder *prevOrder = nullptr;

    MEOrder() = default;
    MEOrder(TickerId tickerId, ClientId clientId, OrderId clientOrderId, OrderId marketOrderId, Side side, Price price,
            Quantity quantity, Priority priority, MEOrder *previousOrder, MEOrder *nextOrder)
        : tickerId(tickerId), clientId(clientId), clientOrderId(clientOrderId), marketOrderId(marketOrderId),
          side(side), price(price), quantity(quantity), priority(priority), nextOrder(nextOrder),
          prevOrder(previousOrder)
    {
    }

    inline auto ToString(u32 indent = 0) -> std::string
    {
        std::string indentString(indent, '\t');
        std::string nullString = "NULL";
        std::stringstream ss;

        ss << indentString << "MEOrder {\n";
        ss << indentString << "\ttickerId: " << TickerIdToString(tickerId) << "\n";
        ss << indentString << "\tclientId: " << ClientIdToString(clientId) << "\n";
        ss << indentString << "\tclientOrderId: " << OrderIdToString(clientOrderId) << "\n";
        ss << indentString << "\tmarketOrderId: " << OrderIdToString(marketOrderId) << "\n";
        ss << indentString << "\tside: " << SideToString(side) << "\n";
        ss << indentString << "\tprice: " << PriceToString(price) << "\n";
        ss << indentString << "\tquantity: " << QuantityToString(quantity) << "\n";
        ss << indentString << "\tpriority: " << PriorityToString(priority) << "\n";
        ss << indentString << "\tnextOrder: " << ((nextOrder != nullptr) ? nextOrder->ToString(indent + 1) : nullString)
           << "\n";
        ss << indentString << "\tprevOrder: " << ((prevOrder != nullptr) ? prevOrder->ToString(indent + 1) : nullString)
           << "\n";
        ss << indentString << "}";

        return ss.str();
    }
};

// Maybe use market order id as key?
using OrderHashMap = std::array<MEOrder *, ME_MAX_ORDER_IDS>;
using ClientOrderHashMap = std::array<OrderHashMap, ME_MAX_NUM_CLIENTS>;

struct MEOrdersAtPrice
{
    Side side = Side::INVALID;
    Price price = Price_INVALID;

    MEOrder *firstOrder = nullptr;

    MEOrdersAtPrice *nextEntry = nullptr;
    MEOrdersAtPrice *prevEntry = nullptr;

    MEOrdersAtPrice() = default;

    MEOrdersAtPrice(Side side, Price price, MEOrder *firstOrder, MEOrdersAtPrice *nextEntry,
                    MEOrdersAtPrice *previousEntry)
        : side(side), price(price), firstOrder(firstOrder), nextEntry(nextEntry), prevEntry(previousEntry)
    {
    }

    auto ToString() const
    {
        std::stringstream ss;
        ss << "MEOrdersAtPrice {\n"
           << "\tside:" << SideToString(side) << "\n"
           << "\tprice:" << PriceToString(price) << "\n"
           << "\tfirst_me_order:" << (firstOrder ? firstOrder->ToString(1) : "null") << "\n"
           << "\tprev:" << PriceToString(prevEntry ? prevEntry->price : Price_INVALID) << "\n"
           << "\tnext:" << PriceToString(nextEntry ? nextEntry->price : Price_INVALID) << "\n"
           << "}";

        return ss.str();
    }
};

using OrdersAtPriceHashMap = std::array<MEOrdersAtPrice *, ME_MAX_PRICE_LEVELS>;

} // namespace Exchange