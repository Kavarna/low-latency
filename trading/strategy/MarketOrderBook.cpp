#include "MarketOrderBook.h"
#include "Limits.h"
#include "MarketUpdate.h"
#include "TradeEngine.h"
#include "Types.h"

namespace Trading
{
MarketOrderBook::MarketOrderBook(TickerId tickerId, QuickLogger *logger)
    : mTickerId(tickerId), mOrdersAtPricePool(ME_MAX_PRICE_LEVELS), mMartketOrdersPool(ME_MAX_ORDER_IDS),
      mLogger(logger)
{
}

MarketOrderBook::~MarketOrderBook()
{
    mTradeEngine = nullptr;
    mBidsByPrice = nullptr;
    mAsksByPrice = nullptr;
    mOrderIdToOrder.fill(nullptr);
}

void MarketOrderBook::AddOrdersAtPrice(MarketOrdersAtPrice *ordersAtPrice)
{
    auto priceIndex = PriceToIndex(ordersAtPrice->price);
    CHECK_FATAL(mPriceOrdersAtPrice[priceIndex] == nullptr, "Cannot add order at a price that is already used");

    mPriceOrdersAtPrice[priceIndex] = ordersAtPrice;

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

void MarketOrderBook::AddOrder(MarketOrder *order)
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
        auto firstOrder = ordersAtPrice->firstMarketOrder;
        firstOrder->prevOrder->nextOrder = order;
        order->prevOrder = firstOrder->prevOrder;
        order->nextOrder = firstOrder;
        firstOrder->prevOrder = order;
    }
    mOrderIdToOrder[order->orderId] = order;
}

void MarketOrderBook::RemoveOrdersAtPrice(Side side, Price price)
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
    mPriceOrdersAtPrice[PriceToIndex(price)] = nullptr;
    mOrdersAtPricePool.Deallocate(ordersAtPrice);
}

void MarketOrderBook::RemoveOrder(MarketOrder *order)
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

        if (ordersAtPrice->firstMarketOrder == order)
        {
            ordersAtPrice->firstMarketOrder = nextOrder;
        }

        order->prevOrder = nullptr;
        order->nextOrder = nullptr;
    }

    mOrderIdToOrder[order->orderId] = nullptr;
    mMartketOrdersPool.Deallocate(order);
}

void MarketOrderBook::UpdateBestBidOffer(bool bidUpdated, bool askUpdated)
{
    if (bidUpdated)
    {
        if (mBidsByPrice)
        {
            mBestBidOffer.bidPrice = mBidsByPrice->price;
            mBestBidOffer.bidQuantity = mBidsByPrice->firstMarketOrder->quantity;
            for (auto order = mBidsByPrice->firstMarketOrder->nextOrder; order != mBidsByPrice->firstMarketOrder;
                 order = order->nextOrder)
            {
                mBestBidOffer.bidQuantity += order->quantity;
            }
        }
        else
        {
            mBestBidOffer.bidPrice = Price_INVALID;
            mBestBidOffer.bidQuantity = Quantity_INVALID;
        }
    }

    if (askUpdated)
    {
        if (mAsksByPrice)
        {
            mBestBidOffer.askPrice = mAsksByPrice->price;
            mBestBidOffer.askQuantity = mAsksByPrice->firstMarketOrder->quantity;
            for (auto order = mAsksByPrice->firstMarketOrder->nextOrder; order != mAsksByPrice->firstMarketOrder;
                 order = order->nextOrder)
            {
                mBestBidOffer.askQuantity += order->quantity;
            }
        }
        else
        {
            mBestBidOffer.askPrice = Price_INVALID;
            mBestBidOffer.askQuantity = Quantity_INVALID;
        }
    }
}

void MarketOrderBook::OnMarketUpdate(Exchange::MEMarketUpdate *marketUpdate)
{
    bool bidUpdated = mBidsByPrice && marketUpdate->side == Side::BUY && marketUpdate->price >= mBidsByPrice->price;
    bool askUpdated = mAsksByPrice && marketUpdate->side == Side::SELL && marketUpdate->price <= mAsksByPrice->price;
    switch (marketUpdate->type)
    {
    case Exchange::MarketUpdateType::ADD: {
        auto order = mMartketOrdersPool.Allocate(marketUpdate->orderId, marketUpdate->side, marketUpdate->price,
                                                 marketUpdate->quantity, marketUpdate->priority, nullptr, nullptr);
        AddOrder(order);
        break;
    }
    case Exchange::MarketUpdateType::MODIFY: {
        auto order = mOrderIdToOrder[marketUpdate->orderId];
        order->quantity = marketUpdate->quantity;
        break;
    }
    case Exchange::MarketUpdateType::CANCEL: {
        auto order = mOrderIdToOrder[marketUpdate->orderId];
        RemoveOrder(order);
        break;
    }
    case Exchange::MarketUpdateType::TRADE: {
        mTradeEngine->OnTradeUpdate(marketUpdate, this);
        break;
    }
    case Exchange::MarketUpdateType::CLEAR: {
        for (auto &order : mOrderIdToOrder)
        {
            if (order)
            {
                mMartketOrdersPool.Deallocate(order);
            }
        }
        mOrderIdToOrder.fill(nullptr);

        if (mBidsByPrice)
        {
            for (auto bid = mBidsByPrice->nextEntry; bid != mBidsByPrice; bid = bid->nextEntry)
            {
                mOrdersAtPricePool.Deallocate(bid);
            }
            mOrdersAtPricePool.Deallocate(mBidsByPrice);
            mBidsByPrice = nullptr;
        }

        if (mAsksByPrice)
        {
            for (auto ask = mAsksByPrice->nextEntry; ask != mAsksByPrice; ask = ask->nextEntry)
            {
                mOrdersAtPricePool.Deallocate(ask);
            }
            mOrdersAtPricePool.Deallocate(mAsksByPrice);
            mAsksByPrice = nullptr;
        }

        break;
    }
    case Exchange::MarketUpdateType::INVALID:
    case Exchange::MarketUpdateType::SNAPSHOT_START:
    case Exchange::MarketUpdateType::SNAPSHOT_END:
    default:
        break;
    }

    UpdateBestBidOffer(bidUpdated, askUpdated);

    mLogger->Log("MarketOrderBook::OnMarketUpdate: ", marketUpdate->ToString(), "\n");

    mTradeEngine->OnOrderBookUpdate(marketUpdate->tickerId, marketUpdate->price, marketUpdate->side, this);
}

} // namespace Trading