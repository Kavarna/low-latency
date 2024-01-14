#pragma once

#include <cstdint>
#include <limits>
#include <string>

using i8 = int8_t;
using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s32 = int32_t;
using f32 = float;
using f64 = double;

using OrderId = u64;
constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();
inline auto OrderIdToString(OrderId orderId) -> std::string
{
    if (orderId == OrderId_INVALID) [[unlikely]]
    {
        return "INVALID";
    }
    return std::to_string(orderId);
}

using TickerId = u32;
constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();
inline auto TickerIdToString(TickerId tickerId) -> std::string
{
    if (tickerId == TickerId_INVALID) [[unlikely]]
    {
        return "INVALID";
    }
    return std::to_string(tickerId);
}

using ClientId = u32;
constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();
inline auto ClientIdToString(ClientId clientID) -> std::string
{
    if (clientID == ClientId_INVALID) [[unlikely]]
    {
        return "INVALID";
    }
    return std::to_string(clientID);
}

using Price = u64;
constexpr auto Price_INVALID = std::numeric_limits<Price>::max();
inline auto PriceToString(Price price) -> std::string
{
    if (price == Price_INVALID) [[unlikely]]
    {
        return "INVALID";
    }
    return std::to_string(price);
}

using Quantity = u32;
constexpr auto Quantity_INVALID = std::numeric_limits<Quantity>::max();
inline auto QuantityToString(Quantity quantity) -> std::string
{
    if (quantity == Quantity_INVALID) [[unlikely]]
    {
        return "INVALID";
    }
    return std::to_string(quantity);
}

using Priority = u64;
constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();
inline auto PriorityToString(Priority priority) -> std::string
{
    if (priority == Priority_INVALID) [[unlikely]]
    {
        return "INVALID";
    }
    return std::to_string(priority);
}

enum class Side : i8
{
    INVALID = 0,
    BUY = 1,
    SELL = -1
};
inline auto SideToString(Side side) -> std::string
{
    switch (side)
    {
    case Side::INVALID:
        return "INVALID";
    case Side::BUY:
        return "BUY";
    case Side::SELL:
        return "SELL";
    }
    return "UNKNOWN";
}