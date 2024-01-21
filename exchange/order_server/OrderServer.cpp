#include "OrderServer.h"
#include "Check.h"
#include "Logger.h"
#include "TCPServer.h"
#include "ThreadUtils.h"
#include "exchange/order_server/ClientRequest.h"
#include "exchange/order_server/ClientResponse.h"
#include "exchange/order_server/FIFOSequencer.h"

#include <cstring>

namespace Exchange
{
OrderServer::OrderServer(MEClientRequestQueue *clientRequests, MEClientResponseQueue *clientResponses,
                         std::string const &iface, i32 port)
    : mIFace(iface), mPort(port), mClientResponses(clientResponses), mLogger("order_server.log"), mTCPServer(mLogger),
      mSequencer(clientRequests, &mLogger)
{
    mClientIdToNextResponseSequenceNumber.fill(1);
    mClientIdToNextRequestSequenceNumber.fill(1);
    mClientIdToSocket.fill(nullptr);

    mTCPServer.recvCallback = [this](auto socket, auto rxTime) { RecvCallback(socket, rxTime); };
    mTCPServer.recvFinishedCallback = [this]() { RecvFinishCallback(); };
}

OrderServer::~OrderServer()
{
    Stop();
}

void OrderServer::Start()
{
    mShouldStop = false;
    mTCPServer.Listen(mIFace, mPort);

    mRunningThread = CreateAndStartThread(-1, "Exchange/OrderServer", [this]() { Run(); });
    CHECK_FATAL(mRunningThread != nullptr, "Couldn't start order server thread");
}

void OrderServer::Run()
{
    while (!mShouldStop)
    {
        /* Poll the server and process sockets */
        mTCPServer.Poll();
        mTCPServer.RecvAndSend();

        while (mClientResponses->GetSize() != 0)
        {
            auto clientResponse = mClientResponses->GetNextRead();

            auto &nextOutgoingSeqNum = mClientIdToNextResponseSequenceNumber[clientResponse->clientId];
            mLogger.Log("Sending response ", clientResponse->ToString(), " with sequence number ", nextOutgoingSeqNum,
                        "\n");

            /* Get the queue and send the data */
            CHECK_FATAL(mClientIdToSocket[clientResponse->clientId] != nullptr, "Can't send response to a null socket");
            auto &socket = mClientIdToSocket[clientResponse->clientId];
            socket->Send(&nextOutgoingSeqNum, sizeof(nextOutgoingSeqNum));
            socket->Send(clientResponse, sizeof(*clientResponse));

            /* Advance to the next response */
            mClientResponses->UpdateReadIndex();
            nextOutgoingSeqNum++;
        }
    }
}

void OrderServer::Stop()
{
    mShouldStop = true;
    mRunningThread->join();
}

void OrderServer::RecvCallback(TCPSocket *socket, Nanos rxTime)
{
    mLogger.Log("Receiving socket: ", socket->socket, "; length: ", socket->nextRecvIndex, "; rxTime: ", rxTime, "\n");
    if (socket->nextRecvIndex >= sizeof(OMClientRequest))
    {
        u64 i = 0;
        for (; i + sizeof(OMClientRequest) <= socket->nextRecvIndex; i += sizeof(OMClientRequest))
        {
            auto request = reinterpret_cast<OMClientRequest *>(socket->recvBuffer.data() + i);
            mLogger.Log("Received request: ", request->ToString(), "\n");

            if (mClientIdToSocket[request->clientRequest.clientId] == nullptr) [[unlikely]]
            {
                /* First time we see this client => save it's client id */
                mClientIdToSocket[request->clientRequest.clientId] = socket;
            }

            if (mClientIdToSocket[request->clientRequest.clientId] != socket) [[unlikely]]
            {
                /* Client id is different than what was expected */
                mLogger.Log("This request is invalid as the client id does not match the expected client id \n");
                continue;
            }

            auto &expectedSequenceNumber = mClientIdToNextRequestSequenceNumber[request->clientRequest.clientId];
            if (expectedSequenceNumber != request->sequenceNumber)
            {
                mLogger.Log("This request is invalid as sequence number does not match the expected number (",
                            expectedSequenceNumber, " != ", request->sequenceNumber, ")\n");
                continue;
            }
            ++expectedSequenceNumber;

            mSequencer.AddClientRequest(rxTime, request->clientRequest);
        }
        memcpy(socket->recvBuffer.data(), socket->recvBuffer.data() + i, socket->nextRecvIndex - i);
        socket->nextRecvIndex -= i;
    }
}

void OrderServer::RecvFinishCallback()
{
    mSequencer.SequenceAndPublish();
}

} // namespace Exchange