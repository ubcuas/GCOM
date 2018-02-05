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
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
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
const QRegExp IP_REGEX("^[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}$");

// tabMain Constants
const bool TAB_ENABLE = true;
const bool TAB_DISABLE = false;
const int TAB_IMAGE_FETCHER = 2;

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
const QString START_SERVER_FAIL_TEXT("Cannot listen on the given server ip address and port.");

// Capabilities Constants
const int SIZE_CAPABILITY = 8;
const QString CAMERA_TAGGED_TEXT("Camera with tags");
const QString CAMERA_UNTAGGED_TEXT("Camera without tags");

// Image Fetcher Constants
#if defined(Q_OS_WIN)
    const QRegExp PATH_REGEX(
            "^([a-zA-z]:)?"
            "/"
            "([^. <>:\"/\\\\|?*][^<>:\"/\\\\|?*]*/)*"
            "([^. <>:\"/\\\\|?*][^<>:\"/\\\\|?*]*)*$");
#elif defined(Q_OS_MACOS)
    const QRegExp PATH_REGEX("^([~])?/([^.:/][^:/]*/)*([^.:/][^:/]*)*$");
#elif defined(Q_OS_LINUX)
    const QRegExp PATH_REGEX("^([~])?/([^/]+/)*([^/]+)*$");
#else
#error
#endif

const int PATH_IMAGES = 0;
const int PATH_TAGS = 1;

const QString FETCHER_READY_LABEL("<font color='#05c400'> READY </font>");
const QString FETCHER_TRANSFER_LABEL("<font color='#05c400'> TRANSFERRING </font>");
const QString FETCHER_INVALID_PATH_LABEL("<font color='#D52D2D'> *Invalid Path </font>");

const QString IMAGE_TRANSER_START_TEXT("Start Image Transfer");
const QString IMAGE_TRANSER_STOP_TEXT("Stop Image Transfer");

const int FETCHER_STATUS_UNAVAILABLE = 0;
const int FETCHER_STATUS_READY = 1;
const int FETCHER_STATUS_TRANSFERRING = 2;


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

    // Set fetcher to nullptr so it is only initialized once
    fetcher = nullptr;
    fetcherStatus = FETCHER_STATUS_UNAVAILABLE;

    enableTabMain(TAB_IMAGE_FETCHER, TAB_DISABLE);
}

