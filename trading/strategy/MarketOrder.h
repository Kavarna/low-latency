#pragma once

#include "Limits.h"
#include "Types.h"
#include <sstream>
#include <string>

namespace Trading
{
struct MarketOrder
{
    OrderId orderId = OrderId_INVALID;
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    Quantity quantity = Quantity_INVALID;
    Priority priority = Priority_INVALID;

    MarketOrder *prevOrder = nullptr;
    MarketOrder *nextOrder = nullptr;

    MarketOrder() = default;
    MarketOrder(OrderId orderId, Side side, Price price, Quantity qty, Priority priority, MarketOrder *prevOrder,
                MarketOrder *nextOrder)
        : orderId(orderId), side(side), price(price), quantity(qty), priority(priority), prevOrder(prevOrder),
          nextOrder(nextOrder)
    {
    }

    inline auto ToString(u32 indent = 0) -> std::string
    {
        std::string indentString(indent, '\t');
        std::string nullString = "NULL";
        std::stringstream ss;

        ss << indentString << "MarketOrder{\n";
        ss << indentString << "\torderId: " << OrderIdToString(orderId) << "\n";
        ss << indentString << "\tside: " << SideToString(side) << "\n";
        ss << indentString << "\tprice: " << PriceToString(price) << "\n";
        ss << indentString << "\tqty: " << QuantityToString(quantity) << "\n";
        ss << indentString << "\tpriority: " << PriorityToString(priority) << "\n";
        ss << indentString << "\tprevOrder: " << ((prevOrder != nullptr) ? prevOrder->ToString(indent + 1) : nullString)
           << "\n";
        ss << indentString << "\tnextOrder: " << ((nextOrder != nullptr) ? nextOrder->ToString(indent + 1) : nullString)
           << "\n";
        ss << indentString << "}";

        return ss.str();
    };
};

using OrderHashMap = std::array<MarketOrder *, ME_MAX_ORDER_IDS>;

struct MarketOrdersAtPrice
{
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    MarketOrder *firstMarketOrder = nullptr;

    MarketOrdersAtPrice *prevEntry = nullptr;
    MarketOrdersAtPrice *nextEntry = nullptr;

    MarketOrdersAtPrice() = default;
    MarketOrdersAtPrice(Side side, Price price, MarketOrder *firstMarketOrder, MarketOrdersAtPrice *prevEntry,
                        MarketOrdersAtPrice *nextEntry)
        : side(side), price(price), firstMarketOrder(firstMarketOrder), prevEntry(prevEntry), nextEntry(nextEntry)
    {
    }

    auto ToString() -> std::string
    {
        std::stringstream ss;
        ss << "MarketOrdersAtPrice {\n"
           << "\tside: " << SideToString(side) << "\n"
           << "\tprice: " << PriceToString(price) << "\n"
           << "\tfirstMarketOrder: " << (firstMarketOrder ? firstMarketOrder->ToString(1) : "null") << "\n"
           << "\tprev: " << PriceToString(prevEntry ? prevEntry->price : Price_INVALID) << "\n"
           << "\tnext: " << PriceToString(nextEntry ? nextEntry->price : Price_INVALID) << "\n"
           << "}";

        return ss.str();
    }
};

using OrdersAtPriceHashMap = std::array<MarketOrdersAtPrice *, ME_MAX_PRICE_LEVELS>;

struct BestBidOffer
{
    Price bidPrice = Price_INVALID;
    Price askPrice = Price_INVALID;
    Quantity bidQuantity = Quantity_INVALID;
    Quantity askQuantity = Quantity_INVALID;

    auto ToString() const -> std::string
    {
        std::stringstream ss;
        ss << "BestBidOffer {\n"
           << "\tbidPrice: " << PriceToString(bidPrice) << "\n"
           << "\taskPrice: " << PriceToString(askPrice) << "\n"
           << "\tbidQuantity: " << QuantityToString(bidQuantity) << "\n"
           << "\taskQuantity: " << QuantityToString(askQuantity) << "\n"
           << "}";

        return ss.str();
    }
};

} // namespace Trading
