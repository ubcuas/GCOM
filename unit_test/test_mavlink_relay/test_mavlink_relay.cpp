//===================================================================
// Includes
//===================================================================

// System Includes
#include <QString>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <QList>
// GCOM Includes
#include "test_mavlink_relay.hpp"

#define PACK_LAT_LON(x) (int32_t) (x*1e7)

const QString IP_ADDRESS("127.0.0.1");
const int PORT = 14550;
const uint8_t ARDUPILOT_SYSID = 1;
const uint8_t ARDUPILOT_COMPID = 1;
const QList<InteropMission::Waypoint> WAYPOINTS({
            {40.0, 49.2623646, -123.2473218, 0},
            {40.0, 49.2620145, -123.2484162, 1},
            {40.0, 49.2624661, -123.2486147, 2},
            {40.0, -69.2624661, 123.2486147, 3}
        });

// Number of items in WAYPOINTS + 3
const int WAYPOINTS_SIZE = 7;

const float TAKEOFF_ALT = 40;

QTEST_MAIN(TestMAVLinkRelay)

void TestMAVLinkRelay::initTestCase()
{
    // Register types for use in QSignalSpy
    qRegisterMetaType<mavlink_message_t>();
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");

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

    waypointNumber = 0;
}

void TestMAVLinkRelay::cleanupTestCase()
{
    mavlinkRelay.stop();
    disconnect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));
    server.close();
    socket->close();
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
                    break;
                case MAVLINK_MSG_ID_MISSION_COUNT:
                    emit receivedMissionCount(message);
                    break;
                case MAVLINK_MSG_ID_MISSION_ITEM:
                case MAVLINK_MSG_ID_MISSION_ITEM_INT:
                    emit receivedMissionItem(message);
                    break;
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

void TestMAVLinkRelay::testWriteMission()
{
    // Write number of waypoints to be sent
    connect(this, SIGNAL(receivedMissionCount(mavlink_message_t)),
            this, SLOT(handleMissionCount(mavlink_message_t)));

    mavlinkRelay.writeMission(TAKEOFF_ALT, WAYPOINTS);

    QSignalSpy countSpy(this, SIGNAL(receivedMissionCount(mavlink_message_t)));
    QVERIFY(countSpy.isValid());
    QVERIFY(countSpy.wait());
    QCOMPARE(countSpy.count(), 1);

    // Request and send waypoints
    connect(this, SIGNAL(receivedMissionItem(mavlink_message_t)),
            this, SLOT(compareMissionItem(mavlink_message_t)));

    for (int i = 0; i < waypointCount; i++)
    {
        QSignalSpy missionItemSpy(this, SIGNAL(receivedMissionItem(mavlink_message_t)));
        QVERIFY(missionItemSpy.isValid());
        QVERIFY(missionItemSpy.wait());
        QCOMPARE(missionItemSpy.count(), 1);
    }
}

void TestMAVLinkRelay::handleMissionCount(mavlink_message_t message)
{
    mavlink_message_t outgoingMessage;

    // Ensure correct number of waypoints is sent
    waypointCount = mavlink_msg_mission_count_get_count(&message);
    QCOMPARE(waypointCount, (uint16_t) (WAYPOINTS.size()+3));

    // Request for first waypoint
    mavlink_msg_mission_request_pack(ARDUPILOT_SYSID, ARDUPILOT_COMPID,
                                     &outgoingMessage, message.sysid,
                                     message.compid, waypointNumber);
    writeData(outgoingMessage);

    // Ensure first waypoint request is received
    QSignalSpy missionRequestSpy(&mavlinkRelay, SIGNAL(receivedMissionRequest(uint16_t)));
    QVERIFY(missionRequestSpy.isValid());
    QVERIFY(missionRequestSpy.wait());
    QCOMPARE(missionRequestSpy.count(), 1);

    disconnect(this, SIGNAL(receivedMissionCount(mavlink_message_t)),
               this, SLOT(handleMissionCount(mavlink_message_t)));
}

