//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QFile>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <cstring>
#include <QRegExp>
// GCOM Includes
#include "test_image_fetcher.hpp"
#include "modules/uas_image_fetcher/image_fetcher.hpp"
#include "modules/uas_dcnc/dcnc.hpp"

const QString IP_ADDRESS = "127.0.0.1";
const int PORT = 4206;

const QString REAL_DIR = IMAGE_DIR_PATH + QString("image_dir_1");
const QString FALSE_DIR = IMAGE_DIR_PATH + QString("not_a_dir");

const int TEST_START_INDEX = 0;
const int TEST_END_INDEX = 4;
const uint8_t SEQUENCE_NUMBER_TEST[] =  { 0,    1,   2,    3, 255 }; //0-255
const int32_t LATITUDE_RAW_TEST[] =     { 0,  -90, -45,   45, 90  }; //-90 to 90
const int32_t LONGITUDE_RAW_TEST[] =    { 0, -180, -90,   90, 180 }; //-180 to 180
const int32_t ALTITUDE_ABS_RAW_TEST[] = { 0,   -1,   1, -100, 100 };
const int32_t ALTITUDE_REL_RAW_TEST[] = { 0,   -1,   1, -100, 100 };
const uint16_t HEADING_RAW_TEST[] =     { 0,   1,   50,  100, 500 };
const QString IMG_PATH_TEST[] = {
        IMAGE_PATH + QString("mavlink_connected.gif"), IMAGE_PATH + QString("connected.png"),
        IMAGE_PATH + QString("dcnc_connecting.gif"), IMAGE_PATH + QString("kingfisher.jpg"),
        IMAGE_PATH + QString("flower.jpeg"), IMAGE_PATH + QString("marbles.bmp"),
        IMAGE_PATH + QString("walle.jpg")};



// In milliseconds
const int SOCKET_TIMEOUT_DURATION = 5000;

void TestImageFetcher::initTestCase()
{
    // Register shared ptr to allow being used in signal
    qRegisterMetaType<std::shared_ptr<ImageTaggedMessage>>();
    qRegisterMetaType<uint8_t>("uint8_t");

    // Create new DCNC pointer
    dcnc = new DCNC();
    dcnc->startServer(IP_ADDRESS, PORT);

    preSeqNum = 0;

    // Create new imageFetcher pointer
    imageFetcher = new ImageFetcher(REAL_DIR, dcnc);
    socket = new QTcpSocket(this);

    socket->connectToHost(IP_ADDRESS, PORT);
    // Verify socket connects
    QVERIFY(socket->waitForConnected(SOCKET_TIMEOUT_DURATION));
    connectionDataStream.setDevice(socket);
}

void TestImageFetcher::cleanupTestCase()
{
    socket->disconnectFromHost();
    dcnc->stopServer();

    connectionDataStream.unsetDevice();

    delete socket;
    delete imageFetcher;
    delete dcnc;
    delete expectedImageData;
}

// Tests constructor to see if it throws an exception when given non-existant directories
void TestImageFetcher::testConstructor()
{
    QVERIFY_EXCEPTION_THROWN(ImageFetcher(FALSE_DIR, dcnc), std::invalid_argument);
    try
    {
        ImageFetcher(REAL_DIR, dcnc);
    }
    catch(std::invalid_argument)
    {
        QVERIFY(false);
    }
}

