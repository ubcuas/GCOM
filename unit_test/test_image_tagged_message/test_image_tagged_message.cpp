//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QFile>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <cstring>
// GCOM Includes
#include "test_image_tagged_message.hpp"

const QString IP_ADDRESS = "127.0.0.1";
const int PORT = 4206;

const int TEST_START_INDEX = 0;
const int TEST_END_INDEX = 6;
const uint8_t SEQUENCE_NUMBER_TEST[] = {0, 1, 34, 100, 178, 201, 255};
const int32_t LATITUDE_RAW_TEST[] = {0, 1, -1, 700001, 12345678, -900000000, 900000000};
const int32_t LONGITUDE_RAW_TEST[] = {0, 1, -1, 129999, 87654321, -1800000000, 1800000000};
const int32_t ALTITUDE_ABS_RAW_TEST[] = {0, 1, -1, 401, 29383, -500000, 500000};
const int32_t ALTITUDE_REL_RAW_TEST[] = {0, 1, -1, 399, 19273, -500000, 500000};
const uint16_t HEADING_RAW_TEST[] = {0, 1, 101, 2345, 10492, 32111, 36000};
const QString IMG_PATH_TEST[] = {
        IMAGE_PATH + QString("connected.png"), IMAGE_PATH + QString("mavlink_connected.gif"),
        IMAGE_PATH + QString("dcnc_connecting.gif"), IMAGE_PATH + QString("kingfisher.jpg"),
        IMAGE_PATH + QString("flower.jpeg"), IMAGE_PATH + QString("marbles.bmp"),
        IMAGE_PATH + QString("walle.jpg")};

// In milliseconds
const int SOCKET_TIMEOUT_DURATION = 5000;

QTEST_MAIN(TestImageTaggedMessage)

void TestImageTaggedMessage::initTestCase()
{
    // Register shared ptr to allow being used in signal
    qRegisterMetaType<std::shared_ptr<ImageTaggedMessage>>();

    dcnc = new DCNC();
    dcnc->startServer(IP_ADDRESS, PORT);

    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

    socket->connectToHost(IP_ADDRESS, PORT);
    // Verify socket connects
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));
}

void TestImageTaggedMessage::cleanupTestCase()
{
    socket->disconnectFromHost();
    dcnc->stopServer();

    disconnect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    disconnect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));

    delete socket;
    delete dcnc;
    delete sentImageData;
}

void TestImageTaggedMessage::socketConnected()
{
    qInfo() << "Socket Connected";
    connectionDataStream.resetStatus();
    connectionDataStream.setDevice(socket);
}

void TestImageTaggedMessage::socketDisconnected()
{
    qInfo() << "Socket Disconnected";
    connectionDataStream.resetStatus();
    connectionDataStream.unsetDevice();
}

void TestImageTaggedMessage::testSendImageTagged_data() {
    // Create a data table with each test as a row

    // Add headings
    QTest::addColumn<uint8_t>("sequenceNumber");
    QTest::addColumn<int32_t>("latitudeRaw");
    QTest::addColumn<int32_t>("longitudeRaw");
    QTest::addColumn<int32_t>("altitudeAbsRaw");
    QTest::addColumn<int32_t>("altitudeRelRaw");
    QTest::addColumn<uint16_t>("headingRaw");
    QTest::addColumn<QString>("imagePath");

    // Add rows
    for (int i = TEST_START_INDEX; i <= TEST_END_INDEX; i++)
    {
        QTest::newRow(qPrintable(QString::number(i))) << SEQUENCE_NUMBER_TEST[i]
                                                      << LATITUDE_RAW_TEST[i]
                                                      << LONGITUDE_RAW_TEST[i]
                                                      << ALTITUDE_ABS_RAW_TEST[i]
                                                      << ALTITUDE_REL_RAW_TEST[i]
                                                      << HEADING_RAW_TEST[i]
                                                      << IMG_PATH_TEST[i];
    }
}

void TestImageTaggedMessage::testSendImageTagged()
{
    // Retrieve all the data from a row in the data table to test
    QFETCH(uint8_t, sequenceNumber);
    QFETCH(int32_t, latitudeRaw);
    QFETCH(int32_t, longitudeRaw);
    QFETCH(int32_t, altitudeAbsRaw);
    QFETCH(int32_t, altitudeRelRaw);
    QFETCH(uint16_t, headingRaw);
    QFETCH(QString, imagePath);

    sentSequenceNumber = sequenceNumber;
    sentLatitudeRaw = latitudeRaw;
    sentLongitudeRaw = longitudeRaw;
    sentAltitudeAbsRaw = altitudeAbsRaw;
    sentAltitudeRelRaw = altitudeRelRaw;
    sentHeadingRaw = headingRaw;

    connect(dcnc, SIGNAL(receivedImageTaggedData(std::shared_ptr<ImageTaggedMessage>)),
            this, SLOT(compareImageTaggedData(std::shared_ptr<ImageTaggedMessage>)));

    QFile image(imagePath);
    // Verify image opened correctly
    QVERIFY(image.open(QIODevice::ReadOnly));

    sentImageSize = image.size();

    sentImageData = new uint8_t[sentImageSize];

    // Extract all bytes into byte array and copy it to sentImageData
    QByteArray imageByteArray = image.readAll();
    std::copy(imageByteArray.begin(), imageByteArray.end(), sentImageData);

    image.close();

    // Create a signal spy to keep track of the signal
    QSignalSpy taggedImageMessageSpy(dcnc,
               SIGNAL(receivedImageTaggedData(std::shared_ptr<ImageTaggedMessage>)));
    QVERIFY(taggedImageMessageSpy.isValid());

    ImageTaggedMessage outgoingMessage(sentSequenceNumber, sentLatitudeRaw, sentLongitudeRaw,
                                       sentAltitudeAbsRaw, sentAltitudeRelRaw, sentHeadingRaw,
                                       sentImageData, sentImageSize);
    messageFramer.frameMessage(outgoingMessage);
    connectionDataStream << messageFramer;

    // Waits until signal is emitted
    QVERIFY(taggedImageMessageSpy.wait());
}

void TestImageTaggedMessage::compareImageTaggedData(std::shared_ptr<ImageTaggedMessage> image)
{
    // Compare all values before sending and after sending
    QCOMPARE(image->sequenceNumber, sentSequenceNumber);
    QCOMPARE(image->latitudeRaw, sentLatitudeRaw);
    QCOMPARE(image->longitudeRaw, sentLongitudeRaw);
    QCOMPARE(image->altitudeAbsRaw, sentAltitudeAbsRaw);
    QCOMPARE(image->altitudeRelRaw, sentAltitudeRelRaw);
    QCOMPARE(image->headingRaw, sentHeadingRaw);

    // Compare the images and image sizes
    QCOMPARE(memcmp(image->imageData.data(), sentImageData, image->imageData.size()), 0);
    QCOMPARE(image->imageData.size(), sentImageSize);
    qInfo() << "Number of bytes checked: " << image->imageData.size();

    disconnect(dcnc, SIGNAL(receivedImageTaggedData(std::shared_ptr<ImageTaggedMessage>)),
           this, SLOT(compareImageTaggedData(std::shared_ptr<ImageTaggedMessage>)));
}
