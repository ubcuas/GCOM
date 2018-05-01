#ifndef TEST_IMAGE_TAGGED_MESSAGE_HPP
#define TEST_IMAGE_TAGGED_MESSAGE_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
// GCOM Includes
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_message/uas_message_tcp_framer.hpp"

Q_DECLARE_METATYPE(std::shared_ptr<ImageTaggedMessage>)

class TestImageTaggedMessage : public QObject
{
    Q_OBJECT

public slots:
    // Socket slots
    void socketConnected();
    void socketDisconnected();

    void compareImageTaggedData(std::shared_ptr<ImageTaggedMessage> message);

private slots:
    // Function to be tested
    void testSendImageTagged();
    void testSendImageTagged_data();

private Q_SLOTS:
    // Functions called before and after all tests have been run
    void initTestCase();
    void cleanupTestCase();

private:
    // Connection Variables
    DCNC *dcnc;
    QTcpSocket *socket;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;

    // Test variables
    uint8_t *sentImageData;
    size_t sentImageSize;
    uint8_t sentSequenceNumber;
    int32_t sentLatitudeRaw;
    int32_t sentLongitudeRaw;
    int32_t sentAltitudeAbsRaw;
    int32_t sentAltitudeRelRaw;
    uint16_t sentHeadingRaw;
};
#endif // TEST_IMAGE_TAGGED_MESSAGE_HPP
