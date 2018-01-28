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
// QT Includes
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QString>
#include <QList>
#include <QtMath>
#include <QElapsedTimer>
#include <QDebug>
// System Includes
#include <vector>
#include <math.h>

//===================================================================
// Defines
//===================================================================
#define HORIZ_DEGREE_TO_MICROSTEPS 1599.722
#define HORIZ_ANGLE_TO_MICROSTEPS(x) (x * HORIZ_DEGREE_TO_MICROSTEPS)
#define VERT_DEGREE_TO_MICROSTEPS 10630.55555555556
#define VERT_ANGLE_TO_MICROSTEPS(x) (x * HORIZ_DEGREE_TO_MICROSTEPS)
#define UNPACK_LAT_LON(x)  (((float) x)/10000000)

//===================================================================
// Constants
//===================================================================
// Strings
const QString ZABER_DEVICE_NAME = "Zaber Technologies Inc.";
const QString TEXT_ZABER_NO_CONTROLLER = "NO ZABER CONTROLLER CONNECTED";
const QString ARDUINO_DEVICE_NAME = "Arduino";
const QString TEXT_ARDUINO_NOT_CONNECTED = "NO ARDUINO CONNECTED";
// Zaber Command Templates
const QString zaberStopCommandTemplate= "/1 %1 stop\n";
const QString zaberMoveCommandTemplate= "/1 %1 move rel %2\n";
// Zaber Setup
const QString zaberSetVerticalMoveSpeed = "/1 1 set maxspeed 120000";
// Constant
const float RADIUS_EARTH = 6378137;

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

    //initialize base coordinates with arbitrary garbage value
    latBase = 0;
    lonBase = 0;

    sentRequest = false;
}

