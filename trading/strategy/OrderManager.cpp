#include "OrderManager.h"
#include "RiskManager.h"
#include "Types.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/strategy/OMOrder.h"
#include "trading/strategy/TradeEngine.h"

namespace Trading
{
void OrderManager::NewOrder(OMOrder *order, TickerId tickerId, Price price, Side side, Quantity quantity)
{
    Exchange::MEClientRequest request;
    {
        request.clientId = 0; /* TODO: Fill client id */
        request.orderId = mNextOrderId;
        request.type = Exchange::ClientRequestType::NEW;
        request.tickerId = tickerId;
        request.price = price;
        request.side = side;
        request.quantity = quantity;
    }
    mTradeEngine->SendClientRequest(&request);

    *order = {tickerId, mNextOrderId, side, price, quantity, OMOrderState::PENDING_NEW};
    ++mNextOrderId;

    mLogger->Log("OrderManager::NewOrder: ", order->ToString(), "\n");
}

void OrderManager::CancelOrder(OMOrder *order)
{
    Exchange::MEClientRequest request;
    {
        request.clientId = 0; /* TODO: Fill client id */
        request.orderId = order->orderId;
        request.type = Exchange::ClientRequestType::CANCEL;
        request.tickerId = order->tickerId;
        request.price = order->price;
        request.side = order->side;
        request.quantity = order->quantity;
    }
    mTradeEngine->SendClientRequest(&request);

    order->state = OMOrderState::PENDING_CANCEL;
    mLogger->Log("OrderManager::CancelOrder: ", order->ToString(), "\n");
}

void OrderManager::OnOrderUpdate(Exchange::MEClientResponse *clientResponse)
{
    /* Get the order */
    auto order = &mTickerSideOrder[clientResponse->tickerId][SideToIndex(clientResponse->side)];
    mLogger->Log("OrderManager::OnOrderUpdate: Order: ", order->ToString(), "\n");
    switch (clientResponse->type)
    {
    case Exchange::ClientResponseType::ACCEPTED: {
        order->state = OMOrderState::LIVE;
        break;
    }
    case Exchange::ClientResponseType::CANCELED: {
        order->state = OMOrderState::DEAD;
    }
    case Exchange::ClientResponseType::FILLED: {
        order->quantity = clientResponse->leaves_quantity;
        if (order->quantity == 0)
        {
            order->state = OMOrderState::DEAD;
        }
    }
    case Exchange::ClientResponseType::CANCEL_REJECTED:
    case Exchange::ClientResponseType::INVALID: {
        break;
    }
    }
}

void OrderManager::MoveOrder(OMOrder *order, TickerId tickerId, Price price, Side side, Quantity qty)
{
    switch (order->state)
    {
    case Trading::OMOrderState::LIVE: {
        if (order->price != price || order->quantity != qty)
        {
            CancelOrder(order);
        }
        break;
    }
    case Trading::OMOrderState::INVALID:
    case Trading::OMOrderState::DEAD: {
        if (price != Price_INVALID) [[likely]]
        {
            if (mRiskManager.CheckPreTradeRisk(tickerId, side, qty) == RiskCheckResult::ALLOWED) [[likely]]
            {
                NewOrder(order, tickerId, price, side, qty);
            }
            else
            {
                mLogger->Log("The risk manager didn't allow order with ticker: ", TickerIdToString(tickerId),
                             "; price: ", PriceToString(price), "; side: ", SideToString(side),
                             "; quantity: ", QuantityToString(qty), "\n");
            }
        }
        break;
    }
    case Trading::OMOrderState::PENDING_NEW:
    case Trading::OMOrderState::PENDING_CANCEL: {
        break;
    }
    }
}

} // namespace Trading