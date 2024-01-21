#pragma once

#include "SafeQueue.h"
#include "Types.h"
#include "exchange/order_server/ClientRequest.h"
#include <sstream>

namespace Exchange
{
enum class ClientResponseType : u8
{
    INVALID = 0,
    ACCEPTED = 1,
    CANCELED = 2,
    FILLED = 3,
    CANCEL_REJECTED = 4,
};

inline auto ClientResponseTypeToString(ClientResponseType type) -> std::string
{
    switch (type)
    {
    case ClientResponseType::INVALID:
        return "INVALID";
    case ClientResponseType::ACCEPTED:
        return "ACCEPTED";
    case ClientResponseType::CANCELED:
        return "CANCELED";
    case ClientResponseType::FILLED:
        return "FILLED";
    case ClientResponseType::CANCEL_REJECTED:
        return "CANCEL_REJECTED";
    }
    return "UNKNOWN";
}

#pragma pack(push, 1)

struct MEClientResponse
{
    ClientResponseType type = ClientResponseType::INVALID;
    ClientId clientId = ClientId_INVALID;
    TickerId tickerId = TickerId_INVALID;
    OrderId clientOrderId = OrderId_INVALID;
    OrderId marketOrderId = OrderId_INVALID;
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    Quantity executed_quantity = Quantity_INVALID;
    Quantity leaves_quantity = Quantity_INVALID;
    inline auto ToString(u32 indent = 0) const -> std::string
    {
        std::string indentString(indent, '\t');

        std::stringstream ss;
        ss << indentString << "MEClientResponse {\n";
        ss << indentString << "\ttype: " << ClientResponseTypeToString(type) << '\n';
        ss << indentString << "\tclientId: " << ClientIdToString(clientId) << '\n';
        ss << indentString << "\ttickerId: " << TickerIdToString(tickerId) << '\n';
        ss << indentString << "\tclientOrderId: " << OrderIdToString(clientOrderId) << '\n';
        ss << indentString << "\tmarketOrderId: " << OrderIdToString(marketOrderId) << '\n';
        ss << indentString << "\tside: " << SideToString(side) << '\n';
        ss << indentString << "\tprice: " << PriceToString(price) << '\n';
        ss << indentString << "\texecuted_quantity: " << QuantityToString(executed_quantity) << '\n';
        ss << indentString << "\tleaves_quantity: " << QuantityToString(leaves_quantity) << '\n';
        ss << indentString << "}";

        return ss.str();
    };
};

struct OMClientResponse
{
    u64 sequenceNumber = 0;
    MEClientResponse clientResponse;

    inline auto ToString() const -> std::string
    {
        std::stringstream ss;

        ss << "OMClientResponse {\n";
        ss << "\tsequenceNumber : " << sequenceNumber << '\n';
        ss << "\tclientResponse : " << clientResponse.ToString(1) << "\n";
        ss << "}";

        return ss.str();
    }
};

#pragma pack(pop)

using MEClientResponseQueue = SafeQueue<MEClientResponse>;

} // namespace Exchange