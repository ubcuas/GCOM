//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QSignalSpy>
#include <QtTest/QtTest>
// GCOM Includes
#include "test_dcnc.hpp"
#include "modules/uas_message/request_message.hpp"
#include "modules/uas_message/system_info_message.hpp"

const QString IP_ADDRESS = "127.0.0.1";
const int PORT = 4206;

const int SOCKET_TIMEOUT_DURATION = 5000;

const int TEST_START_INDEX = 0;
const int TEST_STOP_INDEX = 2;
const int HANDLE_MESSAGE_TEST_START_INDEX = 0;
const int HANDLE_MESSAGE_TEST_STOP_INDEX = 3;

const std::string SYSTEM_ID_TEST[] = {"Gremlin", "Gremlin", "Gremlin", "Gremlin"};

const uint16_t VERSION_NUMBER_TEST[] = { 0, 100, 19999, 65535 };

const bool DROPPED_TEST[] = { true, false, true, true };

const bool AUTO_RESUME_TEST[] = { true, true, false, true };

const CommandMessage::Commands COMMANDS_TEST[] =
{
    CommandMessage::Commands::SYSTEM_RESET,
    CommandMessage::Commands::SYSTEM_RESUME,
    CommandMessage::Commands::DATA_REQUEST
};

const CapabilitiesMessage::Capabilities CAPABILITIES_TEST[] =
{
    CapabilitiesMessage::Capabilities::CAMERA_TAGGED,
    CapabilitiesMessage::Capabilities::CAMERA_UNTAGGED,
    CapabilitiesMessage::Capabilities::IMAGE_RELAY,
    CapabilitiesMessage::Capabilities::IMAGE_RELAY
};

const CommandMessage::Commands HANDLE_RESPONSE_COMMANDS_TEST[] =
{
    CommandMessage::Commands::SYSTEM_RESET,
    CommandMessage::Commands::SYSTEM_RESUME,
    CommandMessage::Commands::SYSTEM_RESUME,
    CommandMessage::Commands::DATA_REQUEST
};

const ResponseMessage::ResponseCodes HANDLE_RESPONSE_RESPONSE_TEST[] =
{
    ResponseMessage::ResponseCodes::NO_ERROR,
    ResponseMessage::ResponseCodes::INVALID_COMMAD,
    ResponseMessage::ResponseCodes::NO_ERROR,
    ResponseMessage::ResponseCodes::INVALID_REQUEST
};

const QString IMAGES_TEST[] =
{
    IMAGE_PATH + QString("connected.png"),
    IMAGE_PATH + QString("connecting.png"),
    IMAGE_PATH + QString("mavlink_connected.gif")
};

const uint8_t SEQUENCE_NUMBER_TEST[] = { 0, 100, 255 };
const int32_t LAT_TEST[] = { 0, -900000000, 900000000 };
const int32_t LON_TEST[] = { 0, -1800000000, 1800000000 };
const int32_t ALT_ABS_TEST[] = { 0, -500000, 500000 };
const int32_t ALT_REL_TEST[] = { 0, -500000, 500000 };
const uint16_t HDG_TEST[] = { 0, 2345, 36000 };

QTEST_MAIN(TestDCNC)

void TestDCNC::initTestCase()
{
    // Register types for use in QSignalSpy
    qRegisterMetaType<std::string>();
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<UASMessage::MessageID>();
    qRegisterMetaType<CapabilitiesMessage::Capabilities>();
    qRegisterMetaType<CommandMessage::Commands>();
    qRegisterMetaType<ResponseMessage::ResponseCodes>();
    qRegisterMetaType<std::shared_ptr<ImageUntaggedMessage>>();
    qRegisterMetaType<std::shared_ptr<ImageTaggedMessage>>();

    // Setup DCNC
    dcnc = new DCNC();
    QVERIFY(dcnc->startServer(IP_ADDRESS, PORT));
    QCOMPARE(dcnc->status(), DCNC::DCNCStatus::SEARCHING);

    // Setup socket
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    socket->connectToHost(IP_ADDRESS, PORT);

    // Verify socket connects
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));

    connect(socket, SIGNAL(readyRead()), this, SLOT(handleClientData()));

    // Verify connection is received on DCNC end
    QSignalSpy receivedConnectionSpy(dcnc, SIGNAL(receivedConnection()));
    QVERIFY(receivedConnectionSpy.isValid());
    QVERIFY(receivedConnectionSpy.wait());
    QCOMPARE(dcnc->status(), DCNC::DCNCStatus::CONNECTED);
    QCOMPARE(receivedConnectionSpy.count(), 1);

    // Verify the DATA_SYSTEM_INFO request in DCNC's handleClientConnection sends correctly
    connect(this, SIGNAL(receivedRequest(UASMessage::MessageID)),
            this, SLOT(compareRequestMessage(UASMessage::MessageID)));
    QSignalSpy receivedRequestSpy(this, SIGNAL(receivedRequest(UASMessage::MessageID)));
    QVERIFY(receivedRequestSpy.isValid());
    QVERIFY(receivedRequestSpy.wait());
    QCOMPARE(receivedRequestSpy.count(), 1);

    // Try connecting another socket to dcnc
    socketOther = new QTcpSocket(this);
    socketOther->connectToHost(IP_ADDRESS, PORT);
    // Verify socket does not connect
    QVERIFY(!socketOther->waitForConnected(SOCKET_TIMEOUT_DURATION));
    // Verify connection is NOT received on DCNC end
    QVERIFY(!receivedConnectionSpy.wait());
}

