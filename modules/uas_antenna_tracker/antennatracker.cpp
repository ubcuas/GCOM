//===================================================================
// Includes
//===================================================================
// GCOM Includes
#include "antennatracker.hpp"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"
#include "modules/uas_message/uas_message.hpp"
#include "modules/uas_message/request_message.hpp"
#include "modules/uas_message/gps_message.hpp"
#include "modules/uas_message/imu_message.hpp"
#include "modules/uas_message/uas_message_serial_framer.hpp"
#include "../Mavlink/ardupilotmega/mavlink.h"
#include "modules/uas_utility/uas_utility.h"
// QT Includes
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QList>
#include <QtMath>
#include <QElapsedTimer>
#include <QDebug>
#include <QThread>
// System Includes
#include <vector>
#include <math.h>

//===================================================================
// Device Detection
//===================================================================
#ifdef __linux__
   const QString ZABER_DEVICE_NAME = "Zaber Technologies Inc.";
#elif __WIN32
   const QString ZABER_DEVICE_NAME = "Microsoft";
#endif

//===================================================================
// Defines
//===================================================================
#define HORIZ_DEGREE_TO_MICROSTEPS 1599.722
#define HORIZ_ANGLE_TO_MICROSTEPS(x) (x * HORIZ_DEGREE_TO_MICROSTEPS)
#define VERT_DEGREE_TO_MICROSTEPS 10630.55555555556
#define VERT_ANGLE_TO_MICROSTEPS(x) (x * VERT_DEGREE_TO_MICROSTEPS)
#define UNPACK_LAT_LON(x)  (((float) x)/10000000)

//===================================================================
// Constants
//===================================================================
// Strings
const QString TEXT_ZABER_NO_CONTROLLER = "NO ZABER CONTROLLER CONNECTED";
const QString ARDUINO_DEVICE_NAME = "Arduino";
const QString TEXT_ARDUINO_NOT_CONNECTED = "NO ARDUINO CONNECTED";
// Zaber Command Templates
const QString zaberStopCommandTemplate= "/1 %1 stop\n";
const QString zaberMoveCommandTemplate= "/1 %1 move rel %2\n";
const QString zaberCheckStatusCommand = "/\n";
// Zaber Setup
const QString zaberSetVerticalMoveSpeed = "/1 1 set maxspeed 120000\n";

//===================================================================
// Class Definitions
//===================================================================
AntennaTracker::AntennaTracker()
{
    // Default Values
    arduinoSerial = nullptr;
    zaberSerial = nullptr;
    arduinoFramer = new UASMessageSerialFramer();

    // Reset the state varaibles
    antennaTrackerConnected = false;
    overrideGPSToggle = false;

    //initialize base coordinates with arbitrary garbage value
    latBase = 0;
    lonBase = 0;

    // initialize base heading
    heading = 0;

    // initialize base elevation
    elevation = 0;

    // Mavlink Relay
    mavlinkRelay = nullptr;

    sentRequest = false;
}

AntennaTracker::~AntennaTracker()
{
    //close serial connection to arduino if it's open
    if(arduinoSerial != nullptr && arduinoSerial->isOpen())
    {
        arduinoSerial->close();
        delete arduinoSerial;
        delete arduinoDataStream;
    }

    //close serial connection to zaber if it's open
    if(zaberSerial != nullptr && zaberSerial->isOpen())
    {
        zaberSerial->close();
        delete zaberSerial;
    }

    delete arduinoFramer;
}

bool AntennaTracker::setupArduino(QString port, QSerialPort::BaudRate baud) {

    if (antennaTrackerConnected)
        stopTracking();

    // Create the object if its not already initialized
    if (arduinoSerial == nullptr)
        arduinoSerial = new QSerialPort(port);

    // Close the port if its open
    if (arduinoSerial->isOpen())
        disconnectArduino();

    if(!arduinoSerial->open(QIODevice::ReadWrite))
        return false;

    // Initialize arduino serial port
    arduinoSerial->setBaudRate(baud);
    arduinoSerial->setDataBits(QSerialPort::Data8);
    arduinoSerial->setParity(QSerialPort::NoParity);
    arduinoSerial->setStopBits(QSerialPort::OneStop);
    arduinoSerial->setFlowControl(QSerialPort::NoFlowControl);

    connect(arduinoSerial,
            SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this,
            SLOT(arduinoDisconnected(QSerialPort::SerialPortError)));

    return true;
}

