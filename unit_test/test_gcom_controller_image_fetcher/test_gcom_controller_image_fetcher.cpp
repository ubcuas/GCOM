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

const int TAB_IMAGE_FETCHER = 2;

const QString CAMERA_TAGGED_TEXT("Camera with tags");
const QString CAMERA_UNTAGGED_TEXT("Camera without tags");

QTEST_MAIN(TestGcomControllerImageFetcher)

void TestGcomControllerImageFetcher::initTestCase()
{
    // Register metatypes for use in QSignalSpy
    qRegisterMetaType<CapabilitiesMessage::Capabilities>();

    socket = new QTcpSocket(this);
    connectionDataStream.setDevice(socket);
}

void TestGcomControllerImageFetcher::cleanupTestCase()
{
    connectionDataStream.unsetDevice();
    socket->disconnectFromHost();
    gcom.close();

    delete socket;
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

void TestGcomControllerImageFetcher::checkInitialStatus()
{
    // Verify status fields are disconnected, fetcher tab is disabled, no capabilities
    QCOMPARE(gcom.ui->dcncStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom.ui->dcncConnectionStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom.ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::startServer()
{
    // Verify that the only change is that dcnc is now searching
    gcom.on_dcncConnectionButton_clicked();
    QCOMPARE(gcom.ui->dcncStatusField->text(), SEARCHING_LABEL);
    QCOMPARE(gcom.ui->dcncConnectionStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom.ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::stopServer()
{
    int waitCounter = 0;

    gcom.on_dcncConnectionButton_clicked();
    checkInitialStatus();

    // wait for socket to change status
    while (socket->state() != QTcpSocket::UnconnectedState && waitCounter++ < 50)
        QTest::qWait(100);
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
}

void TestGcomControllerImageFetcher::connectSocket()
{
    socket->connectToHost(IP_ADDRESS, PORT);
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));
    // Wait for connection to be received on dcnc end
    QSignalSpy receivedConnectionSpy(gcom.dcnc, SIGNAL(receivedConnection()));
    QVERIFY(receivedConnectionSpy.isValid());
    QVERIFY(receivedConnectionSpy.wait());
    QCOMPARE(receivedConnectionSpy.count(), 1);
    // Send capabilities
    sendCapabilities(CapabilitiesMessage::Capabilities::CAMERA_TAGGED, 1);
    // Verify status fields are connected, fetcher tab is enabled, capabilities are displayed
    QCOMPARE(gcom.ui->dcncStatusField->text(), CONNECTED_LABEL);
    QCOMPARE(gcom.ui->dcncConnectionStatusField->text(), CONNECTED_LABEL);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->item(0)->text(), CAMERA_TAGGED_TEXT);
    QVERIFY(gcom.ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::disconnectSocket()
{
    int waitCounter = 0;

    socket->disconnectFromHost();

    // Wait for status to change
    waitCounter = 0;
    while (gcom.ui->dcncStatusField->text() != SEARCHING_LABEL && waitCounter++ < 50)
        QTest::qWait(100);

    QCOMPARE(gcom.ui->dcncStatusField->text(), SEARCHING_LABEL);
    QCOMPARE(gcom.ui->dcncConnectionStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom.ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::sendCapabilities(CapabilitiesMessage::Capabilities
                                                      capabilities, int num)
{
    CapabilitiesMessage capabilitiesMessage(capabilities);
    messageFramer.frameMessage(capabilitiesMessage);
    connectionDataStream << messageFramer;
    // Wait for capabilities to be received
    QSignalSpy receivedCapabilitiesSpy(gcom.dcnc, SIGNAL(receivedGremlinCapabilities(
                                                         CapabilitiesMessage::Capabilities)));
    QVERIFY(receivedCapabilitiesSpy.isValid());
    QVERIFY(receivedCapabilitiesSpy.wait());
    QCOMPARE(receivedCapabilitiesSpy.count(), 1);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->count(), num);
}

void TestGcomControllerImageFetcher::testConnection()
{
    checkInitialStatus();

    // Test start and stop searching without socket connecting
    startServer();
    stopServer();

    // Test start and stop searching with socket connecting
    startServer();
    connectSocket();
    stopServer();

    // Test connection being dropped on socket side, then reconnected
    startServer();
    connectSocket();
    disconnectSocket();
    connectSocket();
    // Ensure that more than one capability can be displayed
    sendCapabilities(CapabilitiesMessage::Capabilities::CAMERA_TAGGED << 8 |
                     CapabilitiesMessage::Capabilities::CAMERA_UNTAGGED, 3);
    // First capability is sent in the connectSocket function
    QCOMPARE(gcom.ui->dcncCapabilitiesField->item(0)->text(), CAMERA_TAGGED_TEXT);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->item(1)->text(), CAMERA_UNTAGGED_TEXT);
    QCOMPARE(gcom.ui->dcncCapabilitiesField->item(2)->text(), CAMERA_TAGGED_TEXT);
    stopServer();

    // Test timer timeout
    startServer();
    // Timeout timer
    gcom.dcncSearchTimeout();
    checkInitialStatus();

    // Start connection for rest of test
    startServer();
    connectSocket();
}
