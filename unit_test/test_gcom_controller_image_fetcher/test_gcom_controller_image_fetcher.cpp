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
const int WAIT_TIMEOUT = 5000;

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

const QString FETCHER_READY_LABEL("<font color='#05c400'> READY </font>");
const QString FETCHER_TRANSFER_LABEL("<font color='#05c400'> TRANSFERRING </font>");
const QString FETCHER_INVALID_PATH_LABEL("<font color='#D52D2D'> *Invalid Path </font>");
const QString FETCHER_NONREAL_PATH_LABEL("<font color='#D52D2D'> *Path does not exist</font>");

const QString IMAGE_TRANSER_START_TEXT("Start Image Transfer");
const QString IMAGE_TRANSER_STOP_TEXT("Stop Image Transfer");

const int FETCHER_STATUS_UNAVAILABLE = 0;
const int FETCHER_STATUS_READY = 1;
const int FETCHER_STATUS_TRANSFERRING = 2;

const int PATH_IMAGES = 0;
const int PATH_TAGS = 1;

#if defined(Q_OS_WIN)
    const QString INVALID_PATH_TEST[] = {
        "C:",                   // forward slash needed
        "path",                 // not an absolute path
        "D:/.path",             // cannot begin name with period
        "E://",                 // cannot begin name with forward slash
        "/ path here",          // cannot begin name with space
        "AC:/",                 // cannot have two letters as root directory
        "1:/",                  // number not allowed as directory
        "C/",                   // colon needed
        "C:/DATA*\"\\\\:?/",    // invalid characters
        "",                     // cannot have blank directory
        "/path",                // cannot have relative path
    };
    const QString VALID_PATH_TEST[] = {
        "C:/",                          // root directory
        "C:/data/TEST_PATH",            // caps allowed
        "C:/path path",                 // space allowed in middle of name
        "C:/path.path",                 // period allowed in middle of name
        "C:/path/",                     // forward slash allowed at end of name
        "c:/path",                      // lower case root directory
        "F:/path",                      // different directory
        "C:/1234567890",                // numbers allowed
        "C:/!@#$%^&()_+-={}[];',.`~",   // valid characters
        "C:/a",                         // single char folder
        "C:/a/b/c/d/e/f/g/i/j/k/l/m"
        "/n/o/p/q/r/s/t/u/v/w/x/y/z"    // long path
    };

    const int TEST_START_INDEX = 0;
    const int TEST_STOP_INDEX = 10;

#elif defined(Q_OS_MACOS)
    const QString INVALID_PATH_TEST[] = {
        "/.path",       // cannot begin name with period
        "/path:",       // cannot have colon in name
        "/asdf//",      // cannot begin name with forward slash
        "",             // cannot have blank directory
        "path",         // not an absolute path
        "/"             // not an absolute path
    };
    const QString VALID_PATH_TEST[] = {
        "/Users/Home",                 // regular path
        "/ PATH_HERE",                 // space allowed
        "/path/",                      // forward slash after
        "/1234567890",                 // numbers
        "/-=!@#$%^&*"
        "/()_+{}|\"<>?[]\\\\;',./",    // all characters but : and / allowed
        "/a/b/c/d/e/f/g/h/i/j/k/l/m/"
        "n/o/p/q/r/s/t/u/v/w/x/y/z/"   // long path
    };

    const int TEST_START_INDEX = 0;
    const int TEST_STOP_INDEX = 5;

#elif defined(Q_OS_LINUX)
    const QString INVALID_PATH_TEST[] = {
        "//",           // cannot have forward slash in name
        "/home//",      // cannot have forward slash in name
        "",             // cannot have blank directory
        "path",         // not an absolute path
        "~"             // not an absolute path
    };
    const QString VALID_PATH_TEST[] = {
        "/home/user",                   // regular path
        "/home/user/",                  // forward slash after path
        "/a/b/c/e/d/f/g/h/i/j/k/l/m/"
        "n/o/p/q/r/s/t/u/v/w/x/y/z/",   // long path
        "/",                            // root directory
        "/home/!@#$%^&*()_+{}|:\"<>?"
        "1234567890-=[]\\;',."          // all characters besides / are valid
    };

    const int TEST_START_INDEX = 0;
    const int TEST_STOP_INDEX = 4;
#else
#endif

