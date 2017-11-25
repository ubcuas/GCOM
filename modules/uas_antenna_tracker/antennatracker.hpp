#ifndef ANTENNATRACKER_H
#define ANTENNATRACKER_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QDebug>
#include <QList>
#include <vector>
#include <math.h>
#include <atomic>
#include "../Mavlink/ardupilotmega/mavlink.h"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"
#include "modules/uas_message/uas_message.hpp"
#include "modules/uas_message/request_message.hpp"
#include "modules/uas_message/uas_message_serial_framer.hpp"

/*!
 * \brief The AntennaTracker class
 */
class AntennaTracker : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief AntennaTracker is the constructor where pointers are declared as null
     */
    AntennaTracker();
    ~AntennaTracker();

    /*!
     * \brief The DEVICE_STAT enum indicates current status of all devices connected.
     */
    enum class AntennaTrackerConnectionState
    {
        UNDEFINED_STATE,
        SUCCESS,
        ARDUINO_UNINITIALIZED,
        ZABER_UNITIALIZED,
        ARDUINO_NOT_OPEN,
        ZABER_NOT_OPEN,
        RELAY_NOT_OPEN,
        FAILED
    };

    enum class AntennaTrackerSerialDevice
    {
        UNDEFINED_DEVICE,
        ARDUINO,
        ZABER
    };

    // TODO: Make static
    /*!
     * \brief getArduinoList returns a list of all available serial devices that can be identified as an Arduino
     * \return QList of port names
     */
    static QList<QString> getArduinoList();

    /*!
     * \brief getZaberList returns a list of all available serial devices that can be identified as a Zaber Controller
     * \return QList of port names
     */
    static QList<QString> getZaberList();

    /*!
     * \brief setupArduino initializes the arduino
     * \param port is the serial port for the arduino
     * \param baud is the baud rate for the arduino
     * \return true if set up was successful, else false
     */
    bool setupArduino(QString port, QSerialPort::BaudRate baud);

    /*!
     * \brief setupZaber initializes the zaber
     * \param port is the serial port for the zaber controller
     * \param baud is the baud rate for the zaber controller
     * \return true if set up was successful, else false
     */
    bool setupZaber(QString port, QSerialPort::BaudRate baud);

    AntennaTrackerConnectionState getArduinoStatus();
    AntennaTrackerConnectionState getZaberStatus();
    void disconnectArduino();
    void disconnectZaber();
    AntennaTrackerConnectionState startTracking(MAVLinkRelay * const relay);
    void stopTracking();

    /*!
     * \brief setStationPos overrides the GPS coordinates of the antenna tracker station with the inputted parameters.
     * \param lon is the longitude value in degrees.
     * \param lat is the latitude value in degrees.
     * \return whether the position has been set.
     */
    bool setStationPos(float lon, float lat);

    /*!
     * \brief overrideGPSToggle sets overrideGPSToggle to the given state.
     */
    void setOverrideGPSToggle(bool toggled);

    /*!
     * \brief AntennaTracker::getAntennaTrackerConnected returns the tracking state of the antenna tracker.
     * \return true if antenna tracker is tracking, else false.
     */
    bool getAntennaTrackerConnected();

    /*!
     * \brief overrideStationHeading overrides the stations heading with the user defined value.
     * \return true if the override was successful.
     */
    bool setOverrideStationHeading(int16_t heading);

    /*!
     * \brief setOverrideStationElevation overrides the stations elevation with the user defined value.
     * \return true if the override was successful.
     */
    bool setOverrideStationElevation(int16_t elevation);

    /*!
     * \brief levelVertical retrieves the antenna trackering station's pitch and levels it to 0.
     * \return true if leveling was successful.
     */
    bool levelVertical();

    /*!
     * \brief calibrateIMU sends motor commands to the zaber to calibrate IMU onboard.
     * \return true if calibration was successful, else false.
     */
    bool calibrateIMU();

    /*!
     * \brief moveZaber recieves a horizontal and vertical angle, calculates the motor steps and then sends the
     * command to the Zaber Controller.
     * \param horizAngle is the angle to move the horizontal motor.
     * \param vertAngle is the angle to move the vertical motor.
     * \return true if movement command was successful, else false.
     */
    bool moveZaber(int16_t horizAngle, int16_t vertAngle);

public slots:
    /*!
     * \brief receiveHandler handles getting serial data from the arduino once a set of
     * GPS data has been received
     * \param gps_data is the mavlink GPS data received
     */
    void receiveHandler(std::shared_ptr<mavlink_global_position_int_t> gpsData);

signals:
    void antennaTrackerDeviceDisconnected(AntennaTrackerSerialDevice device);
    void antennaTrackerStatusUpdate(float latitude, float longitude, float elevation, float heading);
    void antennaTrackerCurrentlyTracking(bool state);

private:
    /*!
     * \brief retrieveStationPos sets the actual GPS coordinates of the antenna tracker station.
     * \return true if the antenna tracker connection state was successful, else false
     */
    bool retrieveStationPos();

    // ================
    // Member Variables
    // ================
    QSerialPort *arduinoSerial;
    QSerialPort *zaberSerial;
    QDataStream *arduinoDataStream;
    UASMessageSerialFramer *arduinoFramer;

    // Base station GPS
    float latBase;
    float lonBase;

    // Base station Heading
    float heading;

    // Base station Elevation
    float elevation;

    // State Variables
    bool antennaTrackerConnected;
    bool overrideGPSToggle;

    // Mavlink relay
    MAVLinkRelay *mavlinkRelay;

    std::atomic<bool> sentRequest;

private slots:
    void zaberControllerDisconnected(QSerialPort::SerialPortError error);
    void arduinoDisconnected(QSerialPort::SerialPortError);
};

#endif // ANTENNATRACKER_H