void TestDCNC::cleanupTestCase()
{
    socket->disconnectFromHost();

    dcnc->stopServer();
    QCOMPARE(dcnc->status(), DCNC::DCNCStatus::OFFLINE);

    disconnect(socket, SIGNAL(readyRead()), this, SLOT(handleClientData()));
    disconnect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    disconnect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

    delete socket;
    delete socketOther;
    delete dcnc;
}

void TestDCNC::socketConnected()
{
    qInfo() << "Socket Connected";
    connectionDataStream.setDevice(socket);
}

void TestDCNC::socketDisconnected()
{
    qInfo() << "Socket Disconnected";
    connectionDataStream.unsetDevice();
}

void TestDCNC::handleClientData()
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

void TestDCNC::handleClientMessage(std::shared_ptr<UASMessage> message)
{
    switch (message->type())
    {
        case UASMessage::MessageID::REQUEST:
        {
            std::shared_ptr<RequestMessage> request =
                    std::static_pointer_cast<RequestMessage>(message);
            emit receivedRequest(request->requestedMessage);
            break;
        }
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

void TestDCNC::compareRequestMessage(UASMessage::MessageID request)
{
    QCOMPARE(request, UASMessage::MessageID::DATA_SYSTEM_INFO);
    disconnect(this, SIGNAL(receivedRequest(UASMessage::MessageID)),
               this, SLOT(compareRequestMessage(UASMessage::MessageID)));
}

void TestDCNC::testSendUASMessage_data()
{
    QTest::addColumn<CommandMessage::Commands>("command");

    for (int i = TEST_START_INDEX; i <= TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << COMMANDS_TEST[i];
    }
}

void TestDCNC::testSendUASMessage()
{
    QFETCH(CommandMessage::Commands, command);
    sentCommand = command;

    connect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
            this, SLOT(compareSendUASMessage(CommandMessage::Commands)));

    QSignalSpy commandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(commandSpy.isValid());

    CommandMessage message(sentCommand);
    QVERIFY(dcnc->sendUASMessage(std::shared_ptr<CommandMessage>(&message)));

    QVERIFY(commandSpy.wait());
    QCOMPARE(commandSpy.count(), 1);
}

void TestDCNC::compareSendUASMessage(CommandMessage::Commands command)
{
    QCOMPARE(command, sentCommand);
    disconnect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
               this, SLOT(compareSendUASMessage(CommandMessage::Commands)));
}

void TestDCNC::testStartImageRelay()
{
    connect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
            this, SLOT(compareStartImageRelay(CommandMessage::Commands)));

    QSignalSpy commandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(commandSpy.isValid());

    dcnc->startImageRelay();

    QVERIFY(commandSpy.wait());
    QCOMPARE(commandSpy.count(), 1);
}

void TestDCNC::compareStartImageRelay(CommandMessage::Commands command)
{
    QCOMPARE(command, CommandMessage::Commands::IMAGE_RELAY_START);
    disconnect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
               this, SLOT(compareStartImageRelay(CommandMessage::Commands)));
}

void TestDCNC::testStopImageRelay()
{
    connect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
            this, SLOT(compareStopImageRelay(CommandMessage::Commands)));

    QSignalSpy commandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(commandSpy.isValid());

    dcnc->stopImageRelay();

    QVERIFY(commandSpy.wait());
    QCOMPARE(commandSpy.count(), 1);
}

