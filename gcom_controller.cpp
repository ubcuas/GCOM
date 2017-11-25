//===================================================================
// Includes
//===================================================================
// System Includes
#include <QMovie>
#include <QString>
#include <QIntValidator>
#include <QRegExpValidator>
#include <QRegExp>
#include <QTimer>
#include <QDebug>
#include <QThread>
// GCOM Includes
#include "gcom_controller.hpp"
#include "ui_gcomcontroller.h"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_antenna_tracker/antennatracker.hpp"
#include "modules/uas_message/uas_message_serial_framer.hpp"

//===================================================================
// Constants
//===================================================================
const QString DISCONNECT_LABEL("<font color='#D52D2D'> DISCONNECTED </font>"
                               "<img src=':/connection/disconnected.png'>");
const QString CONNECTING_LABEL("<font color='#EED202'> CONNECTING </font>"
                               "<img src=':/connection/connecting.png'>");
const QString CONNECTED_LABEL("<font color='#05c400'> CONNECTED </font>"
                               "<img src=':/connection/connected.png'>");
const QString SEARCHING_LABEL("<font color='#EED202'> SEARCHING </font>"
                               "<img src=':/connection/connecting.png'>");

// Regex Field Validation
const QRegExp IP_REGEX("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}$");
const QRegExp LATLON_REGEX("^-?[0-9]*\\.[0-9]*$");
const QRegExp ELEV_HEADING_REGEX("^-?[0-9]*(\\.[0-9]*)?$");

// MAVLink Constants
const QString CONNECT_BUTTON_TEXT("Connect");
const QString CONNECTING_BUTTON_TEXT("Cancel Connecting");
const QString DISCONNECT_BUTTON_TEXT("Disconnect");

// DCNC Constants
const QString START_SEARCHING_BUTTON_TEXT("Start Searching");
const QString STOP_SEARCHING_BUTTON_TEXT("Stop Searching");
const QString STOP_SERVER_BUTTON_TEXT("Stop Server");
const QString UNKNOWN_LABEL("Unknown");
const QString DISCONNECTED_LABEL("Disconnected");

// Antenna Tracker Constants
const QString START_TRACKING_BUTTON_TEXT("Start Tracking");
const QString STOP_TRACKING_BUTTON_TEXT("Stop Tracking");

