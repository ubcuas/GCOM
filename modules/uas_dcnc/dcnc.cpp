﻿//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QDebug>

// GCOM Includes
#include "dcnc.hpp"
#include "modules/uas_message/uas_message.hpp"
#include "modules/uas_message/request_message.hpp"
#include "modules/uas_message/system_info_message.hpp"
#include "modules/uas_message/capabilities_message.hpp"
#include "modules/uas_message/command_message.hpp"

#define PACK_PHOTO_FREQ(x) qRound(x*1e1)

//===================================================================
// Public Class Declaration
//===================================================================
DCNC::DCNC()
{
    // Set default values
    server.setMaxPendingConnections(1);
    port = 42069;
    clientConnection = nullptr;
    serverStatus = DCNCStatus::OFFLINE;
    autoResume = true;

    // Connect Signals to Slots
    connect(&server, SIGNAL(newConnection()),
            this, SLOT(handleClientConnection()));
}


// TODO Finish tearing down everything, delete sockets since they are dynamicly allocated!
DCNC::~DCNC()
{
    //first disconnect the newConnection to handleClientConnection
    disconnect(&server, SIGNAL(newConnection()),
               this, SLOT(handleClientConnection()));
    // put the status offline
    serverStatus = DCNCStatus::OFFLINE;
    clientConnection = nullptr;
    //The server will no longer listen for incoming connections.
    server.close();
}

bool DCNC::startServer(QString address, int port)
{
    // If the DCNC is currently running then stop it
    if (serverStatus != DCNCStatus::OFFLINE)
        stopServer();

    this->port = port;
    this->address = address;
    hostAddress = QHostAddress(address);
    bool startStatus = server.listen(hostAddress, port);

    if (startStatus)
        serverStatus = DCNCStatus::SEARCHING;

    return startStatus;
}

void DCNC::stopServer()
{
    cancelConnection();
    // Stop listning on the selected interfaces and update state
    server.close();
    serverStatus = DCNCStatus::OFFLINE;
}

void DCNC::cancelConnection()
{
    if (serverStatus == DCNCStatus::CONNECTED)
        serverStatus = DCNCStatus::SEARCHING;

    if (clientConnection == nullptr)
        return;

    disconnect(clientConnection, SIGNAL(readyRead()),
               this, SLOT(handleClientData()));
    disconnect(clientConnection, SIGNAL(disconnected()),
               this, SLOT(handleClientDisconnected()));

    // Closes the I/O device for the socket and calls disconnectFromHost() to
    // close the socket's connection.
    clientConnection->close();
    clientConnection->deleteLater();
    clientConnection = nullptr;
    emit droppedConnection();

    server.resumeAccepting();
}

DCNC::DCNCStatus DCNC::status()
{
    return serverStatus;
}

//send  infomation message
void DCNC::handleClientConnection()
{
    // Setup the connection socket and the data stream
    // put clientConnection to connectedState
    // return a new TCP socket
    clientConnection = server.nextPendingConnection();
    // This is important due to the fact that after a disconnection this will be in an error state.
    connectionDataStream.resetStatus();
    connectionDataStream.setDevice(clientConnection);
    // Connect the connection slot's signals
    connect(clientConnection, SIGNAL(readyRead()),
            this, SLOT(handleClientData()));
    connect(clientConnection, SIGNAL(disconnected()),
            this, SLOT(handleClientDisconnected()));

    // Update the DCNC's state and notify listners
    serverStatus = DCNCStatus::CONNECTED;

    // Pause listening for incoming connections
    server.pauseAccepting();

    // Send system info request and check that the message was successfully sent
    RequestMessage request(UASMessage::MessageID::DATA_SYSTEM_INFO);
    if(!sendUASMessage(request))
        emit droppedConnection();
    else
        emit receivedConnection();
}

// TODO Link Directly to handleClientDisconnection
void DCNC::handleClientDisconnected()
{
    cancelConnection();
}

void DCNC::changeAutoResume(bool autoResume)
{
    this->autoResume = autoResume;
}

//===================================================================
// Outgoing message methods
//===================================================================
bool DCNC::sendUASMessage(UASMessage& outgoingMessage)
{
    if (clientConnection == nullptr || clientConnection->state() != QTcpSocket::ConnectedState)
        return false;

    messageFramer.frameMessage(outgoingMessage);
    connectionDataStream << messageFramer;
    if (messageFramer.status() != UASMessageTCPFramer::TCPFramerStatus::SUCCESS)
        return false;

    return true;
}

void DCNC::startImageRelay(float photoFreq)
{
    std::vector<uint8_t> args;
    args.push_back(PACK_PHOTO_FREQ(photoFreq));

    CommandMessage outgoingMessage(CommandMessage::Commands::IMAGE_RELAY_START,
                                   args);
    sendUASMessage(outgoingMessage);
}