void TestDCNC::compareStopImageRelay(CommandMessage::Commands command)
{
    QCOMPARE(command, CommandMessage::Commands::IMAGE_RELAY_STOP);
    disconnect(this, SIGNAL(receivedCommand(CommandMessage::Commands)),
               this, SLOT(compareStopImageRelay(CommandMessage::Commands)));
}

void TestDCNC::testHandleClientMessage_systemInfo_data()
{
    QTest::addColumn<std::string>("systemID");
    QTest::addColumn<uint16_t>("versionNumber");
    QTest::addColumn<bool>("dropped");
    QTest::addColumn<bool>("autoResume");

    for (int i = HANDLE_MESSAGE_TEST_START_INDEX; i <= HANDLE_MESSAGE_TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << SYSTEM_ID_TEST[i]
                                                      << VERSION_NUMBER_TEST[i]
                                                      << DROPPED_TEST[i]
                                                      << AUTO_RESUME_TEST[i];
    }
}

void TestDCNC::testHandleClientMessage_systemInfo()
{
    QFETCH(std::string, systemID);
    QFETCH(uint16_t, versionNumber);
    QFETCH(bool, dropped);
    QFETCH(bool, autoResume);
    std::string prevSystemID;

    sentSystemID = systemID;
    sentVersionNumber = versionNumber;
    sentDropped = dropped;
    dcnc->changeAutoResume(autoResume);

    // Obtain the test number from the name of the row in the data table
    int testNumber = QString(QTest::currentDataTag()).toInt();
    if (testNumber != 0)
        prevSystemID.assign(SYSTEM_ID_TEST[testNumber - 1]);
    else
        prevSystemID.assign("Not_Gremlin");

    connect(dcnc, SIGNAL(receivedGremlinInfo(QString, uint16_t, bool)),
            this, SLOT(compareHandleClientMessage_systemInfo(QString, uint16_t, bool)));

    QSignalSpy systemInfoSpy(dcnc, SIGNAL(receivedGremlinInfo(QString, uint16_t, bool)));
    QVERIFY(systemInfoSpy.isValid());
    QSignalSpy commandSpy(this, SIGNAL(receivedCommand(CommandMessage::Commands)));
    QVERIFY(commandSpy.isValid());
    QSignalSpy requestSpy(this, SIGNAL(receivedRequest(UASMessage::MessageID)));
    QVERIFY(requestSpy.isValid());

    SystemInfoMessage systemInfo(sentSystemID, sentVersionNumber, sentDropped);
    messageFramer.frameMessage(systemInfo);
    connectionDataStream << messageFramer;

    QVERIFY(systemInfoSpy.wait());
    QCOMPARE(systemInfoSpy.count(), 1);

    if (sentDropped && prevSystemID.compare(sentSystemID) == 0)
    {
        QVERIFY(commandSpy.wait());
        QCOMPARE(commandSpy.count(), 1);

        // Retrieve the command from the signal
        QList<QVariant> commandSignal = commandSpy.takeFirst();
        CommandMessage::Commands command = static_cast<CommandMessage::Commands>(
                                                    commandSignal.at(0).toInt());

        if (autoResume)
            QCOMPARE(command, CommandMessage::Commands::SYSTEM_RESUME);
        else
            QCOMPARE(command, CommandMessage::Commands::SYSTEM_RESET);

        return;
    }

    QVERIFY(requestSpy.wait());
    QCOMPARE(requestSpy.count(), 1);

    // Retrieve the request from the signal
    QList<QVariant> requestSignal = requestSpy.takeFirst();
    UASMessage::MessageID request = static_cast<UASMessage::MessageID>(
                                                requestSignal.at(0).toInt());
    QCOMPARE(request, UASMessage::MessageID::DATA_CAPABILITIES);
}

void TestDCNC::compareHandleClientMessage_systemInfo(QString systemID, uint16_t versionNumber,
                                                     bool dropped)
{
    QCOMPARE(systemID.toStdString(), sentSystemID);
    QCOMPARE(versionNumber, sentVersionNumber);
    QCOMPARE(dropped, sentDropped);

    disconnect(dcnc, SIGNAL(receivedGremlinInfo(QString, uint16_t, bool)),
            this, SLOT(compareHandleClientMessage_systemInfo(QString, uint16_t, bool)));
}