const QString REAL_PATH = "C:/DATA";
const QString NONREAL_PATH = "D:/DATA/PATH/";

const bool VISIBLE = false;
const bool HIDDEN = true;
const bool ENABLED = true;
const bool DISABLED = false;

QTEST_MAIN(TestGcomControllerImageFetcher)

void TestGcomControllerImageFetcher::initTestCase()
{
    // Register metatypes for use in QSignalSpy
    qRegisterMetaType<CapabilitiesMessage::Capabilities>();
    qRegisterMetaType<CommandMessage::Commands>();

    gcom = new GcomController();

    socket = new QTcpSocket(this);
    connectionDataStream.setDevice(socket);

    connect(socket, SIGNAL(readyRead()), this, SLOT(handleClientData()));
}

void TestGcomControllerImageFetcher::cleanupTestCase()
{
    disconnect(socket, SIGNAL(readyRead()), this, SLOT(handleClientData()));
    connectionDataStream.unsetDevice();
    socket->disconnectFromHost();
    gcom->close();

    delete gcom;
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

void TestGcomControllerImageFetcher::checkDCNCInitialStatus()
{
    // Verify status fields are disconnected, fetcher tab is disabled, no capabilities
    QCOMPARE(gcom->ui->dcncStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom->ui->dcncConnectionStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom->ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::startServer()
{
    // Verify that the only change from initial status is that dcnc is now searching
    gcom->on_dcncConnectionButton_clicked();
    QCOMPARE(gcom->ui->dcncStatusField->text(), SEARCHING_LABEL);
    QCOMPARE(gcom->ui->dcncConnectionStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom->ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::stopServer()
{
    int waitCounter = 0;

    gcom->on_dcncConnectionButton_clicked();
    checkDCNCInitialStatus();

    // wait for socket to change status
    while (socket->state() != QTcpSocket::UnconnectedState && waitCounter++ < WAIT_TIMEOUT)
        QTest::qWait(1);
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
}

void TestGcomControllerImageFetcher::connectSocket()
{
    QSignalSpy receivedConnectionSpy(gcom->dcnc, SIGNAL(receivedConnection()));
    QVERIFY(receivedConnectionSpy.isValid());

    socket->connectToHost(IP_ADDRESS, PORT);
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));

    // Wait for connection to be received on dcnc end
    QVERIFY(receivedConnectionSpy.wait());
    QCOMPARE(receivedConnectionSpy.count(), 1);

    // Send capabilities
    sendCapabilities(CapabilitiesMessage::Capabilities::CAMERA_TAGGED, 1);

    // Verify status fields are connected, fetcher tab is enabled, capabilities are displayed
    QCOMPARE(gcom->ui->dcncStatusField->text(), CONNECTED_LABEL);
    QCOMPARE(gcom->ui->dcncConnectionStatusField->text(), CONNECTED_LABEL);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->item(0)->text(), CAMERA_TAGGED_TEXT);
    QVERIFY(gcom->ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));

    // Verify all fetcher fields are in default state
    QCOMPARE(gcom->fetcherStatus, FETCHER_STATUS_READY);
    QCOMPARE(gcom->ui->fetcherStatusField->text(), FETCHER_READY_LABEL);
    QCOMPARE(gcom->ui->fetcherImageTransferButton->text(), IMAGE_TRANSER_START_TEXT);
}

void TestGcomControllerImageFetcher::disconnectSocket()
{
    int waitCounter = 0;

    socket->disconnectFromHost();

    // Wait for status to change
    waitCounter = 0;
    while (gcom->ui->dcncStatusField->text() != SEARCHING_LABEL && waitCounter++ < WAIT_TIMEOUT)
        QTest::qWait(1);

    QCOMPARE(gcom->ui->dcncStatusField->text(), SEARCHING_LABEL);
    QCOMPARE(gcom->ui->dcncConnectionStatusField->text(), DISCONNECT_LABEL);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom->ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::connectSocketNoCapabilities()
{
    QSignalSpy receivedConnectionSpy(gcom->dcnc, SIGNAL(receivedConnection()));
    QVERIFY(receivedConnectionSpy.isValid());

    socket->connectToHost(IP_ADDRESS, PORT);
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));

    // Wait for connection to be received on dcnc end
    QVERIFY(receivedConnectionSpy.wait());
    QCOMPARE(receivedConnectionSpy.count(), 1);

    QCOMPARE(gcom->ui->dcncStatusField->text(), CONNECTED_LABEL);
    QCOMPARE(gcom->ui->dcncConnectionStatusField->text(), CONNECTED_LABEL);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->count(), 0);
    QVERIFY(!gcom->ui->tabMain->isTabEnabled(TAB_IMAGE_FETCHER));
}

