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
#include "../Mavlink/ardupilotmega/mavlink.h"

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
    Q_OBJECT

public:

    enum class MAVLinkRelayStatus : int
    {
        CONNECTED           = 0,
        DISCCONNECTED       = 1,
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

signals:
    void mavlinkRelayGPSInfo(std::shared_ptr<mavlink_global_position_int_t> gpsSignal);
    void mavlinkRelayCameraInfo(std::shared_ptr<mavlink_camera_feedback_t> cameraSignal);
    void mavlinkRelayConnected();
    void mavlinkRelayDisconnected();

private slots:
    void connected();
    void disconnected();
    void statusChanged(QAbstractSocket::SocketState socketState);
    void readBytes();

private:
    // Private Member Variables
    QTcpSocket missionplannerSocket;
    MAVLinkRelayStatus relayStatus;
    QString ipaddress;
    qint16 port;
    mavlink_status_t lastStatus;
};
#endif // MAVLINKRELAYTCP_HPP