bool AntennaTracker::setupZaber(QString port, QSerialPort::BaudRate baud) {
    if (antennaTrackerConnected)
        stopTracking();

    // Create the object if its not already initialized
    if (zaberSerial == nullptr)
        zaberSerial = new QSerialPort(port);

    // Close the port if its open
    if (zaberSerial->isOpen())
        disconnectZaber();

    // Initialize the zaber serial port
    zaberSerial = new QSerialPort(port);
    zaberSerial->setBaudRate(baud);
    zaberSerial->setDataBits(QSerialPort::Data8);
    zaberSerial->setParity(QSerialPort::NoParity);
    zaberSerial->setStopBits(QSerialPort::OneStop);
    zaberSerial->setFlowControl(QSerialPort::NoFlowControl);
    if (!zaberSerial->open(QIODevice::ReadWrite))
        return false;

    connect(zaberSerial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
            this, SLOT(zaberControllerDisconnected(QSerialPort::SerialPortError)));

    return true;
}

AntennaTracker::AntennaTrackerConnectionState AntennaTracker::startTracking(MAVLinkRelay * const relay)
{
    this->mavlinkRelay = relay;

    // Connect the Mavlink Relay
    if(relay->status() != MAVLinkRelay::MAVLinkRelayStatus::CONNECTED)
        return AntennaTrackerConnectionState::RELAY_NOT_OPEN;

    // Double check that the conditions required for the connection is correct
    if (arduinoSerial == nullptr)
        return AntennaTrackerConnectionState::ARDUINO_UNINITIALIZED;

    if (zaberSerial == nullptr)
        return AntennaTrackerConnectionState::ZABER_UNITIALIZED;

    if (!arduinoSerial->isOpen())
        return AntennaTrackerConnectionState::ARDUINO_NOT_OPEN;

    if (!zaberSerial->isOpen())
        return AntennaTrackerConnectionState::ZABER_NOT_OPEN;

    // Create the datastreams and framer
    arduinoDataStream = new QDataStream(arduinoSerial);

    // if the GPS is not overrided retrieve the gps location from arduino
    if (!overrideGPSToggle) {
        if(!retrieveStationPos())
            return AntennaTrackerConnectionState::FAILED;
    }

    // Setup desired speed for Zaber vertical movement
    zaberSerial->write(zaberSetVerticalMoveSpeed.toStdString().c_str());
    zaberSerial->flush();

    connect(relay,
            SIGNAL(mavlinkRelayGPSInfo(std::shared_ptr<mavlink_global_position_int_t>)),
            this, SLOT(receiveHandler(std::shared_ptr<mavlink_global_position_int_t>)));

    // update internal state
    antennaTrackerConnected = true;

    // update antenna tracker status params
    emit antennaTrackerStatusUpdate(qRadiansToDegrees(latBase), qRadiansToDegrees(lonBase),
                                    elevation, heading);

    // update antenna tracker currently tracking
    emit antennaTrackerCurrentlyTracking(true);

    return AntennaTrackerConnectionState::SUCCESS;
}

void AntennaTracker::stopTracking()
{
    if (!antennaTrackerConnected)
        return;

    // Disconnect the mavlink relay
    disconnect(this->mavlinkRelay,
               SIGNAL(mavlinkRelayGPSInfo(std::shared_ptr<mavlink_global_position_int_t>)),
               this, SLOT(receiveHandler(std::shared_ptr<mavlink_global_position_int_t>)));

    // update internal state
    antennaTrackerConnected = false;

    // update antenna tracker stopped tracking
    emit antennaTrackerCurrentlyTracking(false);
}

void AntennaTracker::disconnectArduino()
{
    if (arduinoSerial == nullptr)
        return;

    if (!arduinoSerial->isOpen())
        return;

    if (antennaTrackerConnected)
        stopTracking();

    disconnect(arduinoSerial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
               this, SLOT(arduinoDisconnected(QSerialPort::SerialPortError)));
    arduinoSerial->close();
    emit antennaTrackerDeviceDisconnected(AntennaTrackerSerialDevice::ARDUINO);
}