void TestGcomControllerImageFetcher::sendCapabilities(CapabilitiesMessage::Capabilities
                                                      capabilities, int num)
{
    QSignalSpy receivedCapabilitiesSpy(gcom->dcnc, SIGNAL(receivedGremlinCapabilities(
                                                         CapabilitiesMessage::Capabilities)));
    QVERIFY(receivedCapabilitiesSpy.isValid());

    CapabilitiesMessage capabilitiesMessage(capabilities);
    messageFramer.frameMessage(capabilitiesMessage);
    connectionDataStream << messageFramer;

    // Wait for capabilities to be received
    QVERIFY(receivedCapabilitiesSpy.wait());
    QCOMPARE(receivedCapabilitiesSpy.count(), 1);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->count(), num);
}

void TestGcomControllerImageFetcher::testConnection()
{
    checkDCNCInitialStatus();

    // Test start and stop searching without socket connecting
    startServer();
    stopServer();

    // Test start and stop searching with socket connecting, without capabilities sent
    startServer();
    connectSocketNoCapabilities();
    stopServer();

    // Test start and stop searching with socket connecting, capabilities sent
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
    // First capability wass sent in the connectSocket function
    QCOMPARE(gcom->ui->dcncCapabilitiesField->item(0)->text(), CAMERA_TAGGED_TEXT);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->item(1)->text(), CAMERA_UNTAGGED_TEXT);
    QCOMPARE(gcom->ui->dcncCapabilitiesField->item(2)->text(), CAMERA_TAGGED_TEXT);
    disconnectSocket();
    stopServer();

    // Test timer timeout
    startServer();
    // Timeout timer
    gcom->dcncSearchTimeout();
    checkDCNCInitialStatus();
}

void TestGcomControllerImageFetcher::testRegexValidPath_data()
{
    QTest::addColumn<QString>("validPath");

    for (int i = TEST_START_INDEX; i <= TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << VALID_PATH_TEST[i];
    }
}

void TestGcomControllerImageFetcher::testRegexValidPath()
{
    QFETCH(QString, validPath);

    gcom->validatePath(validPath);

    QVERIFY(gcom->ui->fetcherPathInvalidLabel->isHidden());
}

void TestGcomControllerImageFetcher::testRegexInvalidPath_data()
{
    QTest::addColumn<QString>("invalidPath");

    for (int i = TEST_START_INDEX; i <= TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << INVALID_PATH_TEST[i];
    }
}

void TestGcomControllerImageFetcher::testRegexInvalidPath()
{
    QFETCH(QString, invalidPath);

    gcom->validatePath(invalidPath);

    QVERIFY(!gcom->ui->fetcherPathInvalidLabel->isHidden());
    QCOMPARE(gcom->ui->fetcherPathInvalidLabel->text(),
             FETCHER_INVALID_PATH_LABEL);
}

void TestGcomControllerImageFetcher::checkFetcherStatus(bool transferButtonEnabled,
                                                        bool invalidLabelHidden,
                                                        QString invalidLabel)
{
    QCOMPARE(gcom->fetcherStatus, FETCHER_STATUS_READY);
    QCOMPARE(gcom->ui->fetcherStatusField->text(), FETCHER_READY_LABEL);
    QCOMPARE(gcom->ui->fetcherImageTransferButton->text(), IMAGE_TRANSER_START_TEXT);
    QVERIFY(gcom->ui->fetcherImageTransferButton->isEnabled() == transferButtonEnabled);
    QVERIFY(gcom->ui->fetcherPathInvalidLabel->isHidden() == invalidLabelHidden);
    QVERIFY(gcom->ui->fetcherPathField->isEnabled());
    QVERIFY(gcom->ui->fetcherPathButton->isEnabled());
    if (!invalidLabelHidden)
        QCOMPARE(gcom->ui->fetcherPathInvalidLabel->text(), invalidLabel);
}