//===================================================================
// Class Declarations
//===================================================================
GcomController::GcomController(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GcomController)
{
    // General UI Setup
    ui->setupUi(this);
    // Set up field validation
    ui->mavlinkPortField->setValidator(new QIntValidator(0,1000000));
    ui->mavlinkIPField->setValidator(new QRegExpValidator(IP_REGEX));
    ui->dcncServerPortField->setValidator(new QIntValidator(0,1000000));
    ui->dcncServerIPField->setValidator(new QRegExpValidator(IP_REGEX));
    ui->antennaTrackerOverrideLongitudeField->setValidator(new QRegExpValidator(LATLON_REGEX));
    ui->antennaTrackerOverrideLatitudeField->setValidator(new QRegExpValidator(LATLON_REGEX));
    ui->antennaTrackerOverrideHeadingField->setValidator(new QRegExpValidator(ELEV_HEADING_REGEX));
    ui->antennaTrackerOverrideElevationField->setValidator(new QRegExpValidator(ELEV_HEADING_REGEX));
    restMavlinkGUI();

    // Mavlink Setup
    mavlinkRelay = new MAVLinkRelay();
    mavlinkConnectionTimer = new QTimer();
    connect(mavlinkConnectionTimer, SIGNAL(timeout()),
            this, SLOT(mavlinkTimerTimeout()));
    connect(mavlinkRelay, SIGNAL(mavlinkRelayConnected()),
            this, SLOT(mavlinkRelayConnected()));
    connect(mavlinkRelay, SIGNAL(mavlinkRelayDisconnected()),
            this, SLOT(mavlinkRelayDisconnected()));
    mavlinkButtonDisconnect = false;
    mavlinkConnectingMovie = new QMovie (":/connection/mavlink_connecting.gif");
    mavlinkConnectedMovie = new QMovie (":/connection/mavlink_connected.gif");

    // DCNC Setup
    dcnc = new DCNC();
    connect(dcnc, SIGNAL(receivedConnection()), this, SLOT(dcncConnected()));
    connect(dcnc, SIGNAL(droppedConnection()), this, SLOT(dcncDisconnected()));
    connect(dcnc, SIGNAL(receivedGremlinInfo(QString,uint16_t,bool)),
            this, SLOT(gremlinInfo(QString,uint16_t,bool)));
    connect(dcnc, SIGNAL(receivedGremlinCapabilities(CapabilitiesMessage::Capabilities)),
            this, SLOT(gremlinCapabilities(CapabilitiesMessage::Capabilities)));
    dcncConnectingMovie = new QMovie (":/connection/dcnc_connecting.gif");
    dcncConnectedMovie = new QMovie (":/connection/mavlink_connected.gif");
    dcncConnectionTimer = new QTimer();
    connect(dcncConnectionTimer, SIGNAL(timeout()), this, SLOT(dcncTimerTimeout()));
    dcncSearchTimeoutTimer = new QTimer();
    connect(dcncSearchTimeoutTimer, SIGNAL(timeout()), this, SLOT(dcncSearchTimeout()));
    connect(ui->dcncServerAutoResume, SIGNAL(clicked(bool)), dcnc, SLOT(changeAutoResume(bool)));
    resetDCNCGUI();

    // Antenna Tracker Setup
    tracker = new AntennaTracker();
    ui->antennaTrackerTab->setDisabled(true);
    ui->startTrackButton->setEnabled(false);
    ui->antennaTrackerCalibrateIMUButton->setEnabled(false);

    // updates UI with station lat and lon
    connect(tracker,
            SIGNAL(antennaTrackerStatusUpdate(float,float,float,float)),
            this,
            SLOT(antennaTrackerUpdateStatusGUI(float,float,float,float)));

    // disables UI when tracking
    connect(tracker,
            SIGNAL(antennaTrackerCurrentlyTracking(bool)),
            this,
            SLOT(disableAntennaTrackingGUI(bool)));
}

GcomController::~GcomController()
{
    delete ui;
    delete mavlinkRelay;
    delete mavlinkConnectionTimer;
    delete mavlinkConnectingMovie;
    delete dcnc;
    delete tracker;
}

//===================================================================
// MAVLink Methods
//===================================================================

void GcomController::restMavlinkGUI()
{
    mavlinkConnectionTime = 0;
    ui->mavlinkStatusField->setText(DISCONNECT_LABEL);
    ui->mavlinkConnectionStatusField->setText(DISCONNECT_LABEL);
    ui->mavlinkConnectionTime->display(formatDuration(mavlinkConnectionTime));
    ui->mavlinkConnectionButton->setText(CONNECT_BUTTON_TEXT);
    // Enable all input fields
    ui->mavlinkIPField->setDisabled(false);
    ui->mavlinkPortField->setDisabled(false);
}

void GcomController::mavlinkTimerTimeout()
{
    ui->mavlinkConnectionTime->display(formatDuration(++mavlinkConnectionTime));
}

void GcomController::on_mavlinkConnectionButton_clicked()
{
    // If the MAVLink relay was disconnected the state machine progresses to
    // the connection stage
    if (mavlinkRelay->status() == MAVLinkRelay::MAVLinkRelayStatus::DISCCONNECTED)
    {
        // Disable all input fields
        ui->mavlinkIPField->setDisabled(true);
        ui->mavlinkPortField->setDisabled(true);
        // Change the GUI elements to reflect that the relay is in the
        // connecting stage
        ui->mavlinkConnectionButton->setText(CONNECTING_BUTTON_TEXT);
        ui->mavlinkStatusField->setText(CONNECTING_LABEL);
        ui->mavlinkConnectionStatusField->setText(CONNECTING_LABEL);
        mavlinkConnectedMovie->stop();
        ui->mavlinkStatusMovie->setMovie(mavlinkConnectingMovie);
        mavlinkConnectingMovie->start();
        // Start the MAVLinkRelay
        mavlinkRelay->setup(ui->mavlinkIPField->text(), ui->mavlinkPortField->text().toInt());
        mavlinkRelay->start();
    }
    // If the relay is in any other state (Connecting or Connected) then it is
    // stopped
    else
    {
        mavlinkButtonDisconnect = true;
        mavlinkRelay->stop();
    }
}