void TestMAVLinkRelay::compareMissionItem(mavlink_message_t message)
{
    mavlink_mission_item_int_t missionItem;
    mavlink_msg_mission_item_int_decode(&message, &missionItem);

    mavlink_message_t outgoingMessage;

    // Ensure correct waypoint
    QCOMPARE(missionItem.seq, (uint16_t) waypointNumber);

    switch(waypointNumber)
    {
        // First waypoint is replaced with home waypoint, so is unimportant
        case 0:
            break;
        case 1:
            QCOMPARE(missionItem.command, (uint16_t) MAV_CMD_NAV_TAKEOFF);
            QCOMPARE(missionItem.z, TAKEOFF_ALT);
            break;
        case WAYPOINTS_SIZE-1:
            QCOMPARE(missionItem.command, (uint16_t) MAV_CMD_NAV_LAND);
            QCOMPARE(missionItem.x, PACK_LAT_LON(WAYPOINTS.last().latitude));
            QCOMPARE(missionItem.y, PACK_LAT_LON(WAYPOINTS.last().longitude));
            break;
        default:
            QCOMPARE(missionItem.command, (uint16_t) MAV_CMD_NAV_WAYPOINT);
            QCOMPARE(missionItem.x, PACK_LAT_LON(WAYPOINTS.at(waypointNumber-2).latitude));
            QCOMPARE(missionItem.y, PACK_LAT_LON(WAYPOINTS.at(waypointNumber-2).longitude));
            QCOMPARE(missionItem.z, WAYPOINTS.at(waypointNumber-2).altitudeMsl);
    }

    if (waypointNumber < WAYPOINTS_SIZE-1)
    {
        // Request next waypoint
        mavlink_msg_mission_request_pack(ARDUPILOT_SYSID, ARDUPILOT_COMPID,
                                         &outgoingMessage, message.sysid, message.compid,
                                         ++waypointNumber);
        QVERIFY(writeData(outgoingMessage));

        QSignalSpy missionRequestSpy(&mavlinkRelay, SIGNAL(receivedMissionRequest(uint16_t)));
        QVERIFY(missionRequestSpy.isValid());
        QVERIFY(missionRequestSpy.wait());
        QCOMPARE(missionRequestSpy.count(), 1);
    }
    else
    {
        // Acknowledge mission has been received
        mavlink_msg_mission_ack_pack(ARDUPILOT_SYSID, ARDUPILOT_COMPID,
                                     &outgoingMessage, message.sysid, message.compid,
                                     MAV_MISSION_ACCEPTED);
        QVERIFY(writeData(outgoingMessage));

        QSignalSpy missionCmdSpy(&mavlinkRelay, SIGNAL(mavlinkRelayCommandSuccess(bool)));
        QVERIFY(missionCmdSpy.isValid());
        QVERIFY(missionCmdSpy.wait());
        QCOMPARE(missionCmdSpy.count(), 1);
        QVERIFY(missionCmdSpy.takeFirst().at(0).toBool());

        disconnect(this, SIGNAL(receivedMissionItem(mavlink_message_t)),
                this, SLOT(compareMissionItem(mavlink_message_t)));
    }
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

    QSignalSpy missionAckSpy(&mavlinkRelay.missionplannerSocket,
                             SIGNAL(readyRead()));
    QVERIFY(missionAckSpy.isValid());
    QVERIFY(missionAckSpy.wait());
    QCOMPARE(missionAckSpy.count(), 1);

    disconnect(this, SIGNAL(receivedClearMission(mavlink_message_t)),
            this, SLOT(compareClearMission(mavlink_message_t)));
}

void TestMAVLinkRelay::testDisconnection()
{
    // start writing misson
    mavlinkRelay.writeMission(TAKEOFF_ALT, WAYPOINTS);

    // Disconnect
    mavlinkRelay.missionplannerSocket.disconnectFromHost();
    QCOMPARE(mavlinkRelay.sendingStatus, MAVLinkRelay::MAVLinkRelaySendingStatus::SENDING);

    QSignalSpy socketDisconnectedSpy(&mavlinkRelay.missionplannerSocket, SIGNAL(disconnected()));
    QVERIFY(socketDisconnectedSpy.isValid());
    QVERIFY(socketDisconnectedSpy.wait());
    QCOMPARE(socketDisconnectedSpy.count(), 1);

    // Reconnect before timer times out - should continue sending
    mavlinkRelay.setup(IP_ADDRESS, PORT);
    QVERIFY(mavlinkRelay.start());
    QCOMPARE(mavlinkRelay.sendingStatus, MAVLinkRelay::MAVLinkRelaySendingStatus::SENDING);

    QSignalSpy socketConnectedSpy(&mavlinkRelay.missionplannerSocket, SIGNAL(connected()));
    QVERIFY(socketConnectedSpy.isValid());
    QVERIFY(socketConnectedSpy.wait());
    QCOMPARE(socketConnectedSpy.count(), 1);

    // Reset status
    mavlinkRelay.sendingStatus = MAVLinkRelay::MAVLinkRelaySendingStatus::READY;
    // start writing misson
    mavlinkRelay.writeMission(TAKEOFF_ALT, WAYPOINTS);

    // Disconnect
    mavlinkRelay.missionplannerSocket.disconnectFromHost();
    QCOMPARE(mavlinkRelay.sendingStatus, MAVLinkRelay::MAVLinkRelaySendingStatus::SENDING);

    QVERIFY(socketDisconnectedSpy.wait());
    QCOMPARE(socketDisconnectedSpy.count(), 2);

    // Timeout timer
    QTest::qWait(7000);

    // Reconnect - should not resume sending
    mavlinkRelay.setup(IP_ADDRESS, PORT);
    QVERIFY(mavlinkRelay.start());

    QSignalSpy commandSuccessSpy(&mavlinkRelay, SIGNAL(mavlinkRelayCommandSuccess(bool)));
    QVERIFY(commandSuccessSpy.isValid());
    QVERIFY(commandSuccessSpy.wait());
    QCOMPARE(commandSuccessSpy.count(), 1);
    QCOMPARE(mavlinkRelay.sendingStatus, MAVLinkRelay::MAVLinkRelaySendingStatus::READY);
}