void TestGcomControllerImageFetcher::startImageTransferSuccess()
{
    connect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
            this, SLOT(compareStartImageTransfer(CommandMessage::Commands)));

    QSignalSpy receivedCommandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(receivedCommandSpy.isValid());

    gcom->on_fetcherImageTransferButton_clicked();
    QCOMPARE(gcom->fetcherStatus, FETCHER_STATUS_TRANSFERRING);
    QCOMPARE(gcom->ui->fetcherStatusField->text(), FETCHER_TRANSFER_LABEL);
    QCOMPARE(gcom->ui->fetcherImageTransferButton->text(), IMAGE_TRANSER_STOP_TEXT);
    QVERIFY(!gcom->ui->fetcherPathField->isEnabled());
    QVERIFY(!gcom->ui->fetcherPathButton->isEnabled());

    QVERIFY(receivedCommandSpy.wait());
    QCOMPARE(receivedCommandSpy.count(), 1);
}

void TestGcomControllerImageFetcher::startImageTransferFail(bool invalidLabelHidden)
{
    connect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
            this, SLOT(compareStartImageTransfer(CommandMessage::Commands)));

    QSignalSpy receivedCommandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(receivedCommandSpy.isValid());

    gcom->on_fetcherImageTransferButton_clicked();
    checkFetcherStatus(DISABLED, invalidLabelHidden, FETCHER_NONREAL_PATH_LABEL);

    QVERIFY(receivedCommandSpy.wait(500));
    QCOMPARE(receivedCommandSpy.count(), 0);
}

void TestGcomControllerImageFetcher::stopImageTransfer()
{
    connect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
            this, SLOT(compareStopImageTransfer(CommandMessage::Commands)));

    QSignalSpy receivedCommandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(receivedCommandSpy.isValid());

    gcom->on_fetcherImageTransferButton_clicked();
    QCOMPARE(gcom->fetcherStatus, FETCHER_STATUS_READY);
    QCOMPARE(gcom->ui->fetcherStatusField->text(), FETCHER_READY_LABEL);
    QCOMPARE(gcom->ui->fetcherImageTransferButton->text(), IMAGE_TRANSER_START_TEXT);
    QVERIFY(gcom->ui->fetcherPathField->isEnabled());
    QVERIFY(gcom->ui->fetcherPathButton->isEnabled());

    QVERIFY(receivedCommandSpy.wait());
    QCOMPARE(receivedCommandSpy.count(), 1);
}

void TestGcomControllerImageFetcher::compareStartImageTransfer(CommandMessage::Commands command)
{
    QCOMPARE(command, CommandMessage::Commands::IMAGE_RELAY_START);

    disconnect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
               this, SLOT(compareStartImageTransfer(CommandMessage::Commands)));
}

void TestGcomControllerImageFetcher::compareStopImageTransfer(CommandMessage::Commands command)
{
    QCOMPARE(command, CommandMessage::Commands::IMAGE_RELAY_STOP);

    disconnect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
               this, SLOT(compareStopImageTransfer(CommandMessage::Commands)));
}

void TestGcomControllerImageFetcher::testImageTransfer()
{
    // Reset gcom back to initial state
    delete gcom;
    gcom = new GcomController();
    QCOMPARE(gcom->fetcherStatus, FETCHER_STATUS_UNAVAILABLE);

    // Setup connection and check for initial statuses
    startServer();
    connectSocket();
    checkFetcherStatus(ENABLED, HIDDEN, FETCHER_INVALID_PATH_LABEL);

    // Set invalid path and check that user is unable to start image transfer
    gcom->ui->fetcherPathField->setText(INVALID_PATH_TEST[0]);
    checkFetcherStatus(DISABLED, VISIBLE, FETCHER_INVALID_PATH_LABEL);

    // Set valid path and check that user is able to start image transfer
    gcom->ui->fetcherPathField->setText(VALID_PATH_TEST[0]);
    checkFetcherStatus(ENABLED, HIDDEN, FETCHER_INVALID_PATH_LABEL);

    startImageTransferSuccess();

    // Drop connection, restart, and check statuses
    disconnectSocket();
    connectSocket();

    // Restart image transfer
    startImageTransferSuccess();

    stopImageTransfer();

//    fetcherSetPath(NONREAL_PATH);
//    startImageTransferFail(VISIBLE);

//    fetcherSetPath(REAL_PATH);
//    startImageTransferSuccess();
}
