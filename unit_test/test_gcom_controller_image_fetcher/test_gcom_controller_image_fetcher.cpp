//===================================================================
// Includes
//===================================================================

// System Includes
#include <QString>
#include <QSignalSpy>
#include <QtTest/QtTest>
// GCOM Includes
#include "test_gcom_controller_image_fetcher.hpp"
#include "ui_gcomcontroller.h"

const QString IP_ADDRESS = "127.0.0.1";
const int PORT = 4206;

const int SOCKET_TIMEOUT_DURATION = 5000;

const QString DISCONNECT_LABEL("<font color='#D52D2D'> DISCONNECTED </font>"
                               "<img src=':/connection/disconnected.png'>");
const QString CONNECTING_LABEL("<font color='#EED202'> CONNECTING </font>"
                               "<img src=':/connection/connecting.png'>");
const QString CONNECTED_LABEL("<font color='#05c400'> CONNECTED </font>"
                               "<img src=':/connection/connected.png'>");
const QString SEARCHING_LABEL("<font color='#EED202'> SEARCHING </font>"
                               "<img src=':/connection/connecting.png'>");

QTEST_MAIN(TestGcomControllerImageFetcher)

void TestGcomControllerImageFetcher::initTestCase()
{
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

    gcom.on_dcncConnectionButton_clicked();
    socket->connectToHost(IP_ADDRESS, PORT);
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));

    QCOMPARE(gcom.ui->dcncStatusField->text(), SEARCHING_LABEL);
}

void TestGcomControllerImageFetcher::cleanupTestCase()
{
    disconnect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    disconnect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

    delete socket;
}

void TestGcomControllerImageFetcher::socketConnected()
{
    qInfo() << "Socket Connected";
    connectionDataStream.setDevice(socket);
}

void TestGcomControllerImageFetcher::socketDisconnected()
{
    qInfo() << "Socket Disconnected";
    connectionDataStream.unsetDevice();
}

void TestGcomControllerImageFetcher::handleClientData()
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
    connectionDataStream.resetStatus();
}

void TestGcomControllerImageFetcher::handleClientMessage(std::shared_ptr<UASMessage> message)
{
    switch (message->type())
    {
        case UASMessage::MessageID::COMMAND:
        {
            std::shared_ptr<CommandMessage> command =
                    std::static_pointer_cast<CommandMessage>(message);
            emit receivedCommand(command->command);
            break;
        }

        default:
            break;
    }
}
