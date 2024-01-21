#pragma once

#include "SafeQueue.h"
#include "Types.h"
#include <sstream>
#include <string>

namespace Exchange
{

enum class ClientRequestType : u8
{
    INVALID = 0,
    NEW = 1,
    CANCEL = 2
};

inline auto ClientRequestTypeToString(ClientRequestType request) -> std::string
{
    switch (request)
    {
    case ClientRequestType::INVALID:
        return "INVALID";
    case ClientRequestType::NEW:
        return "NEW";
    case ClientRequestType::CANCEL:
        return "CANCEL";
    }
    return "UNKNOWN";
}

#pragma pack(push, 1)

struct MEClientRequest
{
    ClientRequestType type = ClientRequestType::INVALID;
    ClientId clientId = ClientId_INVALID;
    TickerId tickerId = TickerId_INVALID;
    OrderId orderId = OrderId_INVALID;
    Side side = Side::INVALID;
    Price price = Price_INVALID;
    Quantity quantity = Quantity_INVALID;

    inline auto ToString(u32 indent = 0) const -> std::string
    {
        std::string indentString(indent, '\t');

        std::stringstream ss;
        ss << indentString << "MEClientRequest {\n";
        ss << indentString << "\ttype: " << ClientRequestTypeToString(type) << "\n";
        ss << indentString << "\tclientId: " << ClientIdToString(clientId) << "\n";
        ss << indentString << "\ttickerId: " << TickerIdToString(tickerId) << "\n";
        ss << indentString << "\torderId: " << OrderIdToString(orderId) << "\n";
        ss << indentString << "\tside: " << SideToString(side) << "\n";
        ss << indentString << "\tprice: " << PriceToString(price) << "\n";
        ss << indentString << "\tquantity: " << QuantityToString(quantity) << "\n";
        ss << indentString << "}";

        return ss.str();
    }
};

struct OMClientRequest
{
    u64 sequenceNumber = 0;
    MEClientRequest clientRequest;

    inline auto ToString() const -> std::string
    {
        std::stringstream ss;

        ss << "OMClientRequest {\n";
        ss << "\tsequenceNumber : " << sequenceNumber << '\n';
        ss << "\tclientRequest : " << clientRequest.ToString(1) << "\n";
        ss << "}";

        return ss.str();
    }
};

#pragma pack(pop)

using MEClientRequestQueue = SafeQueue<MEClientRequest>;

} // namespace Exchange