void AntennaTracker::disconnectZaber()
{
    if (zaberSerial == nullptr)
        return;

    if (!zaberSerial->isOpen())
        return;

    if (antennaTrackerConnected)
        stopTracking();

    disconnect(zaberSerial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
               this, SLOT(zaberControllerDisconnected(QSerialPort::SerialPortError)));
    zaberSerial->close();
    emit antennaTrackerDeviceDisconnected(AntennaTrackerSerialDevice::ZABER);
}

AntennaTracker::AntennaTrackerConnectionState AntennaTracker::getArduinoStatus() {
    // check arduino serial
    if (arduinoSerial == nullptr)
        return AntennaTrackerConnectionState::ARDUINO_UNINITIALIZED;
    else if (!arduinoSerial->isOpen())
        return AntennaTrackerConnectionState::ARDUINO_NOT_OPEN;
    else
        return AntennaTrackerConnectionState::SUCCESS;
}

AntennaTracker::AntennaTrackerConnectionState AntennaTracker::getZaberStatus() {
    // check zaber serial
    if (zaberSerial == nullptr)
        return AntennaTrackerConnectionState::ZABER_UNITIALIZED;
    else if (!zaberSerial->isOpen())
        return AntennaTrackerConnectionState::ZABER_NOT_OPEN;
    else
        return AntennaTrackerConnectionState::SUCCESS;
}

void AntennaTracker::arduinoDisconnected(QSerialPort::SerialPortError error)
{
    (void) error;
    if (antennaTrackerConnected)
        stopTracking();

    disconnectArduino();
}

void AntennaTracker::zaberControllerDisconnected(QSerialPort::SerialPortError error)
{
    (void) error;
    if (antennaTrackerConnected)
        stopTracking();

    disconnectZaber();
}

void AntennaTracker::receiveHandler(std::shared_ptr<mavlink_global_position_int_t> droneGPSData)
{
    qDebug() << "Mavlink signal consumed!" << endl;

    RequestMessage imuRequest(UASMessage::MessageID::DATA_IMU);
    arduinoFramer->frameMessage(imuRequest);
    (*arduinoDataStream) << (*arduinoFramer);

    // Wait till we recieve data from the IMU
    while (true)
    {
        arduinoDataStream->startTransaction();
        (*arduinoDataStream) >> (*arduinoFramer);
        if (arduinoFramer->status() == UASMessageSerialFramer::SerialFramerStatus::SUCCESS)
        {
            break;
        }
        else if (arduinoFramer->status() == UASMessageSerialFramer::SerialFramerStatus::INCOMPLETE_MESSAGE)
        {
            arduinoDataStream->rollbackTransaction();
        }
        else
        {
            arduinoDataStream->commitTransaction();
            return;
        }
        arduinoSerial->waitForReadyRead(3000);
    }
    arduinoDataStream->commitTransaction();


    std::shared_ptr<UASMessage> imuMessage = arduinoFramer->generateMessage();
    if ((imuMessage == nullptr) || (imuMessage->type() != UASMessage::MessageID::DATA_IMU))
        return;

    const float yawBase= std::static_pointer_cast<IMUMessage>(imuMessage)->x;
    const float pitchBase = std::static_pointer_cast<IMUMessage>(imuMessage)->y;

    // Calculate horizontal angle
    float moveHorizAngle = Utility::calcHorizontal(droneGPSData, yawBase, latBase, lonBase, heading);

    // Calculate vertical angle
    float moveVertAngle = Utility::calcVertical(droneGPSData, pitchBase, latBase, lonBase, elevation);

    // do tracking
    moveZaber(moveHorizAngle, moveVertAngle);
}

