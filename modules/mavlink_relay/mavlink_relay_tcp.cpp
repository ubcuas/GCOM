//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
// GCOM Includes
#include "mavlink_relay_tcp.hpp"
#include "../Mavlink/ardupilotmega/mavlink.h"

#define PACK_LAT_LON(x) qRound(x*1e7) // float -> int32_t

const int WAYPOINT_REQUEST_TIMEOUT = 7000; // ms

//===================================================================
// Class Definitions
//===================================================================
MAVLinkRelay::MAVLinkRelay()
{
    // Build the sockets and connect the signals/slots
    connect(&missionplannerSocket, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(&missionplannerSocket, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(&missionplannerSocket, SIGNAL(readyRead()),
            this, SLOT(readBytes()));
    connect(&missionplannerSocket, &QTcpSocket::stateChanged,
            this, &MAVLinkRelay::statusChanged);
    connect(this, SIGNAL(receivedMissionRequest(uint16_t)),
            this, SLOT(handleMissionRequest(uint16_t)));
    connect(this, SIGNAL(receivedMissionAck(uint8_t)),
            this, SLOT(handleMissionAck(uint8_t)));
}

void MAVLinkRelay::stop()
{
    missionplannerSocket.disconnectFromHost();
}

void MAVLinkRelay::setup(QString ipAddress, qint16 port)
{
    // If the socket is unconnected or is not in the process of closing then we
    // close it
    if (missionplannerSocket.state() != QAbstractSocket::UnconnectedState)
        stop();
    // Save connection Parameters
    this->ipaddress = ipAddress;
    this->port = port;
}

MAVLinkRelay::MAVLinkRelayStatus MAVLinkRelay::status()
{
    return relayStatus;
}

bool MAVLinkRelay::start()
{
    // If the socket if is not unconnected then return false
    if (missionplannerSocket.state() != QAbstractSocket::UnconnectedState)
        return false;
    // Attempt to connect to the specified host
    missionplannerSocket.connectToHost(ipaddress, port);

    return true;
}

void MAVLinkRelay::connected()
{
    /* If disconnection is longer than WAYPOINT_REQUEST_TIMEOUT, ardupilot
     * times out and stop rerequesting waypoints on reconnection, so command fails
     */
    if (sendingStatus == MAVLinkRelaySendingStatus::SENDING &&
        waypointRequestTimer.elapsed() >= WAYPOINT_REQUEST_TIMEOUT)
    {
        sendingStatus = MAVLinkRelaySendingStatus::READY;
        emit mavlinkRelayCommandSuccess(false);
    }

    // Stop timer
    waypointRequestTimer.invalidate();

    relayStatus = MAVLinkRelayStatus::CONNECTED;
    emit mavlinkRelayConnected();
}

void MAVLinkRelay::disconnected()
{
    // Start timer
    if (sendingStatus == MAVLinkRelaySendingStatus::SENDING)
        waypointRequestTimer.start();

    relayStatus = MAVLinkRelayStatus::DISCONNECTED;
    emit mavlinkRelayDisconnected();
}

void MAVLinkRelay::statusChanged(QAbstractSocket::SocketState socketState)
{
    // Handle the connecting case
    if ((socketState == QAbstractSocket::SocketState::HostLookupState)
            || socketState == QAbstractSocket::SocketState::ConnectingState)
    {
        relayStatus = MAVLinkRelayStatus::CONNECTING;
    }
    else if ((relayStatus == MAVLinkRelayStatus::CONNECTING)
             && (socketState == QAbstractSocket::SocketState::UnconnectedState))
        disconnected();
}

void MAVLinkRelay::readBytes()
{
    mavlink_message_t message;
    mavlink_status_t status;
    uint8_t msgReceived = false;
    QByteArray bufferByteArray;

    // Read all available data from TCP Socket
    bufferByteArray = missionplannerSocket.readAll();
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
                case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
                {
                    std::shared_ptr<mavlink_global_position_int_t> gpsPacketPointer(
                                new mavlink_global_position_int_t);
                    mavlink_msg_global_position_int_decode(&message,gpsPacketPointer.get());
                    emit mavlinkRelayGPSInfo(gpsPacketPointer);
                    break;
                }

                case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
                {
                    std::shared_ptr<mavlink_camera_feedback_t> cameraPacketPointer(
                                new mavlink_camera_feedback_t);
                    mavlink_msg_camera_feedback_decode(&message, cameraPacketPointer.get());
                    emit mavlinkRelayCameraInfo(cameraPacketPointer);
                    break;
                }

                case MAVLINK_MSG_ID_HEARTBEAT:
                {
                    targetSysID = message.sysid;
                    targetCompID = message.compid;
                    break;
                }

                case MAVLINK_MSG_ID_MISSION_REQUEST:
                {
                    emit receivedMissionRequest(mavlink_msg_mission_request_get_seq(&message));
                    break;
                }

                case MAVLINK_MSG_ID_MISSION_ACK:
                {
                    emit receivedMissionAck(mavlink_msg_mission_ack_get_type(&message));
                    break;
                }
                default:
                    break;
            }
        }
    }
}