void DCNC::stopImageRelay()
{
    CommandMessage outgoingMessage(CommandMessage::Commands::IMAGE_RELAY_STOP);
    sendUASMessage(outgoingMessage);
}

//===================================================================
// Receive Handler
//===================================================================
void DCNC::handleClientData()
{
    messageFramer.clearMessage();
    do{
        connectionDataStream.startTransaction();
        connectionDataStream >> messageFramer;
        if (messageFramer.status() == UASMessageTCPFramer::TCPFramerStatus::SUCCESS)
        {
            handleClientMessage(messageFramer.generateMessage());
            connectionDataStream.commitTransaction();
        }
        else if (messageFramer.status() == UASMessageTCPFramer::TCPFramerStatus::INCOMPLETE_MESSAGE)
        {
            connectionDataStream.rollbackTransaction();
        }
        else
        {
            connectionDataStream.abortTransaction();
        }
    } while (messageFramer.status() != UASMessageTCPFramer::TCPFramerStatus::INCOMPLETE_MESSAGE);
    connectionDataStream.resetStatus();
}


void DCNC::handleClientMessage(std::shared_ptr<UASMessage> message)
{
    std::shared_ptr<UASMessage> outgoingMessage = nullptr;
    switch (message->type())
    {
        case UASMessage::MessageID::DATA_SYSTEM_INFO:
        {
            std::shared_ptr<SystemInfoMessage> systemInfo = std::static_pointer_cast<SystemInfoMessage>(message);
            outgoingMessage = handleInfo(systemInfo->systemId, systemInfo->dropped,autoResume);
            emit receivedGremlinInfo(QString(systemInfo->systemId.c_str()),
                                     systemInfo->versionNumber,
                                     systemInfo->dropped);
            break;
        }

        case UASMessage::MessageID::DATA_CAPABILITIES:
        {
            std::shared_ptr<CapabilitiesMessage> systemCapabilities =
                    std::static_pointer_cast<CapabilitiesMessage>(message);
            emit receivedGremlinCapabilities(systemCapabilities->capabilities);
            break;
        }

        case UASMessage::MessageID::RESPONSE:
        {
            std::shared_ptr<ResponseMessage> response =
                    std::static_pointer_cast<ResponseMessage>(message);
            handleResponse(response->command, response->response);
            emit receivedGremlinResponse(response->command, response->response);
            break;
        }

        case UASMessage::MessageID::DATA_IMAGE_UNTAGGED:
        {
            std::shared_ptr<ImageUntaggedMessage> image =
                    std::static_pointer_cast<ImageUntaggedMessage>(message);
            emit receivedImageUntaggedData(image);
            break;
        }

        case UASMessage::MessageID::DATA_IMAGE_TAGGED:
        {
            std::shared_ptr<ImageTaggedMessage> image =
                    std::static_pointer_cast<ImageTaggedMessage>(message);
            emit receivedImageTaggedData(image);
            break;
        }

        default:
            outgoingMessage = nullptr;
        break;
    }

    if (outgoingMessage == nullptr)
        return;


    if (!sendUASMessage(*outgoingMessage))
    {
        qDebug() << "Message not sent successfully";
    }
}

// TODO Relay all other responses to an external signal, so that modules that care about them can
// get them
void DCNC::handleResponse(CommandMessage::Commands command,
                          ResponseMessage::ResponseCodes responses)
{
    switch(command)
    {
        case CommandMessage::Commands::SYSTEM_RESET:
        {
            emit reestablishedConnection(CommandMessage::Commands::SYSTEM_RESET, responses);

            // Request capabilities again
            RequestMessage requestCapabilities(UASMessage::MessageID::DATA_CAPABILITIES);
            sendUASMessage(requestCapabilities);
        }
        break;

        case CommandMessage::Commands::SYSTEM_RESUME:
        {
            emit reestablishedConnection(CommandMessage::Commands::SYSTEM_RESUME, responses);

            if (responses == ResponseMessage::ResponseCodes::NO_ERROR)
                return;

            // If there is an error, cancel the connection
            serverStatus = DCNCStatus::OFFLINE;
            cancelConnection();
        }
        break;

        default:
        break;
    }
}

// TODO Handle Connection Status
std::shared_ptr<UASMessage> DCNC::handleInfo(std::string systemId, bool dropped, bool autoResume)
{
    if(preSysID.compare(systemId)==0 && dropped && autoResume)
    {
        return std::shared_ptr<UASMessage>(
                    new CommandMessage(CommandMessage::Commands::SYSTEM_RESUME));
    }
    else if(dropped && preSysID.compare(systemId)==0)
    {
        return std::shared_ptr<UASMessage>(
                    new CommandMessage(CommandMessage::Commands::SYSTEM_RESET));
    }
    else
    {
        preSysID.assign(systemId);
        return std::shared_ptr<UASMessage>(
                    new RequestMessage(UASMessage::MessageID::DATA_CAPABILITIES));
    }
}
