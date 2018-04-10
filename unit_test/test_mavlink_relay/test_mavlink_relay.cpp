//===================================================================
// Includes
//===================================================================

// System Includes
#include <QString>
#include <QSignalSpy>
#include <QtTest/QtTest>
// GCOM Includes
#include "test_mavlink_relay.hpp"

const QString IP_ADDRESS("127.0.0.1");
const int PORT = 14550;
const uint8_t ARDUPILOT_SYSID = 1;
const uint8_t ARDUPILOT_COMPID = 1;

QTEST_MAIN(TestMAVLinkRelay)

void TestMAVLinkRelay::initTestCase()
{
    // Register types for use in QSignalSpy
    qRegisterMetaType<mavlink_message_t>();

    // Setup ardupilot server and mavlink socket
    QVERIFY(server.listen(QHostAddress(IP_ADDRESS), PORT));
    mavlinkRelay.setup(IP_ADDRESS, PORT);
    QVERIFY(mavlinkRelay.start());

    // Connect socket to server
    QSignalSpy connectionSpy(&server, SIGNAL(newConnection()));
    QVERIFY(connectionSpy.isValid());
    QVERIFY(connectionSpy.wait());
    QCOMPARE(connectionSpy.count(), 1);

    socket = server.nextPendingConnection();
    server.pauseAccepting();

    connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    // Send a heartbeat with ardupilot sysid and compid
    mavlink_message_t outgoingMessage;

    mavlink_msg_heartbeat_pack(ARDUPILOT_SYSID, ARDUPILOT_COMPID, &outgoingMessage, 0, 0, 0, 0, 0);
    QVERIFY(writeData(outgoingMessage));

    QSignalSpy heartbeatSpy(&mavlinkRelay.missionplannerSocket, SIGNAL(readyRead()));
    QVERIFY(heartbeatSpy.isValid());
    QVERIFY(heartbeatSpy.wait());
    QCOMPARE(heartbeatSpy.count(), 1);

    QCOMPARE(mavlinkRelay.targetSysID, ARDUPILOT_SYSID);
    QCOMPARE(mavlinkRelay.targetCompID, ARDUPILOT_COMPID);
}

void TestMAVLinkRelay::cleanupTestCase()
{

}

void TestMAVLinkRelay::readBytes()
{
    mavlink_message_t message;
    mavlink_status_t status;
    uint8_t msgReceived = false;
    QByteArray bufferByteArray;

    // Read all available data from TCP Socket
    bufferByteArray = socket->readAll();
    // Loop through every read byte and pass it through the decode function
    for(auto messageCurrentByte = bufferByteArray.begin();
        messageCurrentByte != bufferByteArray.end(); ++messageCurrentByte)
    {
        // Note, mavlink_parse_char keeps an internal buffer which you pass each
        // byte to, when a message is fully decoded, msgReceived is returned as
        // true. If a message has not fully been decoded, or a corrupt message is
        // received then False is returned.

        // The implementation of mavlink_parse_char is not thread safe when
        // decoding the same comm channel!!
        msgReceived = mavlink_parse_char(MAVLINK_COMM_1,
                                         static_cast<uint8_t>(*messageCurrentByte),
                                         &message, &status);
        if(msgReceived)
        {
            switch (message.msgid)
            {
                case MAVLINK_MSG_ID_DIGICAM_CONTROL:
                    emit receivedTriggerCamera(message);
                case MAVLINK_MSG_ID_MISSION_CLEAR_ALL:
                    emit receivedClearMission(message);
                    break;
                default:
                    break;
            }
        }
    }
}

bool TestMAVLinkRelay::writeData(mavlink_message_t outgoingMessage)
{
    if (socket->state() != QAbstractSocket::ConnectedState)
        return false;

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

    int messageLength = mavlink_msg_to_send_buffer(buffer, &outgoingMessage);
    int bytesWritten = socket->write(reinterpret_cast<const char*>(buffer),
                                                  messageLength);

    if (bytesWritten != messageLength)
        return false;

    return true;
}

void TestMAVLinkRelay::testTriggerCamera()
{
    connect(this, SIGNAL(receivedTriggerCamera(mavlink_message_t)),
            this, SLOT(compareTriggerCamera(mavlink_message_t)));

    QSignalSpy messageSpy(this, SIGNAL(receivedTriggerCamera(mavlink_message_t)));
    QVERIFY(messageSpy.isValid());

    QVERIFY(mavlinkRelay.triggerCamera(1, 1, 100, 1, 1, 1, 0, 0));

    QVERIFY(messageSpy.wait());
    QCOMPARE(messageSpy.count(), 1);
}

void TestMAVLinkRelay::compareTriggerCamera(mavlink_message_t message)
{
    mavlink_digicam_control_t triggerCamera;
    mavlink_msg_digicam_control_decode(&message, &triggerCamera);

    QCOMPARE(triggerCamera.session, (uint8_t) 1);
    QCOMPARE(triggerCamera.zoom_pos, (uint8_t) 1);
    QCOMPARE(triggerCamera.zoom_step, (int8_t) 100);
    QCOMPARE(triggerCamera.focus_lock, (uint8_t) 1);
    QCOMPARE(triggerCamera.shot, (uint8_t) 1);
    QCOMPARE(triggerCamera.command_id, (uint8_t) 1);
    QCOMPARE(triggerCamera.extra_param, (uint8_t) 0);
    QCOMPARE(triggerCamera.extra_value, (float) 0);

    disconnect(this, SIGNAL(receivedTriggerCamera(mavlink_message_t)),
               this, SLOT(compareTriggerCamera(mavlink_message_t)));
}

void TestMAVLinkRelay::testClearMission()
{
    connect(this, SIGNAL(receivedClearMission(mavlink_message_t)),
            this, SLOT(compareClearMission(mavlink_message_t)));

    QSignalSpy messageSpy(this, SIGNAL(receivedClearMission(mavlink_message_t)));
    QVERIFY(messageSpy.isValid());

    QVERIFY(mavlinkRelay.clearMission());

    QVERIFY(messageSpy.wait());
    QCOMPARE(messageSpy.count(), 1);
}

void TestMAVLinkRelay::compareClearMission(mavlink_message_t message)
{
    mavlink_mission_clear_all_t clearAll;
    mavlink_msg_mission_clear_all_decode(&message, &clearAll);

    QCOMPARE(clearAll.target_system, ARDUPILOT_SYSID);
    QCOMPARE(clearAll.target_component, ARDUPILOT_COMPID);

    mavlink_message_t outgoingMessage;

    mavlink_msg_mission_ack_pack(ARDUPILOT_SYSID, ARDUPILOT_COMPID, &outgoingMessage,
                                 message.sysid, message.compid, 0);

    QVERIFY(writeData(outgoingMessage));

    disconnect(this, SIGNAL(receivedClearMission(mavlink_message_t)),
            this, SLOT(compareClearMission(mavlink_message_t)));
}