AntennaTracker::~AntennaTracker()
{
    //close serial connection to arduino if it's open
    if(arduinoSerial != nullptr && arduinoSerial->isOpen())
    {
        arduinoSerial->close();
        delete zaberSerial;
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

bool AntennaTracker::setupDevice(QString port, QSerialPort::BaudRate baud,
                                 AntennaTrackerSerialDevice devType)
{
    qDebug()<<"Bonjour";
    if (antennaTrackerConnected)
        stopTracking();

    bool success = false;

    switch(devType)
    {
        case AntennaTrackerSerialDevice::ARDUINO:
            // Create the object if its not already initialized
            if (arduinoSerial == nullptr)
                arduinoSerial = new QSerialPort(port);

            // Close the port if its open
            if (arduinoSerial->isOpen())
                disconnectArduino();



            if(!arduinoSerial->open(QIODevice::ReadWrite))
            {
                qDebug() << "Howdy";
                return false;

            }

            // Initialize arduino serial port
            arduinoSerial->setBaudRate(QSerialPort::BaudRate::Baud9600);
            arduinoSerial->setDataBits(QSerialPort::Data8);
            arduinoSerial->setParity(QSerialPort::NoParity);
            arduinoSerial->setStopBits(QSerialPort::OneStop);
            arduinoSerial->setFlowControl(QSerialPort::NoFlowControl);

            connect(arduinoSerial,
                    SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this,
                    SLOT(arduinoDisconnected(QSerialPort::SerialPortError)));
            qDebug() << "Hola";
            success = true;
            break;

        case AntennaTrackerSerialDevice::ZABER:
            if (zaberSerial == nullptr)
                zaberSerial = new QSerialPort(port);

            if (zaberSerial->isOpen())
                disconnectZaber();

            // Init the  zaber serial port
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

            success = true;
            break;

        default:
            success = false;
            break;
    }

    return success;
}

AntennaTracker::AntennaTrackerConnectionState AntennaTracker::startTracking(MAVLinkRelay * const relay)
{
    QElapsedTimer timer;
    timer.start();
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

    // Retrieve the base's GPS Corrdinates
    RequestMessage gpsRequest = RequestMessage(UASMessage::MessageID::DATA_GPS);
    arduinoFramer->frameMessage(gpsRequest);
    (*arduinoDataStream) << (*arduinoFramer);

    // Verify the connection
    if (!arduinoSerial->waitForReadyRead(3000))
        return AntennaTrackerConnectionState::FAILED;

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
            return AntennaTrackerConnectionState::FAILED;
        arduinoSerial->waitForReadyRead(10);
    }
    arduinoDataStream->commitTransaction();
    qDebug() << timer.elapsed();

    // Setup desired speed for Zaber vertical movement
    zaberSerial->write(zaberSetVerticalMoveSpeed.toStdString().c_str());
    zaberSerial->flush();

    // Process the GPS Coordinates of the base station!
    std::shared_ptr<UASMessage> gpsMessage = arduinoFramer->generateMessage();
    if ((gpsMessage == nullptr) || (gpsMessage->type() != UASMessage::MessageID::DATA_GPS))
        return AntennaTrackerConnectionState::FAILED;

    lonBase = qDegreesToRadians(std::static_pointer_cast<GPSMessage>(gpsMessage)->lon * -1);
    latBase = qDegreesToRadians(std::static_pointer_cast<GPSMessage>(gpsMessage)->lat);

    // Connect the Mavlink Relay
    if(relay->status() != MAVLinkRelay::MAVLinkRelayStatus::CONNECTED)
        return AntennaTrackerConnectionState::RELAY_NOT_OPEN;

    connect(relay,
            SIGNAL(mavlinkRelayGPSInfo(std::shared_ptr<mavlink_global_position_int_t>)),
            this, SLOT(receiveHandler(std::shared_ptr<mavlink_global_position_int_t>)));

    antennaTrackerConnected = true;
    return AntennaTrackerConnectionState::SUCCESS;
}

void AntennaTracker::stopTracking()
{
    if (!antennaTrackerConnected)
        return;

    // Disconnect the mavlink relay
    disconnect(this, SLOT(receiveHandler(std::shared_ptr<mavlink_global_position_int_t>)));
    antennaTrackerConnected = false;
    emit antennaTrackerDisconnected();
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


AntennaTracker::AntennaTrackerConnectionState AntennaTracker::getDeviceStatus(
        AntennaTracker::AntennaTrackerSerialDevice device)
{
    switch (device)
    {
        case AntennaTrackerSerialDevice::ARDUINO:
            if (arduinoSerial == nullptr)
                return AntennaTrackerConnectionState::ARDUINO_UNINITIALIZED;
            else if (!arduinoSerial->isOpen())
                return AntennaTrackerConnectionState::ARDUINO_NOT_OPEN;
            else
                return AntennaTrackerConnectionState::SUCCESS;

        case AntennaTrackerSerialDevice::ZABER:
            if (zaberSerial == nullptr)
                return AntennaTrackerConnectionState::ZABER_UNITIALIZED;
            else if (!zaberSerial->isOpen())
                return AntennaTrackerConnectionState::ZABER_NOT_OPEN;
            else
                return AntennaTrackerConnectionState::SUCCESS;

        default:
            return AntennaTrackerConnectionState::FAILED;
    }
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
    qDebug() << "Mavlink signal consumed!!!!!!" << endl;


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
    const float pitchBase = (std::static_pointer_cast<IMUMessage>(imuMessage)->y) * -1;

    // Do horizontal tracking.
    QString horzZaberCommand = calcHorizontal(droneGPSData, yawBase);
    zaberSerial->write(horzZaberCommand.toStdString().c_str());

    // Do vertical tracking.
    QString vertZaberCommand = calcVertical (droneGPSData, pitchBase);
    zaberSerial->write(vertZaberCommand.toStdString().c_str());

    zaberSerial->flush();
}

QString AntennaTracker::calcHorizontal(std::shared_ptr<mavlink_global_position_int_t> droneGPSData, float yawIMU)
{
    // ====================================================================
    // Calculations from http://www.movable-type.co.uk/scripts/latlong.html
    // ====================================================================

    //Grabbing individual pieces of data from gpsData
    const float droneLat = qDegreesToRadians(((float) droneGPSData->lat)/ 10000000);
    const float droneLon = qDegreesToRadians(((float) droneGPSData->lon)/ 10000000);

    const float yDiff = (droneLon-lonBase);

    const float y = sin(yDiff) * cos(droneLat);
    const float x = (cos(latBase) * sin(droneLat)) - (sin(latBase) * cos(droneLat) * cos(yDiff));

    const float horizAngle = fmod((qRadiansToDegrees(atan2(y,x)) + 360), 360);

    // Find the quickest angle to reach the point
    float horzAngleDiff = horizAngle - (yawIMU);

    if(horzAngleDiff > 180) {
        horzAngleDiff -= 360;
    }
    else if(horzAngleDiff < -180) {
        horzAngleDiff += 360;
    }
    else if(horzAngleDiff > -1 && horzAngleDiff < 1) {
        // don't move if angle is too small to reduce drifting
        horzAngleDiff = 0;
    }

    const int microSteps = -1 * HORIZ_ANGLE_TO_MICROSTEPS(horzAngleDiff);

    return QString(zaberMoveCommandTemplate).arg(2).arg(microSteps);
}

QString AntennaTracker::calcVertical (std::shared_ptr<mavlink_global_position_int_t> droneGPSData, float pitchIMU)
{
    // ====================================================================
    // Calculations from http://www.movable-type.co.uk/scripts/latlong.html
    // ====================================================================

    const float droneLat = qDegreesToRadians(((float) droneGPSData->lat)/ 10000000);
    const float droneLon = qDegreesToRadians(((float) droneGPSData->lon)/ 10000000);

    const float droneRelativeAlt = ((float)droneGPSData->relative_alt / 1000);

    const float xDiff = (droneLat-latBase);
    const float yDiff = (droneLon-lonBase);

    const float a = pow(sin(xDiff/2),2)+cos(latBase) * cos(droneLat)*pow(sin(yDiff/2),2);
    const float d = 2 * atan2(sqrt(a), sqrt(1-a));
    const float distance = RADIUS_EARTH * d;

    const float vertAngle = atan((droneRelativeAlt)/distance)*180/M_PI;

    float vertAngleDiff = vertAngle - pitchIMU;

    if(vertAngleDiff > -1 && vertAngleDiff < 1) {
        // don't move if angle is too small to reduce drifting
        vertAngleDiff = 0;
    }

    int microSteps = -1 * VERT_ANGLE_TO_MICROSTEPS(vertAngleDiff);

    return QString(zaberMoveCommandTemplate).arg(1).arg(microSteps);
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
