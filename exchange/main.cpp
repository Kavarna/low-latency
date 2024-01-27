#include "Check.h"
#include "Limits.h"
#include "Logger.h"
#include "exchange/market_data/MarketDataPublisher.h"
#include "exchange/matcher/MEOrderBook.h"
#include "exchange/matcher/MatchingEngine.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include "exchange/order_server/OrderServer.h"
#include <csignal>
#include <cstdlib>

QuickLogger *gLogger = nullptr;
Exchange::MatchingEngine *gMatchingEngine = nullptr;
Exchange::MarketDataPublisher *gMarketDataPublisher = nullptr;
Exchange::OrderServer *gOrderServer;

bool gStartDestroying = false;

extern "C" void InterruptHandler(int signal)
{
    if (gStartDestroying)
        return;

    gStartDestroying = true;

    SHOWINFO("Signal received: ", signal);

    SHOWINFO("Start deleting the market data publisher");
    if (gMarketDataPublisher)
    {
        delete gMarketDataPublisher;
        gMarketDataPublisher = nullptr;
    }
    SHOWINFO("Deleted the market data publisher");

    SHOWINFO("Start deleting the order server");
    if (gOrderServer)
    {
        delete gOrderServer;
        gOrderServer = nullptr;
    }
    SHOWINFO("Deleted the order server");

    SHOWINFO("Start deleting the matching engine");
    if (gMatchingEngine)
    {
        delete gMatchingEngine;
        gMatchingEngine = nullptr;
    }
    SHOWINFO("Deleted the matching engine");

    SHOWINFO("Start deleting the logger");
    if (gLogger)
    {
        delete gLogger;
        gLogger = nullptr;
    }
    SHOWINFO("Deleted the logger");

    exit(EXIT_SUCCESS);
}

int main()
{
    signal(SIGINT, InterruptHandler);
    signal(SIGABRT, InterruptHandler);

    Exchange::MEClientRequestQueue clientRequests(ME_MAX_CLIENT_UPDATES);
    Exchange::MEClientResponseQueue clientResponses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateQueue marketUpdates(ME_MAX_MARKET_UPDATES);
    gLogger = new QuickLogger("exchange.logs");

    gLogger->Log("Starting the matching engine\n");
    gMatchingEngine = new Exchange::MatchingEngine(&clientRequests, &clientResponses, &marketUpdates);
    gMatchingEngine->Start();

    const std::string marketDataPublisherIface = "lo";
    const std::string snapshotPublicIp = "233.252.14.1", incrementalPublicIp = "233.252.14.3";
    const i32 snapshotPublicPort = 20000, incrementalPublicPort = 20001;
    gLogger->Log("Starting the market data publisher\n");
    gMarketDataPublisher =
        new Exchange::MarketDataPublisher(&marketUpdates, marketDataPublisherIface, snapshotPublicIp,
                                          snapshotPublicPort, incrementalPublicIp, incrementalPublicPort);
    gMarketDataPublisher->Start();

    const std::string orderServerIface = "lo";
    const int orderServerPort = 12345;
    gLogger->Log("Starting the order server\n");
    gOrderServer = new Exchange::OrderServer(&clientRequests, &clientResponses, orderServerIface, orderServerPort);
    gOrderServer->Start();

    while (true)
    {
        gLogger->Log("Sleeping for a few seconds . . .\n");
        std::this_thread::sleep_for(std::chrono::seconds(100));
    }
}
