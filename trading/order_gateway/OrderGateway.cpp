#include "OrderGateway.h"
#include "exchange/order_server/ClientResponse.h"

#include <cstring>

namespace Trading
{
OrderGateway::OrderGateway(ClientId clientId, Exchange::MEClientRequestQueue *clientRequests,
                           Exchange::MEClientResponseQueue *clientResponses, std::string const &ip,
                           std::string const &iface, i32 port)
    : mClientId(clientId), mIp(ip), mIFace(iface), mPort(port),
      mLogger("trading_order_gateway_" + std::to_string(clientId) + ".log"), mRequests(clientRequests),
      mResponses(clientResponses), mSocket(mLogger)
{
    mSocket.recvCallback = [this](TCPSocket *socket, Nanos rxTime) { RecvCallback(socket, rxTime); };
}

OrderGateway::~OrderGateway()
{
    Stop();
}

void OrderGateway::Run()
{
    while (!mShouldStop)
    {
        mSocket.RecvAndSend();

        while (mRequests->GetSize() > 0)
        {
            auto request = mRequests->GetNextRead();

            mLogger.Log("Sending request with id: ", mNextOutgoingSequenceNumber, ": ", request->ToString(), "\n");

            mSocket.Send(&mNextOutgoingSequenceNumber, sizeof(mNextOutgoingSequenceNumber));
            mSocket.Send(&request, sizeof(Exchange::MEClientRequest));

            mRequests->UpdateReadIndex();
            mNextOutgoingSequenceNumber++;
        }
    }
}

void OrderGateway::RecvCallback(TCPSocket *socket, Nanos rxTime)
{
    if (socket->nextRecvIndex >= sizeof(Exchange::OMClientResponse))
    {
        u64 i = 0;
        for (; i + sizeof(Exchange::OMClientResponse) <= socket->nextRecvIndex; i += sizeof(Exchange::OMClientResponse))
        {
            auto response = reinterpret_cast<Exchange::OMClientResponse *>(socket->recvBuffer.data() + i);
            mLogger.Log("Received response from server: ", response->ToString(), "\n");

            if (response->clientResponse.clientId != mClientId)
            {
                mLogger.Log("Received a response for a different client\n");
                continue;
            }

            if (response->sequenceNumber != mNextExpectedSequenceNumber)
            {
                mLogger.Log("Incorrect sequence number received in response. Expecting ", mNextExpectedSequenceNumber,
                            " but received ", response->sequenceNumber, "\n");
                continue;
            }

            ++mNextExpectedSequenceNumber;

            auto nextWrite = mResponses->GetNextWriteTo();
            *nextWrite = std::move(response->clientResponse);
            mResponses->UpdateWriteIndex();
        }

        memcpy(socket->recvBuffer.data(), socket->recvBuffer.data() + i, socket->nextRecvIndex - i);
        socket->nextRecvIndex -= i;
    }
}

} // namespace Trading