void GcomController::mavlinkRelayConnected()
{
    // Update the labels the on screen text to indicate that we have connected
    ui->mavlinkStatusField->setText(CONNECTED_LABEL);
    ui->mavlinkConnectionStatusField->setText(CONNECTED_LABEL);
    ui->mavlinkConnectionButton->setText(DISCONNECT_BUTTON_TEXT);
    // Stop the connection movie
    ui->mavlinkStatusMovie->setText(" ");
    mavlinkConnectingMovie->stop();
    ui->mavlinkStatusMovie->setMovie(mavlinkConnectedMovie);
    mavlinkConnectedMovie->start();
    // Start the timer
    mavlinkConnectionTimer->start(1000);
    // Enable the antenna tracker
    ui->antennaTrackerTab->setDisabled(false);
}

void GcomController::mavlinkRelayDisconnected()
{
    if (ui->mavlinkAutoReconnect->isChecked() && mavlinkButtonDisconnect != true)
    {
        on_mavlinkConnectionButton_clicked();
        return;
    }
    // When a disconnection is detected then the GUI is reset to the unconnected
    // state
    restMavlinkGUI();
    // Stop any movies
    ui->mavlinkStatusMovie->setText(" ");
    mavlinkConnectedMovie->stop();
    mavlinkConnectingMovie->stop();
    // Stop the timer
    mavlinkConnectionTimer->stop();
    // Stop the tracker if its on
    ui->antennaTrackerTab->setDisabled(true);
    // Reset the button method
    mavlinkButtonDisconnect = false;
}

//===================================================================
// DCNC Methods
//===================================================================

void GcomController::resetDCNCGUI()
{
    // Reset the connection timer
    dcncConnectionTime = 0;
    ui->dcncConnectionTime->display(formatDuration(dcncConnectionTime));
    // Reset Lables
    ui->dcncConnectionStatusField->setText(DISCONNECT_LABEL);
    ui->dcncStatusField->setText(DISCONNECT_LABEL);
    ui->dcncConnectionButton->setText(START_SEARCHING_BUTTON_TEXT);
    ui->dcncIPAdressField->setText(DISCONNECTED_LABEL);
    ui->dcncIPVersionField->setText(DISCONNECTED_LABEL);
    ui->dcncVersionNumberField->setText(DISCONNECTED_LABEL);
    ui->dcncDeviceIDField->setText(DISCONNECTED_LABEL);
    // Clear Capabilities
    ui->dcncCapabilitiesField->clear();
    // Enable all input input fields
    ui->dcncServerIPField->setDisabled(false);
    ui->dcncServerPortField->setDisabled(false);
    ui->dcncServerTimeoutField->setDisabled(false);
    // Reset the animations
    dcncConnectedMovie->stop();
    dcncConnectingMovie->stop();
    ui->dcncStatusMovie->setText(" ");
    // Deactivate the drop gremlin button
    ui->dcncDropGremlin->setDisabled(false);

    // Clear capabilities field
    ui->dcncCapabilitiesField->clear();
}

