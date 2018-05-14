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
#include <QElapsedTimer>
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

    enum class FlightModeIndex : int
    {
        STABILIZE           = 0,
        AUTO                = 1,
        GUIDED              = 2,
        LOITER              = 3,
        RTL                 = 4,
        ALTHOLD             = 5,
        MANUAL              = 6,
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
     * \brief setFlightMode, sets flight mode for drone (eg. auto, loiter, stabilize...)
     * \param mode, mode to be set
     * \return command successfully sent or not
     */
    bool setFlightMode(int mode);
    /*!
     * \brief changeSpeed, changes drone speed in m/s
     * \param speed, speed in m/s
     * \return command successfully sent or not
     */
    bool changeSpeed(float speed);

    /*!
     * \brief arm, arms drone
     * \return command successfully sent or not
     */
    bool arm();
    /*!
     * \brief missionStart, starts mission, setting mode to auto
     * \return command successfully sent or not
     */
    bool missionStart();
    /*!
     * \brief takeoff, commands multirotor (no fixed wings) to takeoff
     * \param takeoffAlt, takeoff altitude in m
     * \return command successfully sent or not
     */
    bool takeoff(float takeoffAlt);
    /*!
     * \brief land, commands multirotor (no fixed wings) to land
     * \return command successfully sent or not
     */
    bool land();

    /*!
     * \brief writeMission, write mission (takeoff, waypoints, land) to drone
     * \param takeoffAlt, takeoff altitude
     * \param waypoints, list of waypoints to be sent
     * \param climb, climb angle for fixed wings
     * \return true if written successfully
     */
    bool writeMission(float takeoffAlt, QList<InteropMission::Waypoint> waypoints, float climb=20);
    /*!
     * \brief clearMission, clears all waypoints
     */
    bool clearMission();

signals:
    void mavlinkRelayGPSInfo(std::shared_ptr<mavlink_global_position_int_t> gpsSignal);
    void mavlinkRelayCameraInfo(std::shared_ptr<mavlink_camera_feedback_t> cameraSignal);
    void mavlinkRelayConnected();
    void mavlinkRelayDisconnected();

    void receivedMissionAck(uint8_t type);
    void receivedMissionRequest(uint16_t seq);
    void receivedCommandAck(uint8_t result);
    /*!
     * \brief mavlinkRelayMissionCommandSuccess
     * \details true if mission was sent, received, and acknowledged without error
     *          false if mission had any error
     */
    void mavlinkMissionCommandSuccess(bool);
    /*!
     * \brief mavlinkRelayCommandSuccess
     * \details true if command was sent, received, and acknowledged without error
     *          false if command had any error
     */
    void mavlinkCommandSuccess(bool);

private slots:
    void connected();
    void disconnected();
    void statusChanged(QAbstractSocket::SocketState socketState);
    void readBytes();

    void handleMissionRequest(uint16_t seq);
    void handleMissionAck(uint8_t type);
    void handleCommandAck(uint8_t result);

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

    // Returns the correct code used by ardupilot for each flight mode
    int handleMultiRotorFlightMode(int mode);
    int handleFixedWingFlightMode(int mode);

    // Private Member Variables
    QTcpSocket missionplannerSocket;
    MAVLinkRelayStatus relayStatus = MAVLinkRelayStatus::DISCONNECTED;
    QString ipaddress = "127.0.0.1";
    qint16 port = 14550;
    mavlink_status_t lastStatus;

    // sys and comp id of ground
    uint8_t systemSysID = 2;
    uint8_t systemCompID = 1;
    // sys and comp id of drone
    uint8_t targetSysID = 1;
    uint8_t targetCompID = 1;
    uint8_t vehicleType = 0;

    MAVLinkRelaySendingStatus sendingStatus = MAVLinkRelaySendingStatus::READY;
    QElapsedTimer waypointRequestTimer;

    QList<InteropMission::Waypoint> waypointList;
    float takeoffAltitude;
    float climbAngle;
};
#endif // MAVLINKRELAYTCP_HPP
