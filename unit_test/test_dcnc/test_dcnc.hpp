#ifndef TEST_DCNC_HPP
#define TEST_DCNC_HPP

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

Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(uint16_t)
Q_DECLARE_METATYPE(UASMessage::MessageID)
Q_DECLARE_METATYPE(CapabilitiesMessage::Capabilities)
Q_DECLARE_METATYPE(CommandMessage::Commands)
Q_DECLARE_METATYPE(ResponseMessage::ResponseCodes)
Q_DECLARE_METATYPE(std::shared_ptr<ImageUntaggedMessage>)
Q_DECLARE_METATYPE(std::shared_ptr<ImageTaggedMessage>)

class TestDCNC: public QObject
{
    Q_OBJECT

signals:
    void receivedRequest(UASMessage::MessageID request);
    void receivedCommand(CommandMessage::Commands command);

public slots:
    // Socket slots
    void socketConnected();
    void socketDisconnected();

    // Receive handlers
    void handleClientData();
    void handleClientMessage(std::shared_ptr<UASMessage> message);

    // Message compares
    void compareRequestMessage(UASMessage::MessageID request);
    void compareSendUASMessage(CommandMessage::Commands command);
    void compareStartImageRelay(CommandMessage::Commands command);
    void compareStopImageRelay(CommandMessage::Commands command);
    void compareHandleClientMessage_systemInfo(QString systemID, uint16_t versionNumber,
                                               bool dropped);
    void compareHandleClientMessage_capabilities(CapabilitiesMessage::Capabilities capability);
    void compareHandleClientMessage_response(CommandMessage::Commands command,
                                             ResponseMessage::ResponseCodes responseCode);
    void compareHandleClientMessage_imageUntagged(std::shared_ptr<ImageUntaggedMessage> image);
    void compareHandleClientMessage_imageTagged(std::shared_ptr<ImageTaggedMessage> image);

private slots:
    // Functions to be tested
    void testSendUASMessage_data();
    void testSendUASMessage();

    void testStartImageRelay();
    void testStopImageRelay();

    void testHandleClientMessage_systemInfo_data();
    void testHandleClientMessage_systemInfo();

    void testHandleClientMessage_capabilities_data();
    void testHandleClientMessage_capabilities();

    void testHandleClientMessage_response_data();
    void testHandleClientMessage_response();

    void testHandleClientMessage_imageUntagged_data();
    void testHandleClientMessage_imageUntagged();

    void testHandleClientMessage_imageTagged_data();
    void testHandleClientMessage_imageTagged();

    void testCancelConnection();
    void testReconnect();

private Q_SLOTS:
    // Project initialization and cleanup
    void initTestCase();
    void cleanupTestCase();

private:
    // Connection Variables
    DCNC *dcnc;
    QTcpSocket *socket;
    QTcpSocket *socketOther;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;

    // Test variables
    std::string sentSystemID;
    uint16_t sentVersionNumber;
    bool sentDropped;
    CommandMessage::Commands sentCommand;
    ResponseMessage::ResponseCodes sentResponseCode;
    CapabilitiesMessage::Capabilities sentCapability;
    uint8_t sentSequenceNumber;
    size_t sentImageSize;
    uint8_t *sentImageData;
    int32_t sentLatitude;
    int32_t sentLongitude;
    int32_t sentAltitudeAbs;
    int32_t sentAltitudeRel;
    uint16_t sentHeading;
};

#endif // TEST_DCNC_HPP