void TestImageFetcher::testHandleImageTaggedMessage_data()
{
    // Set the current directory to a valid one
    imageFetcher->changeDir(REAL_DIR);

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
        QTest::newRow(qPrintable(QString::number(i))) << SEQUENCE_NUMBER_TEST[i] << LATITUDE_RAW_TEST[i]
                                                      << LONGITUDE_RAW_TEST[i] << ALTITUDE_ABS_RAW_TEST[i]
                                                      << ALTITUDE_REL_RAW_TEST[i] << HEADING_RAW_TEST[i]
                                                      << IMG_PATH_TEST[i];
    }
}
void TestImageFetcher::testHandleImageTaggedMessage()
{
    // Retrieve all the data from a row in the data table to test
    QFETCH(uint8_t, sequenceNumber);
    QFETCH(int32_t, latitudeRaw);
    QFETCH(int32_t, longitudeRaw);
    QFETCH(int32_t, altitudeAbsRaw);
    QFETCH(int32_t, altitudeRelRaw);
    QFETCH(uint16_t, headingRaw);
    QFETCH(QString, imagePath);

    // Save expected values for comparison
    expectedSequenceNumber = sequenceNumber;
    expectedLatitude = latitudeRaw;
    expectedLongitude = longitudeRaw;
    expectedAltitudeAbs = altitudeAbsRaw;
    expectedAltitudeRel = altitudeRelRaw;
    expectedHeading = headingRaw;
    int imageNum = -1;
    QRegExp imageNumberRegExp("(^IMG_)(\\d+)(.jpg)");
    QDirIterator it(REAL_DIR, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        QString filename(fileInfo.fileName());
        int pos = imageNumberRegExp.indexIn(filename);
        if (pos < 0)
            continue;
        QString imageNumber = imageNumberRegExp.cap(2);
        if (imageNumber.toInt() > imageNum)
            imageNum = imageNumber.toInt();

    }
    expectedFilePath = REAL_DIR + QString("/IMG_") + QString::number(imageNum+1) + QString(".jpg");

    // Open image and extract data
    QFile image(imagePath);
    QVERIFY(image.open(QIODevice::ReadOnly));
    expectedImageSize = image.size();
    expectedImageData = new uint8_t[expectedImageSize];
    QByteArray imageByteArray = image.readAll();
    std::copy(imageByteArray.begin(), imageByteArray.end(), expectedImageData);
    image.close();

    // Connect taggedImage signal to compare function
    connect(imageFetcher, SIGNAL(taggedImage(QString, double, double, float, float, float)),
            this, SLOT(compareHandleImageTaggedMessage(QString, double, double, float, float, float)));
    QSignalSpy taggedImageMessageSpy(imageFetcher,
                                     SIGNAL(taggedImage(QString, double, double, float, float, float)));
    QVERIFY(taggedImageMessageSpy.isValid());

    // Connect skippedFromSeqNumTo signal to compare function
    connect(imageFetcher, SIGNAL(skippedFromSeqNumTo(uint8_t, uint8_t)),
            this, SLOT(compareSkippedFromSeqNumTo(uint8_t, uint8_t)));
    QSignalSpy skippedFromSeqNumToSpy(imageFetcher, SIGNAL(skippedFromSeqNumTo(uint8_t, uint8_t)));
    QVERIFY(skippedFromSeqNumToSpy.isValid());

    // Send the image message
    ImageTaggedMessage outgoingMessage(expectedSequenceNumber, expectedLatitude, expectedLongitude,
                                       expectedAltitudeAbs, expectedAltitudeRel, expectedHeading,
                                       expectedImageData, expectedImageSize);
    messageFramer.frameMessage(outgoingMessage);
    connectionDataStream << messageFramer;

    // Waits until signal is emitted
    QVERIFY(taggedImageMessageSpy.wait());
    QCOMPARE(taggedImageMessageSpy.count(), 1);

    // Only sends signal if there was a jump in the sequence number
    if ((sequenceNumber == 0 && preSeqNum == 255) || (sequenceNumber == preSeqNum + 1))
        QCOMPARE(skippedFromSeqNumToSpy.count(), 0);
    else
        QCOMPARE(skippedFromSeqNumToSpy.count(), 1);
    preSeqNum = expectedSequenceNumber;
}

// Verifies that skippedFromSeqNumTo emitted the correct sequence numbers
void TestImageFetcher::compareSkippedFromSeqNumTo(uint8_t fromSeqNum, uint8_t toSeqNum)
{
    QCOMPARE(fromSeqNum, preSeqNum);
    QCOMPARE(toSeqNum, expectedSequenceNumber);

    // Disconnect skippedFromSeqNumTo singal from compareSkippedFromSeqNumTo slot
    disconnect(imageFetcher, SIGNAL(skippedFromSeqNumTo(uint8_t, uint8_t)),
            this, SLOT(compareSkippedFromSeqNumTo(uint8_t, uint8_t)));
}

// Verifies that the data in the signal taggedImage is as expected
void TestImageFetcher::compareHandleImageTaggedMessage(QString filePath, double latitude, double longitude,
                                                       float altitude_abs, float altitude_rel, float heading)
{
    QCOMPARE(filePath, expectedFilePath);
    QCOMPARE(latitude, expectedLatitude);
    QCOMPARE(longitude, expectedLongitude);
    QCOMPARE(altitude_abs, expectedAltitudeAbs);
    QCOMPARE(altitude_rel, expectedAltitudeRel);
    QCOMPARE(heading, expectedHeading);

    // Disconnect taggedImage signal from compareHandleImageTaggedMessage slot
    disconnect(imageFetcher, SIGNAL(taggedImage(QString, double, double, float, float, float)),
               this, SLOT(compareHandleImageTaggedMessage(QString, double, double, float, float, float)));
}

QTEST_MAIN(TestImageFetcher)
