#include "MEOrderBook.h"
#include "Check.h"
#include "Limits.h"
#include "Types.h"
#include "exchange/market_data/MarketUpdate.h"
#include "exchange/matcher/MEOrder.h"
#include "exchange/matcher/MatchingEngine.h"
#include "exchange/order_server/ClientResponse.h"

namespace Exchange
{
MEOrderBook::MEOrderBook(TickerId tickerId, QuickLogger *logger, MatchingEngine *matchingEngine)
    : mMatchingEngine(matchingEngine), mOrdersAtPricePool(ME_MAX_PRICE_LEVELS), mOrdersPool(ME_MAX_ORDER_IDS),
      mTickerId(tickerId), mLogger(logger)
{
    mOrdersAtPrice.fill(nullptr);
}

MEOrderBook::~MEOrderBook()
{
    mLogger->Log("Destroying orderbook for ticker", mTickerId, "\n");
    mMatchingEngine = nullptr;
    mAsksByPrice = nullptr;
    mBidsByPrice = nullptr;
    for (auto &it : mClientIdToOrderId)
    {
        it.fill(nullptr);
    }
}

void MEOrderBook::Match(ClientId clientId, TickerId tickerId, Side side, OrderId clientOrderId,
                        OrderId newMartkerOrderId, MEOrder *order, Quantity &leavesQuantity)
{
    auto fillQuantity = std::min(leavesQuantity, order->quantity);

    leavesQuantity -= fillQuantity;
    order->quantity -= fillQuantity;

    {
        /* Send a response to both clients about the filled order */
        mClientResponse.type = ClientResponseType::FILLED;
        mClientResponse.clientId = clientId;
        mClientResponse.clientOrderId = clientOrderId;
        mClientResponse.marketOrderId = newMartkerOrderId;
        mClientResponse.executed_quantity = fillQuantity;
        mClientResponse.leaves_quantity = leavesQuantity;
        mClientResponse.price = order->price;
        mClientResponse.side = side;
        mClientResponse.tickerId = tickerId;
        mMatchingEngine->SendClientResponse(&mClientResponse);

        mClientResponse.type = ClientResponseType::FILLED;
        mClientResponse.clientId = order->clientId;
        mClientResponse.clientOrderId = order->clientOrderId;
        mClientResponse.marketOrderId = order->marketOrderId;
        mClientResponse.executed_quantity = fillQuantity;
        mClientResponse.leaves_quantity = leavesQuantity;
        mClientResponse.price = order->price;
        mClientResponse.side = side;
        mClientResponse.tickerId = tickerId;
        mMatchingEngine->SendClientResponse(&mClientResponse);
    }

    {
        /* Send a response to the market */
        mMarketUpdate.type = MarketUpdateType::TRADE;
        mMarketUpdate.orderId = order->marketOrderId;
        mMarketUpdate.price = order->price;
        mMarketUpdate.priority = Priority_INVALID;
        mMarketUpdate.side = side;
        mMarketUpdate.tickerId = tickerId;
        mMarketUpdate.quantity = fillQuantity;
        mMatchingEngine->SendMarketUpdate(&mMarketUpdate);
    }

    if (order->quantity == 0)
    {
        RemoveOrder(order);

        /* The order had been fully executed => send a market update and remove it */
        mMarketUpdate.type = MarketUpdateType::CANCEL;
        mMarketUpdate.orderId = order->marketOrderId;
        mMarketUpdate.price = order->price;
        mMarketUpdate.priority = Priority_INVALID;
        mMarketUpdate.side = side;
        mMarketUpdate.tickerId = tickerId;
        mMarketUpdate.quantity = fillQuantity;
        mMatchingEngine->SendMarketUpdate(&mMarketUpdate);
    }
    else
    {
        /* The order hadn't been fully executed => send a modify response */
        mMarketUpdate.type = MarketUpdateType::MODIFY;
        mMarketUpdate.orderId = order->marketOrderId;
        mMarketUpdate.price = order->price;
        mMarketUpdate.priority = Priority_INVALID;
        mMarketUpdate.side = side;
        mMarketUpdate.tickerId = tickerId;
        mMarketUpdate.quantity = order->quantity;
        mMatchingEngine->SendMarketUpdate(&mMarketUpdate);
    }
}

Quantity MEOrderBook::CheckForMatch(ClientId clientId, OrderId clientOrderId, TickerId tickerId, Side side, Price price,
                                    Quantity qty, OrderId marketOrderId)
{
    auto leavesQuantity = qty;

    if (side == Side::BUY)
    {
        while (leavesQuantity != 0 && mAsksByPrice != nullptr)
        {
            auto firstOrder = mAsksByPrice->firstOrder;
            if (price < firstOrder->price)
            {
                break;
            }
        }
    }
    else if (side == Side::SELL)
    {
        while (leavesQuantity != 0 && mBidsByPrice != nullptr)
        {
            auto firstOrder = mBidsByPrice->firstOrder;
            if (price > firstOrder->price)
            {
                break;
            }
        }
    }

    return leavesQuantity;
}

Priority MEOrderBook::GetNextPriority(Price price)
{
    auto ordersAtPrice = GetOrdersAtPrice(price);
    if (ordersAtPrice == nullptr)
    {
        return 1;
    }
    return ordersAtPrice->firstOrder->prevOrder->priority + 1;
}

void MEOrderBook::AddOrdersAtPrice(MEOrdersAtPrice *ordersAtPrice)
{
    auto priceIndex = PriceToIndex(ordersAtPrice->price);
    CHECK_FATAL(mOrdersAtPrice[priceIndex] == nullptr, "Cannot add order at a price that is already used");

    mOrdersAtPrice[priceIndex] = ordersAtPrice;

    auto headOfList = ordersAtPrice->side == Side::BUY ? mBidsByPrice : mAsksByPrice;
    if (headOfList == nullptr) [[unlikely]]
    {
        (ordersAtPrice->side == Side::BUY ? mBidsByPrice : mAsksByPrice) = ordersAtPrice;
        ordersAtPrice->prevEntry = ordersAtPrice->nextEntry = ordersAtPrice;
    }
    else
    {
        auto target = headOfList;
        bool found = false;
        do
        {
            bool shouldInsert = false;
            if (ordersAtPrice->side == Side::SELL)
            {
                shouldInsert = (ordersAtPrice->price > target->price);
            }
            else
            {
                shouldInsert = (ordersAtPrice->price < target->price);
            }

            if (shouldInsert)
            {
                auto prevEntry = target->prevEntry;

                ordersAtPrice->prevEntry = prevEntry;
                ordersAtPrice->nextEntry = target;

                prevEntry->nextEntry = ordersAtPrice;
                target->prevEntry = ordersAtPrice;

                if (target == headOfList)
                {
                    /* Update the head of the list */
                    (ordersAtPrice->side == Side::BUY ? mBidsByPrice : mAsksByPrice) = ordersAtPrice;
                }

                found = true;
            }

            target = target->nextEntry;

        } while (target != headOfList && !found);

        if (!found)
        {
            /* If a suitable position could not be found => put it at the back of the list */
            auto prevEntry = headOfList->prevEntry;
            ordersAtPrice->prevEntry = prevEntry;
            ordersAtPrice->nextEntry = headOfList;

            prevEntry->nextEntry = ordersAtPrice;
            headOfList->prevEntry = ordersAtPrice;
        }
    }
}

void MEOrderBook::AddOrder(MEOrder *order)
{
    auto ordersAtPrice = GetOrdersAtPrice(order->price);
    if (ordersAtPrice == nullptr)
    {
        order->nextOrder = nullptr;
        order->prevOrder = nullptr;

        auto newOrdersAtPrice = mOrdersAtPricePool.Allocate(order->side, order->price, order, nullptr, nullptr);
        AddOrdersAtPrice(newOrdersAtPrice);
    }
    else
    {
        auto firstOrder = ordersAtPrice->firstOrder;
        firstOrder->prevOrder->nextOrder = order;
        order->prevOrder = firstOrder->prevOrder;
        order->nextOrder = firstOrder;
        firstOrder->prevOrder = order;
    }

    mClientIdToOrderId[order->clientId][order->clientOrderId] = order;
}

void MEOrderBook::Add(ClientId clientId, OrderId clientOrderId, TickerId tickerId, Side side, Price price, Quantity qty)
{
    auto newMarketOrderId = GenerateNewMarketOrderId();

    {
        /* Generate response to accept order */
        mClientResponse.type = ClientResponseType::ACCEPTED;
        mClientResponse.clientId = clientId;
        mClientResponse.clientOrderId = clientOrderId;
        mClientResponse.marketOrderId = newMarketOrderId;
        mClientResponse.price = price;
        mClientResponse.side = side;
        mClientResponse.tickerId = tickerId;
        mClientResponse.leaves_quantity = qty;
        mClientResponse.executed_quantity = 0;

        mMatchingEngine->SendClientResponse(&mClientResponse);
    }

    Quantity leftQuantity = CheckForMatch(clientId, clientOrderId, tickerId, side, price, qty, newMarketOrderId);

    if (leftQuantity) [[likely]]
    {
        Priority priority = GetNextPriority(price);

        auto order = mOrdersPool.Allocate(tickerId, clientId, clientOrderId, newMarketOrderId, side, price,
                                          leftQuantity, priority, nullptr, nullptr);
        AddOrder(order);

        SHOWINFO("Orderbook now: ", mBidsByPrice->ToString());

        {
            /* Generate response for market */
            mMarketUpdate.orderId = newMarketOrderId;
            mMarketUpdate.price = price;
            mMarketUpdate.priority = priority;
            mMarketUpdate.side = side;
            mMarketUpdate.tickerId = tickerId;
            mMarketUpdate.type = MarketUpdateType::ADD;
            mMarketUpdate.quantity = leftQuantity;

            mMatchingEngine->SendMarketUpdate(&mMarketUpdate);
        }
    }
}

void MEOrderBook::RemoveOrdersAtPrice(Side side, Price price)
{
    auto headOfList = (side == Side::BUY ? mBidsByPrice : mAsksByPrice);
    auto ordersAtPrice = GetOrdersAtPrice(price);

    if (ordersAtPrice->nextEntry == ordersAtPrice) [[unlikely]]
    {
        (side == Side::BUY ? mBidsByPrice : mAsksByPrice) = nullptr;
    }
    else
    {
        const auto beforeEntry = ordersAtPrice->prevEntry;
        const auto nextEntry = ordersAtPrice->nextEntry;

        beforeEntry->nextEntry = nextEntry;
        nextEntry->prevEntry = beforeEntry;

        if (ordersAtPrice == headOfList)
        {
            (side == Side::BUY ? mBidsByPrice : mAsksByPrice) = ordersAtPrice->nextEntry;
        }

        ordersAtPrice->nextEntry = nullptr;
        ordersAtPrice->prevEntry = nullptr;
    }
    mOrdersAtPrice[PriceToIndex(price)] = nullptr;
    mOrdersAtPricePool.Deallocate(ordersAtPrice);
}

void MEOrderBook::RemoveOrder(MEOrder *order)
{
    auto ordersAtPrice = GetOrdersAtPrice(order->price);
    if (ordersAtPrice->nextEntry == ordersAtPrice)
    {
        /* This means there's only one order at this price => Remove all orders at price */
        RemoveOrdersAtPrice(order->side, order->price);
    }
    else
    {
        const auto beforeOrder = order->prevOrder;
        const auto nextOrder = order->nextOrder;

        beforeOrder->nextOrder = nextOrder;
        nextOrder->prevOrder = beforeOrder;

        if (ordersAtPrice->firstOrder == order)
        {
            ordersAtPrice->firstOrder = nextOrder;
        }

        order->prevOrder = nullptr;
        order->nextOrder = nullptr;
    }

    mClientIdToOrderId[order->clientId][order->clientOrderId] = nullptr;
    mOrdersPool.Deallocate(order);
}

void MEOrderBook::Cancel(ClientId clientId, OrderId clientOrderId, TickerId tickerId)
{
    bool isCancellable = clientId < mClientIdToOrderId.size();
    MEOrder *exchangeOrder = nullptr;
    if (isCancellable) [[likely]]
    {
        auto &order = mClientIdToOrderId[clientId];
        exchangeOrder = order.at(clientOrderId);
        isCancellable = (exchangeOrder != nullptr);
    }

    if (!isCancellable) [[unlikely]]
    {
        /* We need to send the client a response that we couldn't perform the action */
        mClientResponse.type = ClientResponseType::CANCEL_REJECTED;
        mClientResponse.clientId = clientId;
        mClientResponse.clientOrderId = clientOrderId;
        mClientResponse.tickerId = tickerId;
        mClientResponse.marketOrderId = OrderId_INVALID;
        mClientResponse.executed_quantity = Quantity_INVALID;
        mClientResponse.leaves_quantity = Quantity_INVALID;
        mClientResponse.price = Price_INVALID;
        mClientResponse.side = Side::INVALID;
    }
    else
    {
        RemoveOrder(exchangeOrder);

        /* We need to send the client a response that we managed to cancel the order
           Also we need to notify the market */
        {
            mClientResponse.type = ClientResponseType::CANCELED;
            mClientResponse.clientId = clientId;
            mClientResponse.clientOrderId = clientOrderId;
            mClientResponse.tickerId = tickerId;
            mClientResponse.marketOrderId = exchangeOrder->marketOrderId;
            mClientResponse.executed_quantity = Quantity_INVALID;
            mClientResponse.leaves_quantity = Quantity_INVALID;
            mClientResponse.price = exchangeOrder->price;
            mClientResponse.side = exchangeOrder->side;
        }

        {
            mMarketUpdate.orderId = exchangeOrder->marketOrderId;
            mMarketUpdate.price = exchangeOrder->price;
            mMarketUpdate.priority = exchangeOrder->priority;
            mMarketUpdate.side = exchangeOrder->side;
            mMarketUpdate.quantity = exchangeOrder->quantity;
            mMarketUpdate.tickerId = tickerId;
            mMarketUpdate.type = MarketUpdateType::CANCEL;

            mMatchingEngine->SendMarketUpdate(&mMarketUpdate);
        }
    }

    mMatchingEngine->SendClientResponse(&mClientResponse);
}

} // namespace Exchange