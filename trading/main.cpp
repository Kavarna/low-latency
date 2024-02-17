#include "Check.h"
#include "Limits.h"
#include "Logger.h"
#include "MarketUpdate.h"
#include "Types.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include "trading/market_data/MarketDataConsumer.h"
#include "trading/order_gateway/OrderGateway.h"
#include "trading/strategy/MarketOrderBook.h"
#include "trading/strategy/RiskManager.h"
#include "trading/strategy/TradeEngine.h"
#include <chrono>
#include <cstdlib>
#include <thread>

int main(i32 argc, char **argv)
{
    CHECK_FATAL(argc >= 3, "USAGE: ", argv[0],
                " client_id algo_type [CLIP THRESHOLD MAX_ORDER_SIZE MAX_POS_1 MAX_LOSS_1]");

    ClientId clientId = atoi(argv[1]);
    srand(clientId);

    auto algoType = StringToAlgorithmType(argv[2]);
    CHECK_FATAL(algoType != AlgorithmType::INVALID, "Invalid algorithm type");

    QuickLogger logger("trading_main_" + std::to_string(clientId) + ".log");

    Exchange::MEClientRequestQueue clientRequests(ME_MAX_CLIENT_UPDATES);
    Exchange::MEClientResponseQueue clientResponses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateQueue marketUpdates(ME_MAX_CLIENT_UPDATES);

    Trading::TradeEngineConfigHashMap tickerConfig;

    u32 nextTickerId = 0;
    for (i32 i = 0; i < argc; i += 5)
    {
        tickerConfig[nextTickerId].clip = atoi(argv[i]);
        tickerConfig[nextTickerId].threshold = atof(argv[i + 1]);

        tickerConfig[nextTickerId].riskConfig.maxOrderSize = atoi(argv[i + 2]);
        tickerConfig[nextTickerId].riskConfig.maxPositions = atoi(argv[i + 3]);
        tickerConfig[nextTickerId].riskConfig.maxLoss = atoi(argv[i + 4]);
    }

    logger.Log("Starting trade engine\n");
    Trading::TradeEngine *tradeEngine =
        new Trading::TradeEngine(clientId, algoType, tickerConfig, &clientRequests, &clientResponses, &marketUpdates);

    tradeEngine->Start();

    const std::string orderGatewayIp = "127.0.0.1";
    const std::string orderGatewayIface = "lo";
    const i32 orderGatewayPort = 12345;

    logger.Log("Starting OrderGateWay\n");
    Trading::OrderGateway *orderGateway = new Trading::OrderGateway(
        clientId, &clientRequests, &clientResponses, orderGatewayIp, orderGatewayIface, orderGatewayPort);
    orderGateway->Start();

    const std::string marketPublisherIp = "127.0.0.1";
    const std::string marketPublisherIface = "lo";
    const i32 marketPublisherPortIncremental = 6942;
    const i32 marketPublisherPortSnapshot = 4269;

    logger.Log("Starting market data consumer\n");
    Trading::MarketDataConsumer *marketDataConsumer =
        new Trading::MarketDataConsumer(clientId, &marketUpdates, marketPublisherIface, marketPublisherIp,
                                        marketPublisherPortSnapshot, marketPublisherIp, marketPublisherPortIncremental);
    marketDataConsumer->Start();

    tradeEngine->InitLastEventTime();

    if (algoType == AlgorithmType::RANDOM)
    {
        OrderId orderId = clientId * 1000;
        std::vector<Exchange::MEClientRequest> randomRequests;
        std::array<Price, ME_MAX_TICKERS> tickerBasePrice;
        for (u32 i = 0; i < ME_MAX_TICKERS; ++i)
        {
            tickerBasePrice[i] = rand() % 100 + 100;
        }

        for (i32 i = 0; i < 10000; ++i)
        {
            TickerId tickerId = rand() % ME_MAX_TICKERS;
            Price price = tickerBasePrice[tickerId] + (rand() % 10) + 1;
            Quantity qty = 1 + (rand() % 100) + 1;
            Side side = rand() % 2 ? Side::BUY : Side::SELL;

            Exchange::MEClientRequest request;
            {
                request.type = Exchange::ClientRequestType::NEW;
                request.clientId = clientId;
                request.orderId = orderId++;
                request.side = side;
                request.price = price;
                request.quantity = qty;
            }
            tradeEngine->SendClientRequest(&request);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            randomRequests.push_back(request);

            auto indexToCancel = rand() % randomRequests.size();
            auto toCancelRequest = randomRequests[indexToCancel];
            toCancelRequest.type = Exchange::ClientRequestType::CANCEL;

            tradeEngine->SendClientRequest(&toCancelRequest);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (tradeEngine->SilentSeconds() >= 60)
            {
                logger.Log("Stopping random client early because of ", tradeEngine->SilentSeconds(),
                           " seconds of inactivity in the market\n");
                break;
            }
        }
    }

    while (tradeEngine->SilentSeconds() < 60)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    tradeEngine->Stop();
    marketDataConsumer->Stop();
    orderGateway->Stop();

    delete tradeEngine;
    delete marketDataConsumer;
    delete orderGateway;

    exit(EXIT_SUCCESS);
}