bool MAVLinkRelay::writeData(mavlink_message_t outgoingMessage)
{
    if (missionplannerSocket.state() != QAbstractSocket::ConnectedState)
        return false;

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

    int messageLength = mavlink_msg_to_send_buffer(buffer, &outgoingMessage);
    int bytesWritten = missionplannerSocket.write(reinterpret_cast<const char*>(buffer),
                                                  messageLength);

    if (bytesWritten != messageLength)
        return false;

    return true;
}

bool MAVLinkRelay::triggerCamera(uint8_t session, uint8_t zoom_pos, int8_t zoom_step,
                                 uint8_t focus_lock, uint8_t shot, uint8_t command_id,
                                 uint8_t extra_param, float extra_value)
{
    mavlink_message_t outgoingMessage;

    mavlink_msg_digicam_control_pack(systemSysID, systemCompID, &outgoingMessage,
                                     targetSysID, targetCompID,
                                     session, zoom_pos, zoom_step, focus_lock, shot,
                                     command_id, extra_param, extra_value);

    if (!writeData(outgoingMessage))
        return false;

    return true;
}

bool MAVLinkRelay::clearMission()
{
    if (sendingStatus == MAVLinkRelaySendingStatus::SENDING)
        return false;

    mavlink_message_t outgoingMessage;

    mavlink_msg_mission_clear_all_pack(systemSysID, systemCompID, &outgoingMessage,
                                       targetSysID, targetCompID);

    if (!writeData(outgoingMessage))
        return false;

    return true;
}

void MAVLinkRelay::writeMission(float takeoffAlt, QList<InteropMission::Waypoint> waypoints)
{   
    if (sendingStatus == MAVLinkRelaySendingStatus::SENDING ||
        missionplannerSocket.state() != QAbstractSocket::ConnectedState)
        return;

    mavlink_message_t outgoingMessage;

    waypointList = waypoints;
    takeoffAltitude = takeoffAlt;

    // Tell the drone the number of waypoints to be sent
    // 3 extra waypoints to be sent - home, takeoff, landing
    mavlink_msg_mission_count_pack(systemSysID, systemCompID, &outgoingMessage,
                                   targetSysID, targetCompID, waypointList.size()+3);

    if (writeData(outgoingMessage))
        sendingStatus = MAVLinkRelaySendingStatus::SENDING;
}

void MAVLinkRelay::handleMissionRequest(uint16_t seq)
{
    if (sendingStatus != MAVLinkRelaySendingStatus::SENDING)
        return;

    mavlink_message_t outgoingMessage;

    // Last waypoint is a land command, land at current location
    if (seq == waypointList.size()+2)
    {
        mavlink_msg_mission_item_int_pack(systemSysID, systemCompID, &outgoingMessage,
                                          targetSysID, targetCompID, seq,
                                          MAV_FRAME_GLOBAL, MAV_CMD_NAV_LAND,
                                          0, 0, 0, 0, 0, 0,
                                          PACK_LAT_LON(waypointList.at(seq-3).latitude),
                                          PACK_LAT_LON(waypointList.at(seq-3).longitude),
                                          0);
        writeData(outgoingMessage);
        return;
    }

    switch(seq)
    {
        // First waypoint is always replaced with the home location,
        // so send a blank waypoint
        case 0:
        {
            mavlink_msg_mission_item_int_pack(systemSysID, systemCompID, &outgoingMessage,
                                              targetSysID, targetCompID, seq,
                                              MAV_FRAME_GLOBAL, MAV_CMD_NAV_WAYPOINT,
                                              0, 1, 0, 0, 0, 0, 0, 0, 0);
            break;
        }
        // Second waypoint is the takeoff command
        case 1:
        {
            mavlink_msg_mission_item_int_pack(systemSysID, systemCompID, &outgoingMessage,
                                              targetSysID, targetCompID, seq,
                                              MAV_FRAME_GLOBAL, MAV_CMD_NAV_TAKEOFF,
                                              0, 1, 0, 0, 0, 0, 0, 0, takeoffAltitude);
            break;
        }
        // Any other is a regular waypoint
        default:
        {
            mavlink_msg_mission_item_int_pack(systemSysID, systemCompID, &outgoingMessage,
                                              targetSysID, targetCompID, seq,
                                              MAV_FRAME_GLOBAL, MAV_CMD_NAV_WAYPOINT,
                                              0, 1, 0, 0, 0, 0,
                                              PACK_LAT_LON(waypointList.at(seq-2).latitude),
                                              PACK_LAT_LON(waypointList.at(seq-2).longitude),
                                              waypointList.at(seq-2).altitudeMsl);
        }
    }

    writeData(outgoingMessage);
}

void MAVLinkRelay::handleMissionAck(uint8_t type)
{
    if (sendingStatus != MAVLinkRelaySendingStatus::SENDING)
        return;

    switch(type)
    {
        // This error does not affect overall command, so ignore
        case MAV_MISSION_INVALID_SEQUENCE:
        break;

        case MAV_MISSION_ACCEPTED:
        {
            sendingStatus = MAVLinkRelaySendingStatus::READY;
            emit mavlinkRelayCommandSuccess(true);
            break;
        }
        // All other acknowledgement types are errors
        default:
        {
            sendingStatus = MAVLinkRelaySendingStatus::READY;
            emit mavlinkRelayCommandSuccess(false);
        }
    }
}