void TestDCNC::testHandleClientMessage_capabilities_data()
{
    QTest::addColumn<CapabilitiesMessage::Capabilities>("capability");

    for (int i = HANDLE_MESSAGE_TEST_START_INDEX; i <= HANDLE_MESSAGE_TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << CAPABILITIES_TEST[i];
    }
}

void TestDCNC::testHandleClientMessage_capabilities()
{
    QFETCH(CapabilitiesMessage::Capabilities, capability);
    sentCapability = capability;

    connect(dcnc, SIGNAL(receivedGremlinCapabilities(CapabilitiesMessage::Capabilities)),
            this, SLOT(compareHandleClientMessage_capabilities(
                       CapabilitiesMessage::Capabilities)));

    QSignalSpy capabilitySpy(dcnc, SIGNAL(
                             receivedGremlinCapabilities(CapabilitiesMessage::Capabilities)));
    QVERIFY(capabilitySpy.isValid());

    CapabilitiesMessage capabilityMessage(sentCapability);
    messageFramer.frameMessage(capabilityMessage);
    connectionDataStream << messageFramer;

    QVERIFY(capabilitySpy.wait());
    QCOMPARE(capabilitySpy.count(), 1);
}

void TestDCNC::compareHandleClientMessage_capabilities(CapabilitiesMessage::Capabilities capability)
{
    QCOMPARE(capability, sentCapability);
    disconnect(dcnc, SIGNAL(receivedGremlinCapabilities(CapabilitiesMessage::Capabilities)),
               this, SLOT(compareHandleClientMessage_capabilities(
                          CapabilitiesMessage::Capabilities)));
}

void TestDCNC::testHandleClientMessage_response_data()
{
    QTest::addColumn<CommandMessage::Commands>("command");
    QTest::addColumn<ResponseMessage::ResponseCodes>("responseCode");

    for (int i = HANDLE_MESSAGE_TEST_START_INDEX; i <= HANDLE_MESSAGE_TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << HANDLE_RESPONSE_COMMANDS_TEST[i] <<
                                                         HANDLE_RESPONSE_RESPONSE_TEST[i];
    }
}

void TestDCNC::testHandleClientMessage_response()
{
    QFETCH(CommandMessage::Commands, command);
    QFETCH(ResponseMessage::ResponseCodes, responseCode);
    sentCommand = command;
    sentResponseCode = responseCode;

    connect(dcnc, SIGNAL(receivedGremlinResponse(CommandMessage::Commands,
                                                 ResponseMessage::ResponseCodes)),
            this, SLOT(compareHandleClientMessage_response(CommandMessage::Commands,
                                                           ResponseMessage::ResponseCodes)));

    QSignalSpy responseSpy(dcnc, SIGNAL(receivedGremlinResponse(CommandMessage::Commands,
                                                                ResponseMessage::ResponseCodes)));
    QVERIFY(responseSpy.isValid());

    QSignalSpy droppedConnectionSpy(dcnc, SIGNAL(droppedConnection()));
    QVERIFY(droppedConnectionSpy.isValid());

    ResponseMessage response(sentCommand, sentResponseCode);
    messageFramer.frameMessage(response);
    connectionDataStream << messageFramer;

    QVERIFY(responseSpy.wait());
    QCOMPARE(responseSpy.count(), 1);

    switch(sentCommand)
    {
        case CommandMessage::Commands::SYSTEM_RESET:
        {
            QCOMPARE(droppedConnectionSpy.count(), 1);
            break;
        }
        case CommandMessage::Commands::SYSTEM_RESUME:
        {
            if (sentResponseCode != ResponseMessage::ResponseCodes::NO_ERROR)
            {
                QCOMPARE(droppedConnectionSpy.count(), 1);
                break;
            }

            QCOMPARE(droppedConnectionSpy.count(), 0);
            break;
        }
        default:
            QCOMPARE(droppedConnectionSpy.count(), 0);
    }
}

void TestDCNC::compareHandleClientMessage_response(CommandMessage::Commands command,
                                                   ResponseMessage::ResponseCodes responseCode)
{
    QCOMPARE(command, sentCommand);
    QCOMPARE(responseCode, sentResponseCode);

    disconnect(dcnc, SIGNAL(receivedGremlinResponse(CommandMessage::Commands,
                                                    ResponseMessage::ResponseCodes)),
               this, SLOT(compareHandleClientMessage_response(CommandMessage::Commands,
                                                              ResponseMessage::ResponseCodes)));
}