void GcomController::on_dcncConnectionButton_clicked()
{
    bool status;
    switch(dcnc->status())
    {
        // If we are offline start the search
        case DCNC::DCNCStatus::OFFLINE:
        {
            // Lock the input fields
            ui->dcncServerIPField->setDisabled(true);
            ui->dcncServerIPField->setDisabled(false);
            ui->dcncServerTimeoutField->setDisabled(false);

            status = dcnc->startServer(
                        ui->dcncServerIPField->text(),
                        ui->dcncServerPortField->text().toInt());

            // TODO Add a warning message
            if (status == false)
                resetDCNCGUI();

            // Update UI text to indicate searching
            ui->dcncConnectionButton->setText(STOP_SEARCHING_BUTTON_TEXT);
            ui->dcncStatusField->setText(SEARCHING_LABEL);
            ui->dcncIPAdressField->setText(UNKNOWN_LABEL);
            ui->dcncIPVersionField->setText(UNKNOWN_LABEL);
            ui->dcncVersionNumberField->setText(UNKNOWN_LABEL);
            ui->dcncDeviceIDField->setText(UNKNOWN_LABEL);

            // Update the status line
            dcncConnectingMovie->stop();
            ui->dcncStatusMovie->setMovie(dcncConnectingMovie);
            dcncConnectingMovie->start();

            // Start the timeout timer
            dcncSearchTimeoutTimer->start(ui->dcncServerTimeoutField->text().toULong() * 1000);
        }
        break;

        // If we are searching or are connected then we just stop the server.
        case DCNC::DCNCStatus::SEARCHING:
        case DCNC::DCNCStatus::CONNECTED:
        {
            dcncSearchTimeoutTimer->stop();
            dcnc->stopServer();
            resetDCNCGUI();
        }
        break;

        default:
        break;
    }
}

void GcomController::on_dcncDropGremlin_clicked()
{
    dcnc->cancelConnection();
}

// TODO Pass the IP address and IP version
void GcomController::dcncConnected()
{
    dcncSearchTimeoutTimer->stop();

    // When we are connected then change the button to dissconnect server
    ui->dcncConnectionButton->setText(STOP_SERVER_BUTTON_TEXT);
    ui->dcncConnectionStatusField->setText(CONNECTED_LABEL);

    // Start the the connection timer and stop the timeout timer
    dcncConnectionTimer->start(1000);
    dcncSearchTimeoutTimer->stop();

    // Update the status line
    ui->dcncStatusField->setText(CONNECTED_LABEL);
    ui->dcncStatusMovie->setMovie(dcncConnectedMovie);
    dcncConnectingMovie->start();

    // Activate the drop gremlin button
    ui->dcncDropGremlin->setDisabled(true);
}

void GcomController::dcncDisconnected()
{
    // Update the UI
    ui->dcncConnectionButton->setText(STOP_SEARCHING_BUTTON_TEXT);
    ui->dcncStatusField->setText(SEARCHING_LABEL);
    ui->dcncConnectionStatusField->setText(DISCONNECT_LABEL);
    // Stop the connection timer
    dcncConnectionTimer->stop();
    // Update the UI
    ui->dcncStatusMovie->setMovie(dcncConnectingMovie);
    dcncConnectingMovie->start();
    // Activate the drop gremlin button
    ui->dcncDropGremlin->setDisabled(false);
    // Start the connection timeout timer.
    dcncSearchTimeoutTimer->start(ui->dcncServerTimeoutField->text().toULong() * 1000);

    // Clear capabilities field
    ui->dcncCapabilitiesField->clear();
}

void GcomController::gremlinInfo(QString systemId, uint16_t versionNumber, bool dropped)
{
    (void) dropped;
    ui->dcncDeviceIDField->setText(systemId);
    ui->dcncVersionNumberField->setText(QString::number(versionNumber));
}

void GcomController::gremlinCapabilities(CapabilitiesMessage::Capabilities capabilities)
{
    // May have several capabilities, so loop through all of them
    do {
        if (static_cast<uint32_t>(capabilities & CapabilitiesMessage::Capabilities::IMAGE_RELAY))
        {
            ui->dcncCapabilitiesField->addItem("Image Relay");
            dcnc->startImageRelay();
        }

        capabilities = capabilities >> 8;
    } while (static_cast<uint32_t>(capabilities) > 0);
}

