#ifndef MAVLINKRELAYTCP_HPP
#define MAVLINKRELAYTCP_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include <string>
#include <memory>
#include <QDataStream>
#include "../Mavlink/ardupilotmega/mavlink.h"
#include "modules/uas_interop_system/InteropObjects/interop_mission.hpp"

//===================================================================
// Class Declarations
//===================================================================
/*!
 * \brief The MAVLinkRelay class is designed to connect to Mission Planner, or
 *        any other TCP mavlink messagesource and relay the relevant messages to
 *        the rest of the program using relevant signals
 * \author Grant Nicol
 * \author Zeyad Tamimi
 */
class MAVLinkRelay : public QObject
{
    friend class TestMAVLinkRelay;

    Q_OBJECT

public:

    enum class MAVLinkRelayStatus : int
    {
        CONNECTED           = 0,
        DISCONNECTED        = 1,
        CONNECTING          = 2,
    };
    // Public Methods
    MAVLinkRelay();

    /*!
     * \brief setup creates the TCP socket with all the specified settings and
     *        connects all the internal slots. Multiple calls to setup will result
     *        in stop being called followed by setup
     * \param [in] ipAddress, the IP address of the TCP MAVLink stream
     * \param [in] port, the port that the TCP MAVLink stream is broadcasting on
     */
    void setup(QString ipAddress, qint16 port);

    /*!
     * \brief start, attempts to open the TCP socket and connect to the MAVLink
     *        stream
     * \return True if the connection was successful. False if setup was never
     *         called
     */
    bool start();

    /*!
     * \brief stop, tearsdown the TCP socket
     */
    void stop();

    /*!
     * \brief status
     * \return the status of the MAVLinkRelay as a MAVLinkRelayStatus
     */
    MAVLinkRelayStatus status();


    /*!
     * \brief triggerCamera
     * \param session, 0: stop, 1: start or keep it up //Session control e.g. show/hide lens
     * \param zoom_pos, 1 to N //Zoom's absolute position (0 means ignore)
     * \param zoom_step, -100 to 100 //Zooming step value to offset zoom from the current position
     * \param focus_lock, 0: unlock focus or keep unlocked,
     *                    1: lock focus or keep locked, 3: re-lock focus
     * \param shot, 0: ignore, 1: shot or start filming
     * \param command_id, Command Identity (incremental loop: 0 to 255)
     *                    A command sent multiple times will be executed or pooled just once
     * \param extra_param, Extra parameters enumeration (0 means ignore)
     * \param extra_value, Correspondent value to given extra_param
     * \return true if message sent sucessfully, false if unsuccessfully
     */
    bool triggerCamera(uint8_t session, uint8_t zoom_pos, int8_t zoom_step,
                       uint8_t focus_lock, uint8_t shot, uint8_t command_id,
                       uint8_t extra_param, float extra_value);

    /*!
     * \brief writeMission, write mission (takeoff, waypoints, land) to drone
     * \param takeoffAlt, takeoff altitude
     * \param waypoints, list of waypoints
     */
    void writeMission(float takeoffAlt, QList<InteropMission::Waypoint> waypoints);
    /*!
     * \brief clearMission, clears all waypoints
     */
    bool clearMission();

signals:
    void mavlinkRelayGPSInfo(std::shared_ptr<mavlink_global_position_int_t> gpsSignal);
    void mavlinkRelayCameraInfo(std::shared_ptr<mavlink_camera_feedback_t> cameraSignal);
    void mavlinkRelayConnected();
    void mavlinkRelayDisconnected();

    void missionAck(uint8_t type);

private slots:
    void connected();
    void disconnected();
    void statusChanged(QAbstractSocket::SocketState socketState);
    void readBytes();

public slots:
    void changeAutoReconnect(bool autoReconnect);

private:
    enum class MAVLinkRelaySendingStatus : int
    {
        READY               = 0,
        SENDING             = 1
    };

    /*!
     * \brief writeData, writes data to the socket
     * \param outgoingMessage, message to write
     * \return true if successful, false if unsuccessful
     */
    bool writeData(mavlink_message_t outgoingMessage);

    void handleMissionRequest(uint16_t seq);
    void handleMissionAck(uint8_t type);

    // Private Member Variables
    QTcpSocket missionplannerSocket;
    MAVLinkRelayStatus relayStatus;
    QString ipaddress;
    qint16 port;
    mavlink_status_t lastStatus;

    bool autoReconnect;

    uint8_t systemSysID;
    uint8_t systemCompID;
    uint8_t targetSysID;
    uint8_t targetCompID;

    MAVLinkRelaySendingStatus sendingStatus;

    QList<InteropMission::Waypoint> waypointList;
    float takeoffAltitude;
};
#endif // MAVLINKRELAYTCP_HPP