void TestDCNC::testHandleClientMessage_imageUntagged_data()
{
    QTest::addColumn<uint8_t>("sequenceNumber");
    QTest::addColumn<QString>("imageUntaggedPath");

    for (int i = TEST_START_INDEX; i <= TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << SEQUENCE_NUMBER_TEST[i] << IMAGES_TEST[i];
    }
}

void TestDCNC::testHandleClientMessage_imageUntagged()
{
    QFETCH(uint8_t, sequenceNumber);
    QFETCH(QString, imageUntaggedPath);

    sentSequenceNumber = sequenceNumber;

    QFile imageUntagged(imageUntaggedPath);
    // Verify the image opened correctly
    QVERIFY(imageUntagged.open(QIODevice::ReadOnly));

    sentImageSize = imageUntagged.size();
    sentImageData = new uint8_t[sentImageSize];

    // Extract all bytes into byte array and copy it to sentImageData
    QByteArray imageByteArray = imageUntagged.readAll();
    std::copy(imageByteArray.begin(), imageByteArray.end(), sentImageData);

    imageUntagged.close();

    connect(dcnc, SIGNAL(receivedImageUntaggedData(std::shared_ptr<ImageUntaggedMessage>)),
            this, SLOT(compareHandleClientMessage_imageUntagged(
                       std::shared_ptr<ImageUntaggedMessage>)));

    QSignalSpy untaggedImageSpy(dcnc,
               SIGNAL(receivedImageUntaggedData(std::shared_ptr<ImageUntaggedMessage>)));
    QVERIFY(untaggedImageSpy.isValid());

    ImageUntaggedMessage imageUntaggedMessage(sentSequenceNumber, sentImageData, sentImageSize);
    messageFramer.frameMessage(imageUntaggedMessage);
    connectionDataStream << messageFramer;

    QVERIFY(untaggedImageSpy.wait());
    QCOMPARE(untaggedImageSpy.count(), 1);
}

void TestDCNC::compareHandleClientMessage_imageUntagged(std::shared_ptr<ImageUntaggedMessage> image)
{
    QCOMPARE(image->sequenceNumber, sentSequenceNumber);
    QCOMPARE(memcmp(image->imageData.data(), sentImageData, image->imageData.size()), 0);
    QCOMPARE(image->imageData.size(), sentImageSize);

    disconnect(dcnc, SIGNAL(receivedImageUntaggedData(std::shared_ptr<ImageUntaggedMessage>)),
               this, SLOT(compareHandleClientMessage_imageUntagged(
                          std::shared_ptr<ImageUntaggedMessage>)));

    delete sentImageData;
}

void TestDCNC::testHandleClientMessage_imageTagged_data()
{
    QTest::addColumn<uint8_t>("sequenceNumber");
    QTest::addColumn<int32_t>("latitude");
    QTest::addColumn<int32_t>("longitude");
    QTest::addColumn<int32_t>("altitudeAbs");
    QTest::addColumn<int32_t>("altitudeRel");
    QTest::addColumn<uint16_t>("heading");
    QTest::addColumn<QString>("imageTaggedPath");

    for (int i = TEST_START_INDEX; i <= TEST_STOP_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << SEQUENCE_NUMBER_TEST[i]
                                                      << LAT_TEST[i]
                                                      << LON_TEST[i]
                                                      << ALT_ABS_TEST[i]
                                                      << ALT_REL_TEST[i]
                                                      << HDG_TEST[i]
                                                      << IMAGES_TEST[i];
    }
}