bool AntennaTracker::moveZaber(int16_t horizAngle, int16_t vertAngle)
{
    // checks if serial connection is made
    if (!zaberSerial->isOpen())
        return false;

    // converts user defined angles to microsteps
    int microStepsHoriz = -1 * HORIZ_ANGLE_TO_MICROSTEPS(horizAngle);
    int microStepsVert = -1 * VERT_ANGLE_TO_MICROSTEPS(vertAngle);

    if(vertAngle != 0) {
        // move vertical command
        QString vertZaberCommand = QString(zaberMoveCommandTemplate).arg(1).arg(microStepsVert);
        zaberSerial->write(vertZaberCommand.toStdString().c_str());
    }
    if(horizAngle != 0) {
        // move horizontal command
        QString horizZaberCommand = QString(zaberMoveCommandTemplate).arg(2).arg(microStepsHoriz);
        zaberSerial->write(horizZaberCommand.toStdString().c_str());
    }

    if(vertAngle != 0 && horizAngle != 0)
        zaberSerial->flush();

    return true;
}

bool AntennaTracker::retrieveStationPos()
{
    // Retrieve the base's GPS Corrdinates
    RequestMessage gpsRequest = RequestMessage(UASMessage::MessageID::DATA_GPS);
    arduinoFramer->frameMessage(gpsRequest);
    (*arduinoDataStream) << (*arduinoFramer);

    // Verify the connection
    if (!arduinoSerial->waitForReadyRead(3000))
        return false;

    arduinoFramer->clearMessage();

    // Attempt to read the GPS Message from the antenna tracker
    // Note: This is a bad busy loop! We are only allowed to do this because the drone will not be
    // in flight at this time!
    while (true)
    {
        arduinoDataStream->startTransaction();
        (*arduinoDataStream) >> (*arduinoFramer);
        if (arduinoFramer->status() == UASMessageSerialFramer::SerialFramerStatus::SUCCESS)
            break;
        else if (arduinoFramer->status() == UASMessageSerialFramer::SerialFramerStatus::INCOMPLETE_MESSAGE)
            arduinoDataStream->rollbackTransaction();
        else
            return false;
        arduinoSerial->waitForReadyRead(10);
    }
    arduinoDataStream->commitTransaction();

    // Process the GPS Coordinates of the base station!
    std::shared_ptr<UASMessage> gpsMessage = arduinoFramer->generateMessage();
    if ((gpsMessage == nullptr) || (gpsMessage->type() != UASMessage::MessageID::DATA_GPS))
        return false;

    lonBase = qDegreesToRadians(std::static_pointer_cast<GPSMessage>(gpsMessage)->lon);
    latBase = qDegreesToRadians(std::static_pointer_cast<GPSMessage>(gpsMessage)->lat);

    return true;
}

//===================================================================
// Setters and Getters
//===================================================================
bool AntennaTracker::setStationPos(float lon, float lat)
{
    // check inputs in range
    if((lat < -90 || lat > 90) || (lon < -180 || lat > 180))
        return false;

    // check if the tracker is running
    if(antennaTrackerConnected)
        return false;

    lonBase = qDegreesToRadians(lon);
    latBase = qDegreesToRadians(lat);

    // lat lon successfully set
    return true;
}

bool AntennaTracker::getAntennaTrackerConnected()
{
   return this->antennaTrackerConnected;
}

//===================================================================
// Override Functions
//===================================================================
bool AntennaTracker::setOverrideStationHeading(int16_t heading)
{
    this->heading = heading;
    return true;
}

bool AntennaTracker::setOverrideStationElevation(int16_t elevation)
{
    this->elevation = elevation;
    return true;
}

void AntennaTracker::setOverrideGPSToggle(bool toggled)
{
    overrideGPSToggle = toggled;
}

