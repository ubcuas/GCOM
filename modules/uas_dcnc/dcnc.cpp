//===================================================================
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


//===================================================================
// Public Class Declaration
//===================================================================
DCNC::DCNC()
{
    // Set default values
    server = new QTcpServer();
    server->setMaxPendingConnections(1);
    port = 42069;
    clientConnection = nullptr;
    serverStatus = DCNCStatus::OFFLINE;
    autoResume = true;

    // Connect Signals to Slots
    connect(server, SIGNAL(newConnection()),
            this, SLOT(handleClientConection()));
}

DCNC::~DCNC()
{
    //stopServer();
    //delete(server);
}

bool DCNC::startServer(QString address, int port)
{
    // If the DCNC is currently running then stop it
    if (serverStatus != DCNCStatus::OFFLINE)
        stopServer();

    this->port = port;
    this->address = address;
    hostAddress = QHostAddress(address);
    bool startStatus = server->listen(hostAddress, port);
    if (startStatus == true)
        serverStatus = DCNCStatus::SEARCHING;

    return startStatus;
}

void DCNC::stopServer()
{
    cancelConnection();
    // Stop listning on the selected interfaces and update state
    server->close();
    serverStatus = DCNCStatus::OFFLINE;
}

void DCNC::cancelConnection()
{
    if (serverStatus == DCNCStatus::CONNECTED)
        serverStatus = DCNCStatus::SEARCHING;

    if(clientConnection != nullptr)
    {
        disconnect(clientConnection, SIGNAL(readyRead()),
                   this, SLOT(handleClientData()));
        disconnect(clientConnection, SIGNAL(disconnected()),
                   this, SLOT(handleClientDisconnected()));
        clientConnection->close();
        clientConnection->deleteLater();
        clientConnection = nullptr;
        emit droppedConnection();
    }

    server->resumeAccepting();
}

DCNC::DCNCStatus DCNC::status()
{
    return serverStatus;
}

void DCNC::handleClientConection()
{
    // While this connection is established stop accepting more connections
    server->pauseAccepting();
    // Setup the connection socket and the data stream
    clientConnection = server->nextPendingConnection();
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

    // Send system info request
    // TODO: that the message is successfully sent
    RequestMessage request(UASMessage::MessageID::DATA_SYSTEM_INFO);
    messageFramer.frameMessage(request);
    connectionDataStream << messageFramer;

    emit receivedConnection();
}

// TODO Link Directly to handleClientDisconnection
void DCNC::handleClientDisconnected()
{
    cancelConnection();
}

void DCNC::handleClientData()
{
    messageFramer.clearMessage();
    while (messageFramer.status() != UASMessageTCPFramer::TCPFramerStatus::INCOMPLETE_MESSAGE)
    {
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
    }
}

void DCNC::changeAutoResume(bool autoResume)
{
    this->autoResume = autoResume;
}

//===================================================================
// Outgoing message methods
//===================================================================

bool DCNC::sendUASMessage(std::shared_ptr<UASMessage> outgoingMessage)
{
    if (clientConnection == nullptr || clientConnection->state() != QTcpSocket::ConnectedState)
        return false;

    messageFramer.frameMessage(*outgoingMessage);
    connectionDataStream << messageFramer;
    if (messageFramer.status() != UASMessageTCPFramer::TCPFramerStatus::SUCCESS)
        return false;

    return true;
}

void DCNC::startImageRelay()
{
    CommandMessage outgoingMessage = CommandMessage(CommandMessage::Commands::IMAGE_RELAY_START);
    messageFramer.frameMessage(outgoingMessage);
    connectionDataStream << messageFramer;
}

void DCNC::stopImageRelay()
{
    CommandMessage outgoingMessage = CommandMessage(CommandMessage::Commands::IMAGE_RELAY_STOP);
    messageFramer.frameMessage(outgoingMessage);
    connectionDataStream << messageFramer;
}

//===================================================================
// Receive Handler
//===================================================================
void DCNC::handleClientMessage(std::shared_ptr<UASMessage> message)
{
    UASMessage *outgoingMessage = nullptr;
    switch (message->type())
    {
        case UASMessage::MessageID::DATA_SYSTEM_INFO:
        {
            std::shared_ptr<SystemInfoMessage> systemInfo = std::static_pointer_cast<SystemInfoMessage>(message);
            if (systemInfo->dropped && autoResume)
                outgoingMessage = new CommandMessage(CommandMessage::Commands::SYSTEM_RESUME);
            else if (systemInfo->dropped)
                outgoingMessage = new CommandMessage(CommandMessage::Commands::SYSTEM_RESET);
            else
                outgoingMessage = new RequestMessage(UASMessage::MessageID::DATA_CAPABILITIES);
            break;
            emit receivedGremlinInfo(QString(systemInfo->systemId.c_str()),
                                     systemInfo->versionNumber,
                                     systemInfo->dropped);
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
            outgoingMessage = handleResponse(response->command, response->response);
            emit receivedGremlinResponse(response->command, response->response);
            break;
        }

        case UASMessage::MessageID::DATA_IMAGE_UNTAGGED:
        {
            std::shared_ptr<ImageUntaggedMessage> image =
                    std::static_pointer_cast<ImageUntaggedMessage>(message);
            emit receivedImageUntaggedData(image);
        }

        case UASMessage::MessageID::DATA_IMAGE_TAGGED:
        {
            std::shared_ptr<ImageTaggedMessage> image =
                    std::static_pointer_cast<ImageTaggedMessage>(message);
            emit receivedImageTaggedData(image);
        }

        default:
            outgoingMessage = nullptr;
    }

    if (outgoingMessage == nullptr)
        return;

    messageFramer.frameMessage(*outgoingMessage);
    // TODO: that the message is successfully sent
    connectionDataStream << messageFramer;
    qDebug() << ((int)messageFramer.status());
    delete outgoingMessage;
}


UASMessage* DCNC::handleResponse(CommandMessage::Commands command,
                           ResponseMessage::ResponseCodes responses)
{
    switch(command)
    {
        case CommandMessage::Commands::SYSTEM_RESET:
        case CommandMessage::Commands::SYSTEM_RESUME:
        {
            if (responses == ResponseMessage::ResponseCodes::NO_ERROR)
                return new RequestMessage(UASMessage::MessageID::DATA_CAPABILITIES);
            else
                 droppedConnection();
        }
        break;

        default:
        break;
    }

    return nullptr;
}
