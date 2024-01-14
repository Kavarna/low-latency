#include "Check.h"
#include "Limits.h"
#include "Logger.h"
#include "Types.h"
#include "exchange/market_data/MarketUpdate.h"
#include "exchange/matcher/MEOrder.h"
#include "exchange/matcher/MEOrderBook.h"
#include "exchange/matcher/MatchingEngine.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <thread>

QuickLogger *gLogger = nullptr;
Exchange::MatchingEngine *gMatchingEngine = nullptr;

extern "C" void InterruptHandler(int signal)
{
    SHOWINFO("Semnal received: ", signal);

    SHOWINFO("Start deleting the matching engine");
    if (gMatchingEngine)
    {
        delete gMatchingEngine;
        gMatchingEngine = nullptr;
    }

    SHOWINFO("Deleted the matching engine");

    SHOWINFO("Start deleting the logger");
    std::this_thread::sleep_for(std::chrono::seconds(10));
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
    gMatchingEngine = new Exchange::MatchingEngine(&clientRequests, &clientResponses, &marketUpdates);

    gMatchingEngine->Start();

    while (true)
    {
        gLogger->Log("Sleeping for a few milliseconds . . .\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
