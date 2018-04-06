//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <memory>
#include <QDebug>
#include <QDateTime>
// GCOM Includes
#include "mavlink_relay_tcp.hpp"
#include "../Mavlink/ardupilotmega/mavlink.h"

//===================================================================
// Class Definitions
//===================================================================
MAVLinkRelay::MAVLinkRelay()
{
    // Set defualt values
    ipaddress = "127.0.0.1";
    port = 14550;
    relayStatus = MAVLinkRelayStatus::DISCONNECTED;
    // Build the sockets and connect the signals/slots
    connect(&missionplannerSocket, SIGNAL(connected()),
            this, SLOT(connected()));
    connect(&missionplannerSocket, SIGNAL(disconnected()),
            this, SLOT(disconnected()));
    connect(&missionplannerSocket, SIGNAL(readyRead()),
            this, SLOT(readBytes()));
    connect(&missionplannerSocket, &QTcpSocket::stateChanged,
            this, &MAVLinkRelay::statusChanged);
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

    systemSysID = 2;
    systemCompID = 1;

    return true;
}

void MAVLinkRelay::connected()
{
     relayStatus = MAVLinkRelayStatus::CONNECTED;
     emit mavlinkRelayConnected();
}

void MAVLinkRelay::disconnected()
{
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
                    missionPlannerSysID = message.sysid;
                    missionPlannerCompID = message.compid;
                    break;
                }

                case MAVLINK_MSG_ID_MISSION_REQUEST:
                {
                    mavlink_mission_request_t mission_request;
                    mavlink_msg_mission_request_decode(&message, &mission_request);
                    handleMissionRequest(mission_request.seq);
                    break;
                }

                case MAVLINK_MSG_ID_MISSION_ACK:
                {
                    mavlink_mission_ack_t mission_ack;
                    mavlink_msg_mission_ack_decode(&message, &mission_ack);
                    qDebug() << "ack received:" << mission_ack.type;
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
                                     missionPlannerSysID, missionPlannerCompID,
                                     session, zoom_pos, zoom_step, focus_lock, shot,
                                     command_id, extra_param, extra_value);

    if (!writeData(outgoingMessage))
        return false;

    return true;
}

void MAVLinkRelay::writeWayPoints(QList<InteropMission::Waypoint> waypoints)
{   
    mavlink_message_t outgoingMessage;

    waypointList = waypoints;

    mavlink_msg_mission_count_pack(systemSysID, systemCompID, &outgoingMessage,
                                   missionPlannerSysID, missionPlannerCompID,
                                   waypointList.size());

    if(!writeData(outgoingMessage))
        qDebug() << "count failed";
    else
        qDebug() << "sent" << waypointList.size();
}

void MAVLinkRelay::handleMissionRequest(uint16_t seq)
{
    mavlink_message_t outgoingMessage;

    mavlink_msg_mission_item_pack(systemSysID, systemCompID, &outgoingMessage,
                                  missionPlannerSysID, missionPlannerCompID,
                                  seq, MAV_FRAME_GLOBAL, MAV_CMD_NAV_WAYPOINT,
                                  0, 1, 0, 0, 0, 0,
                                  waypointList.at(seq).latitude,
                                  waypointList.at(seq).longitude,
                                  waypointList.at(seq).altitudeMsl);

    if(!writeData(outgoingMessage))
        qDebug() << "failed to send waypoint";
}

void MAVLinkRelay::clearWayPoints()
{
    mavlink_message_t outgoingMessage;

    mavlink_msg_mission_clear_all_pack(systemSysID, systemCompID, &outgoingMessage,
                                       missionPlannerSysID, missionPlannerCompID);

    writeData(outgoingMessage);
}