GcomController::~GcomController()
{
    delete ui;
    delete mavlinkRelay;
    delete mavlinkConnectionTimer;
    delete mavlinkConnectingMovie;
    delete dcnc;
    delete tracker;
    delete fetcher;
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

    // If currently transferring images, reset status
    if (fetcherStatus == FETCHER_STATUS_TRANSFERRING) {
        fetcherStatus = FETCHER_STATUS_READY;
        ui->fetcherStatusField->setText(FETCHER_READY_LABEL);
        ui->fetcherImageTransferButton->setText(IMAGE_TRANSER_START_TEXT);
    }

    enableTabMain(TAB_IMAGE_FETCHER, TAB_DISABLE);
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
            ui->dcncServerPortField->setDisabled(true);
            ui->dcncServerTimeoutField->setDisabled(true);

            status = dcnc->startServer(
                        ui->dcncServerIPField->text(),
                        ui->dcncServerPortField->text().toInt());

            // TODO Add a warning message
            if (status == false) {
                QMessageBox::information(
                            this,
                            GcomController::objectName().toStdString().c_str(),
                            START_SERVER_FAIL_TEXT.toStdString().c_str());
                resetDCNCGUI();
                return;
            }

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

    ui->dcncCapabilitiesField->clear();

    // If currently transferring images, reset status
    if (fetcherStatus == FETCHER_STATUS_TRANSFERRING) {
        fetcherStatus = FETCHER_STATUS_READY;
        ui->fetcherStatusField->setText(FETCHER_READY_LABEL);
        ui->fetcherImageTransferButton->setText(IMAGE_TRANSER_START_TEXT);
    }

    enableTabMain(TAB_IMAGE_FETCHER, TAB_DISABLE);
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
     while (static_cast<uint32_t>(capabilities)) {
         if (static_cast<uint32_t>(capabilities &
                                   CapabilitiesMessage::Capabilities::CAMERA_TAGGED))
         {
             enableTabMain(TAB_IMAGE_FETCHER, TAB_ENABLE);
             if (fetcher == nullptr)
                setupImageFetcher(CapabilitiesMessage::Capabilities::CAMERA_TAGGED);
             ui->dcncCapabilitiesField->addItem(CAMERA_TAGGED_TEXT);
         }
         else if (static_cast<uint32_t>(capabilities &
                                   CapabilitiesMessage::Capabilities::CAMERA_UNTAGGED))
         {
             enableTabMain(TAB_IMAGE_FETCHER, TAB_ENABLE);
             if (fetcher == nullptr)
                setupImageFetcher(CapabilitiesMessage::Capabilities::CAMERA_UNTAGGED);
             ui->dcncCapabilitiesField->addItem(CAMERA_UNTAGGED_TEXT);
         }
         // Remove capabilities that have already been used
         capabilities = capabilities >> SIZE_CAPABILITY;
     }
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
    qDebug() << "hello";
    if (tracker->getDeviceStatus(AntennaTracker::AntennaTrackerSerialDevice::ARDUINO)
            != AntennaTracker::AntennaTrackerConnectionState::SUCCESS)
    {
        qDebug() << "hi";
        QModelIndex selectedIndex = ui->availableArduinoPorts->currentIndex();
        QString selectedPort = selectedIndex.data().toString();

        bool status = tracker->setupDevice(selectedPort, QSerialPort::Baud9600,
                             AntennaTracker::AntennaTrackerSerialDevice::ARDUINO);
        if (status)
        {
            ui->arduinoConnectButton->setText(DISCONNECT_BUTTON_TEXT);
            qDebug() << "Hey";
        }
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
    if (tracker->getDeviceStatus(AntennaTracker::AntennaTrackerSerialDevice::ZABER)
            != AntennaTracker::AntennaTrackerConnectionState::SUCCESS)
    {
        QModelIndex selectedIndex = ui->availableZaberPorts->currentIndex();
        QString selectedPort = selectedIndex.data().toString();

        bool status = tracker->setupDevice(selectedPort, QSerialPort::Baud9600,
                             AntennaTracker::AntennaTrackerSerialDevice::ZABER);
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
    if ((tracker->getDeviceStatus(AntennaTracker::AntennaTrackerSerialDevice::ZABER)
         != AntennaTracker::AntennaTrackerConnectionState::SUCCESS) ||
        (tracker->getDeviceStatus(AntennaTracker::AntennaTrackerSerialDevice::ARDUINO)
         != AntennaTracker::AntennaTrackerConnectionState::SUCCESS))
        ui->startTrackButton->setEnabled(false);
    else
        ui->startTrackButton->setEnabled(true);
}

void GcomController::on_startTrackButton_clicked()
{
    AntennaTracker::AntennaTrackerConnectionState status = tracker->startTracking(mavlinkRelay);

    if(status == AntennaTracker::AntennaTrackerConnectionState::SUCCESS)
        qDebug() << "both devices started";
    else if(status == AntennaTracker::AntennaTrackerConnectionState::ARDUINO_UNINITIALIZED)
        qDebug() << "arduino not initialized";
    else if(status == AntennaTracker::AntennaTrackerConnectionState::ARDUINO_NOT_OPEN)
        qDebug() << "arduino not open";
    else
        qDebug() << "wrong neighbourhood";
}

//===================================================================
// Image Fetcher Methods
//===================================================================
void GcomController::setupImageFetcher(CapabilitiesMessage::Capabilities camera) {
    QDir dir;
    QString currentDir = dir.currentPath();

    // Initialize fetcher with default current working directory paths
    switch(camera) {
        case CapabilitiesMessage::Capabilities::CAMERA_TAGGED:
            fetcher = new ImageTagger(currentDir, currentDir, currentDir, dcnc);
            break;
        default:
            break;
    }

    ui->fetcherPathImagesField->setText(currentDir);
    ui->fetcherPathTagsField->setText(currentDir);

    ui->fetcherPathImagesField->setValidator(new QRegExpValidator(PATH_REGEX));
    ui->fetcherPathTagsField->setValidator(new QRegExpValidator(PATH_REGEX));

    ui->fetcherPathImagesInvalidLabel->setText(FETCHER_INVALID_PATH_LABEL);
    ui->fetcherPathImagesInvalidLabel->hide();
    ui->fetcherPathTagsInvalidLabel->setText(FETCHER_INVALID_PATH_LABEL);
    ui->fetcherPathTagsInvalidLabel->hide();

    ui->fetcherStatusField->setText(FETCHER_READY_LABEL);
    fetcherStatus = FETCHER_STATUS_READY;
}

void GcomController::on_fetcherPathImagesButton_clicked()
{
    fetcherBrowseDir(PATH_IMAGES);
}

void GcomController::on_fetcherPathTagsButton_clicked()
{
    fetcherBrowseDir(PATH_TAGS);
}

void GcomController::on_fetcherPathImagesField_returnPressed()
{
    ui->fetcherPathImagesField->clearFocus();
}

void GcomController::on_fetcherPathTagsField_returnPressed()
{
    ui->fetcherPathTagsField->clearFocus();
}

void GcomController::on_fetcherPathImagesField_editingFinished()
{
    // If path field has not been modified, do not need to validate
    if (!ui->fetcherPathImagesField->isModified())
        return;

    validatePath(ui->fetcherPathImagesField->text(), PATH_IMAGES);
    // Reset field to not modified
    ui->fetcherPathImagesField->setModified(false);
}

void GcomController::on_fetcherPathTagsField_editingFinished()
{
    // If path field has not been modified, do not need to validate
    if (!ui->fetcherPathTagsField->isModified())
        return;

    validatePath(ui->fetcherPathTagsField->text(), PATH_TAGS);
    // Reset field to not modified
    ui->fetcherPathTagsField->setModified(false);
}

void GcomController::fetcherBrowseDir(const int pathType) {
    QDir dir;
    QString currentDir = dir.currentPath();

    // Open file dialog, allows user to select a folder and saves the path to a string
    QString folderPath = QFileDialog::getExistingDirectory(
                            this,
                            "Select Folder",
                            currentDir,
                            QFileDialog::ShowDirsOnly);

    // Check if directory has been changed
    if (!folderPath.length())
        return;

    // Update path fields
    switch(pathType) {
        case PATH_IMAGES:
        {
            ui->fetcherPathImagesField->setText(folderPath);
            validatePath(folderPath, PATH_IMAGES);
        }
        break;
        case PATH_TAGS:
        {
            ui->fetcherPathTagsField->setText(folderPath);
            validatePath(folderPath, PATH_TAGS);
        }
    }
}

void GcomController::validatePath(QString path, const int pathType) {
    // If the path is invalid, show error message and disable the start image transfer button
    // If the path is valid and the error message is showing, hide the error message
    switch(pathType)
    {
        case PATH_IMAGES:
        {
            if(!PATH_REGEX.exactMatch(path))
            {
                ui->fetcherPathImagesInvalidLabel->show();
                ui->fetcherImageTransferButton->setEnabled(false);
            }
            else if (!ui->fetcherPathImagesInvalidLabel->isHidden())
            {
                ui->fetcherPathImagesInvalidLabel->hide();
            }
        }
        break;
        case PATH_TAGS:
        {
            if(!PATH_REGEX.exactMatch(path))
            {
                ui->fetcherPathTagsInvalidLabel->show();
                ui->fetcherImageTransferButton->setEnabled(false);
            }
            else if (!ui->fetcherPathTagsInvalidLabel->isHidden())
            {
                ui->fetcherPathTagsInvalidLabel->hide();
            }
        }
    }

    if (ui->fetcherImageTransferButton->isEnabled())
        return;

    // If the start image transfer button is disabled and path errors have been fixed, enable it
    if (ui->fetcherPathImagesInvalidLabel->isHidden() &&
        ui->fetcherPathTagsInvalidLabel->isHidden())
    {
        ui->fetcherImageTransferButton->setEnabled(true);
    }
}

void GcomController::on_fetcherImageTransferButton_clicked()
{
    switch(fetcherStatus)
    {
        case FETCHER_STATUS_READY:
        {
            // Show error message if paths are invalid,
            // Hide if paths are valid and messages are still showing
            if (!fetcher->checkImagesDir(ui->fetcherPathImagesField->text()))
            {
                ui->fetcherPathImagesInvalidLabel->show();
            }
            else if (ui->fetcherPathImagesInvalidLabel->isVisible())
            {
                ui->fetcherPathImagesInvalidLabel->hide();
            }

            if (!fetcher->checkTagsDir(ui->fetcherPathTagsField->text()))
            {
                ui->fetcherPathTagsInvalidLabel->show();
            }
            else if (ui->fetcherPathTagsInvalidLabel->isVisible())
            {
                ui->fetcherPathTagsInvalidLabel->hide();
            }

            // If either path is invalid, do not start image transfer
            if (ui->fetcherPathImagesInvalidLabel->isVisible() ||
                ui->fetcherPathTagsInvalidLabel->isVisible())
                return;

            dcnc->startImageRelay();
            ui->fetcherStatusField->setText(FETCHER_TRANSFER_LABEL);
            ui->fetcherImageTransferButton->setText(IMAGE_TRANSER_STOP_TEXT);
            fetcherStatus = FETCHER_STATUS_TRANSFERRING;
        }
        break;

        case FETCHER_STATUS_TRANSFERRING:
        {
            dcnc->stopImageRelay();
            ui->fetcherStatusField->setText(FETCHER_READY_LABEL);
            ui->fetcherImageTransferButton->setText(IMAGE_TRANSER_START_TEXT);
            fetcherStatus = FETCHER_STATUS_READY;
        }
    }
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

void GcomController::on_tabMain_tabBarClicked(int index)
{
    if (index == 1)
    {
        on_arduinoRefreshButton_clicked();
        on_zaberRefreshButton_clicked();
    }
}

void GcomController::enableTabMain(const int tab, const bool enable) {
     ui->tabMain->setTabEnabled(tab, enable);
}
