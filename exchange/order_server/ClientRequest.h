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

    inline auto toString() const -> std::string
    {
        std::stringstream ss;
        ss << "MEClientRequest {\n";
        ss << "\ttype: " << ClientRequestTypeToString(type) << "\n";
        ss << "\tclientId: " << ClientIdToString(clientId) << "\n";
        ss << "\ttickerId: " << TickerIdToString(tickerId) << "\n";
        ss << "\torderId: " << OrderIdToString(orderId) << "\n";
        ss << "\tside: " << SideToString(side) << "\n";
        ss << "\tprice: " << PriceToString(price) << "\n";
        ss << "\tquantity: " << QuantityToString(quantity) << "\n";
        ss << "}";

        return ss.str();
    }
};

#pragma pack(pop)

using MEClientRequestQueue = SafeQueue<MEClientRequest>;

} // namespace Exchange