void TestDCNC::testHandleClientMessage_imageTagged()
{
    QFETCH(uint8_t, sequenceNumber);
    QFETCH(int32_t, latitude);
    QFETCH(int32_t, longitude);
    QFETCH(int32_t, altitudeAbs);
    QFETCH(int32_t, altitudeRel);
    QFETCH(uint16_t, heading);
    QFETCH(QString, imageTaggedPath);

    sentSequenceNumber = sequenceNumber;
    sentLatitude = latitude;
    sentLongitude = longitude;
    sentAltitudeAbs = altitudeAbs;
    sentAltitudeRel = altitudeRel;
    sentHeading = heading;

    QFile imageTagged(imageTaggedPath);
    // Verify the image opened correctly
    QVERIFY(imageTagged.open(QIODevice::ReadOnly));

    sentImageSize = imageTagged.size();
    sentImageData = new uint8_t[sentImageSize];

    // Extract all bytes into byte array and copy it to sentImageData
    QByteArray imageByteArray = imageTagged.readAll();
    std::copy(imageByteArray.begin(), imageByteArray.end(), sentImageData);

    imageTagged.close();

    connect(dcnc, SIGNAL(receivedImageTaggedData(std::shared_ptr<ImageTaggedMessage>)),
            this, SLOT(compareHandleClientMessage_imageTagged(
                       std::shared_ptr<ImageTaggedMessage>)));

    QSignalSpy taggedImageSpy(dcnc,
               SIGNAL(receivedImageTaggedData(std::shared_ptr<ImageTaggedMessage>)));
    QVERIFY(taggedImageSpy.isValid());

    ImageTaggedMessage imageTaggedMessage(sentSequenceNumber, sentLatitude, sentLongitude,
                                          sentAltitudeAbs, sentAltitudeRel, sentHeading,
                                          sentImageData, sentImageSize);
    messageFramer.frameMessage(imageTaggedMessage);
    connectionDataStream << messageFramer;

    QVERIFY(taggedImageSpy.wait());
    QCOMPARE(taggedImageSpy.count(), 1);
}

void TestDCNC::compareHandleClientMessage_imageTagged(std::shared_ptr<ImageTaggedMessage> image)
{
    QCOMPARE(image->sequenceNumber, sentSequenceNumber);
    QCOMPARE(image->latitudeRaw, sentLatitude);
    QCOMPARE(image->longitudeRaw, sentLongitude);
    QCOMPARE(image->altitudeAbsRaw, sentAltitudeAbs);
    QCOMPARE(image->altitudeRelRaw, sentAltitudeRel);
    QCOMPARE(image->headingRaw, sentHeading);
    QCOMPARE(memcmp(image->imageData.data(), sentImageData, image->imageData.size()), 0);
    QCOMPARE(image->imageData.size(), sentImageSize);

    disconnect(dcnc, SIGNAL(receivedImageTaggedData(std::shared_ptr<ImageTaggedMessage>)),
               this, SLOT(compareHandleClientMessage_imageTagged(
                          std::shared_ptr<ImageTaggedMessage>)));

    delete sentImageData;
}

void TestDCNC::testCancelConnection()
{
    QSignalSpy droppedConnectionSpy(dcnc, SIGNAL(droppedConnection()));
    QVERIFY(droppedConnectionSpy.isValid());

    dcnc->cancelConnection();

    QCOMPARE(dcnc->status(), DCNC::DCNCStatus::SEARCHING);
    QCOMPARE(droppedConnectionSpy.count(), 1);

    // Verify that DCNC is now unable to send messages
    CapabilitiesMessage message(CapabilitiesMessage::Capabilities::IMAGE_RELAY);
    QVERIFY(!dcnc->sendUASMessage(std::shared_ptr<CapabilitiesMessage>(&message)));
}

void TestDCNC::testReconnect()
{
    socket->disconnectFromHost();
    socket->connectToHost(IP_ADDRESS, PORT);

    // Verify socket connects
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));

    // Verify connection is received on DCNC end
    QSignalSpy receivedConnectionSpy(dcnc, SIGNAL(receivedConnection()));
    QVERIFY(receivedConnectionSpy.isValid());
    QVERIFY(receivedConnectionSpy.wait());
    QCOMPARE(dcnc->status(), DCNC::DCNCStatus::CONNECTED);
    QCOMPARE(receivedConnectionSpy.count(), 1);

    // Verify the DATA_SYSTEM_INFO request in DCNC's handleClientConnection sends correctly
    connect(this, SIGNAL(receivedRequest(UASMessage::MessageID)),
            this, SLOT(compareRequestMessage(UASMessage::MessageID)));
    QSignalSpy receivedRequestSpy(this, SIGNAL(receivedRequest(UASMessage::MessageID)));
    QVERIFY(receivedRequestSpy.isValid());
    QVERIFY(receivedRequestSpy.wait());
    QCOMPARE(receivedRequestSpy.count(), 1);

    // Try connecting another socket to dcnc
    socketOther->connectToHost(IP_ADDRESS, PORT);
    // Verify socket does not connect
    QVERIFY(!socketOther->waitForConnected(SOCKET_TIMEOUT_DURATION));
    // Verify connection is NOT received on DCNC end
    QVERIFY(!receivedConnectionSpy.wait());
}