void GcomController::dcncTimerTimeout()
{
    ui->dcncConnectionTime->display(formatDuration(++dcncConnectionTime));
}

void GcomController::dcncSearchTimeout()
{
    if(dcnc->status() == DCNC::DCNCStatus::SEARCHING)
    {
        dcnc->stopServer();
        resetDCNCGUI();
    }
}

//===================================================================
// Antenna Tracker Methods
//===================================================================
void GcomController::on_arduinoRefreshButton_clicked()
{
    QList<QString> portList = tracker->getArduinoList();

    ui->availableArduinoPorts->clear();
    ui->availableArduinoPorts->addItems(QStringList(portList));
}

void GcomController::on_arduinoConnectButton_clicked()
{
    if (tracker->getArduinoStatus() != AntennaTracker::AntennaTrackerConnectionState::SUCCESS)
    {
        QModelIndex selectedIndex = ui->availableArduinoPorts->currentIndex();
        QString selectedPort = selectedIndex.data().toString();

        bool status = tracker->setupArduino(selectedPort, QSerialPort::Baud9600);

        if (status)
            ui->arduinoConnectButton->setText(DISCONNECT_BUTTON_TEXT);
    }
    else
    {
        tracker->disconnectArduino();
        ui->arduinoConnectButton->setText(CONNECT_BUTTON_TEXT);
    }

    updateStartTrackerButton();
}

void GcomController::on_zaberRefreshButton_clicked()
{
    QList<QString> portList = tracker->getZaberList();

    ui->availableZaberPorts->clear();
    ui->availableZaberPorts->addItems(QStringList(portList));
}

void GcomController::on_zaberConnectButton_clicked()
{
    if (tracker->getZaberStatus() != AntennaTracker::AntennaTrackerConnectionState::SUCCESS)
    {
        QModelIndex selectedIndex = ui->availableZaberPorts->currentIndex();
        QString selectedPort = selectedIndex.data().toString();

        bool status = tracker->setupZaber(selectedPort, QSerialPort::Baud115200);

        if (status)
            ui->zaberConnectButton->setText(DISCONNECT_BUTTON_TEXT);
    }
    else
    {
        tracker->disconnectZaber();
        ui->zaberConnectButton->setText(CONNECT_BUTTON_TEXT);
    }

    updateStartTrackerButton();
}

void GcomController::updateStartTrackerButton()
{
    if ((tracker->getZaberStatus() != AntennaTracker::AntennaTrackerConnectionState::SUCCESS) ||
        (tracker->getArduinoStatus() != AntennaTracker::AntennaTrackerConnectionState::SUCCESS)) {
        ui->startTrackButton->setEnabled(false);

        // enable calibration
        ui->antennaTrackerCalibrateIMUButton->setEnabled(false);
    }
    else {
        ui->startTrackButton->setEnabled(true);
        ui->startTrackButton->setText(START_TRACKING_BUTTON_TEXT);

        // disable calibration
        ui->antennaTrackerCalibrateIMUButton->setEnabled(true);
    }
}

void GcomController::on_startTrackButton_clicked()
{
    // if not currently tracking, go through tracking setup
    if(!tracker->getAntennaTrackerConnected()) {

        // checks if the GPS override is enabled
        if(ui->antennaTrackerGPSOverrideCheckBox->checkState() == Qt::Checked) {
            float overrideLonBase = ui->antennaTrackerOverrideLongitudeField->text().toFloat();
            float overrideLatBase = ui->antennaTrackerOverrideLatitudeField->text().toFloat();

            // overrides the station base coordinates and returns an error if unsuccessful
            if(!tracker->setStationPos(overrideLonBase, overrideLatBase)) {
                qDebug() << "Override GPS failed";
                return;
            }
        }

        // initiate tracking and update button
        AntennaTracker::AntennaTrackerConnectionState status = tracker->startTracking(mavlinkRelay);
        ui->startTrackButton->setText(STOP_TRACKING_BUTTON_TEXT);

        // checks the tracking status
        if(status == AntennaTracker::AntennaTrackerConnectionState::SUCCESS)
            qDebug() << "both devices started";
        else if(status == AntennaTracker::AntennaTrackerConnectionState::ARDUINO_UNINITIALIZED)
            qDebug() << "arduino not initialized";
        else if(status == AntennaTracker::AntennaTrackerConnectionState::ARDUINO_NOT_OPEN)
            qDebug() << "arduino not open";
        else
            qDebug() << "wrong neighbourhood";
    } else {
        // station is currently tracking, stop it and enable fields
        tracker->stopTracking();
        ui->startTrackButton->setText(START_TRACKING_BUTTON_TEXT);
    }
}

