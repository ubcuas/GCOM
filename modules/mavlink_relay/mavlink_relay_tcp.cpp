//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <memory>
#include <QDebug>
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
    relayStatus = MAVLinkRelayStatus::DISCCONNECTED;
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

    return true;
}

void MAVLinkRelay::connected()
{
     relayStatus = MAVLinkRelayStatus::CONNECTED;
     emit mavlinkRelayConnected();
}

void MAVLinkRelay::disconnected()
{
    relayStatus = MAVLinkRelayStatus::DISCCONNECTED;
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

                default:
                    break;
            }
        }
    }
}