//===================================================================
// Calibration Functions
//===================================================================
bool AntennaTracker::levelVertical()
{
    // Create the datastreams and framer
    arduinoDataStream = new QDataStream(arduinoSerial);

    // Send request data request to arduino/IMU
    RequestMessage imuRequest(UASMessage::MessageID::DATA_IMU);
    arduinoFramer->frameMessage(imuRequest);
    (*arduinoDataStream) << (*arduinoFramer);

    // Wait till we recieve data from the IMU
    while (true)
    {
        arduinoDataStream->startTransaction();
        (*arduinoDataStream) >> (*arduinoFramer);
        if (arduinoFramer->status() == UASMessageSerialFramer::SerialFramerStatus::SUCCESS)
        {
            break;
        }
        else if (arduinoFramer->status() == UASMessageSerialFramer::SerialFramerStatus::INCOMPLETE_MESSAGE)
        {
            arduinoDataStream->rollbackTransaction();
        }
        else
        {
            arduinoDataStream->commitTransaction();
            return false;
        }
        arduinoSerial->waitForReadyRead(3000);
    }
    arduinoDataStream->commitTransaction();

    std::shared_ptr<UASMessage> imuMessage = arduinoFramer->generateMessage();
    if ((imuMessage == nullptr) || (imuMessage->type() != UASMessage::MessageID::DATA_IMU))
        return false;

    // store pitch angle
    const float pitchBase = std::static_pointer_cast<IMUMessage>(imuMessage)->y;

    // negate pitch angle for leveling
    float vertAngleFlat = pitchBase * -1;

    if(vertAngleFlat > -1 && vertAngleFlat < 1) {
        // don't move if angle is too small to reduce drifting
        vertAngleFlat = 0;
    }

    // level zaber vertically
    moveZaber(0,vertAngleFlat);

    return true;
}

bool AntennaTracker::calibrateIMU()
{
    // checks if serial connection is made
    if (!zaberSerial->isOpen()) {
        qDebug() << "zaber is not connected" << endl;
        return false;
    }

    // horizontal, vertical commands for calibration
    int numberOfTests = 11;
    int calibrationArray[numberOfTests][2] =
    {
        {0,50},     // 60   deg
        {90,0},     // 90   deg CW horizontal
        {0,-100},   // -120 deg
        {90,0},     // 90   deg CW horizontal
        {0,100},    // 120  deg
        {90,0},     // 90   deg CW horizontal
        {0,-100},   // -120 deg
        {90,0},     // 90   deg CW horizontal
        {0,100},    // 120  deg
        {-360,0},   // 360  deg CCW horizontal
        {0,-50}     // -60  deg
    };

    // check status output
    QString outputLine = "";

    // level vertical upon start up
    levelVertical();

    // iterate through each command in the calibration array and check for idle
    for(int testIndex = 0; testIndex < numberOfTests; testIndex++) {
        // checks for IDLE, and executes move command once found
        while(true) {
            // send check status command
            zaberSerial->write(zaberCheckStatusCommand.toStdString().c_str());
            zaberSerial->flush();

            // check bytes written before reading
            if(zaberSerial->waitForBytesWritten(500) && zaberSerial->waitForReadyRead(500)) {
                // read data
                QByteArray datas = zaberSerial->readAll();
                outputLine = datas; // convert to QString

                // check for IDLE
                if(outputLine.contains("IDLE", Qt::CaseInsensitive))
                    break;
            }
        }

        // move next command
        moveZaber(calibrationArray[testIndex][0],calibrationArray[testIndex][1]);
    }

    return true;
}

//===================================================================
// Listing Functions
//===================================================================
QList<QString> AntennaTracker::getZaberList()
{
    QList<QString> zaberPorts;
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();    //get a list of all available ports

    //Iterate through list of ports
    foreach(QSerialPortInfo currPort, portList)
    {
        //check for zaber ports and add to list
        if(currPort.manufacturer().contains(ZABER_DEVICE_NAME))
            zaberPorts.append(currPort.portName());
    }

    if(zaberPorts.isEmpty())
    {
        zaberPorts.append(TEXT_ZABER_NO_CONTROLLER);
        return zaberPorts;
    }
    else
    {
        return zaberPorts;   //return list of zaber controllers
    }
}

QList<QString> AntennaTracker::getArduinoList()
{
    QList<QString> arduinoPorts;    //list of arduinos
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();    //get a list of all available ports

    //iterate through list of ports
    foreach(QSerialPortInfo currPort, portList)
    {
        //check for arduino ports and add to list
        if(currPort.manufacturer().contains(ARDUINO_DEVICE_NAME))
            arduinoPorts.append(currPort.portName());
    }

    if(arduinoPorts.isEmpty())
    {
        arduinoPorts.append(TEXT_ARDUINO_NOT_CONNECTED);
        return arduinoPorts;
    }
    else
    {
        return arduinoPorts;    //return list of arduinos
    }
}