void GcomController::antennaTrackerUpdateStatusGUI(float latitude, float longitude, float elevation, float heading)
{
    // update antenna tracking status params
    ui->antennaTrackerCurrentLongitudeField->setText(QString::number(longitude));
    ui->antennaTrackerCurrentLatitudeField->setText(QString::number(latitude));
    ui->antennaTrackerCurrentElevationField->setText(QString::number(elevation));
    ui->antennaTrackerCurrentHeadingField->setText(QString::number(heading));
}

//===================================================================
// Override Antenna Tracker Methods
//===================================================================
void GcomController::disableAntennaTrackingGUI(bool toggle)
{
    // toggle override methods
    ui->antennaTrackerOverrideSettingsGroup->setDisabled(toggle);

    // toggle calibration methods
    ui->antennaTrackerCalibrationGroup->setDisabled(toggle);
}

void GcomController::on_antennaTrackerGPSOverrideCheckBox_toggled(bool checked)
{
    // update the overide GPS toggle
    tracker->setOverrideGPSToggle(checked);
}

void GcomController::on_antennaTrackerOverrideHeadingCheckBox_toggled(bool checked)
{
    // checked: set heading to user defined value
    if(checked) {
        tracker->setOverrideStationHeading(ui->antennaTrackerOverrideHeadingField->text().toLong());
    } else {
    // unchecked: set heading back to 0
        tracker->setOverrideStationHeading(0);
    }
}

void GcomController::on_antennaTrackerOverrideElevationCheckBox_toggled(bool checked)
{
    // checked: set elevation to user defined value
    if(checked) {
        tracker->setOverrideStationElevation(ui->antennaTrackerOverrideElevationField->text().toLong());
    } else {
    // unchecked: set elevation back to 0
        tracker->setOverrideStationElevation(0);
    }
}

void GcomController::on_antennaTrackerOverrideElevationField_editingFinished()
{
    // if override is checked, and field is modified, update param
    if(ui->antennaTrackerOverrideElevationCheckBox->isChecked()) {
        tracker->setOverrideStationElevation(ui->antennaTrackerOverrideElevationField->text().toLong());
    }
}

void GcomController::on_antennaTrackerOverrideHeadingField_editingFinished()
{
    // if override is checked, and field is modified, update param
    if(ui->antennaTrackerOverrideHeadingCheckBox->isChecked()) {
        tracker->setOverrideStationHeading(ui->antennaTrackerOverrideHeadingField->text().toLong());
    }
}


//===================================================================
// Antenna Tracker Calibration Methods
//===================================================================

void GcomController::on_antennaTrackerCalibrateIMUButton_clicked()
{
    // start IMU calibration method
    tracker->calibrateIMU();
}

//===================================================================
// Utility Methods
//===================================================================
QString GcomController::formatDuration(unsigned long seconds)
{
    // Convert seconds into
    int minutes = seconds / 60;
    seconds = seconds % 60;
    int hours = minutes / 60;
    minutes = minutes % 60;

    return QString("%1:%2:%3").arg(hours).arg(minutes).arg(seconds);
}

void GcomController::on_tabWidget_tabBarClicked(int index)
{
    if (index == 1)
    {
        on_arduinoRefreshButton_clicked();
        on_zaberRefreshButton_clicked();
    }
}
