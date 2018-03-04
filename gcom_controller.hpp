#ifndef GCOMCONTROLLER_HPP
#define GCOMCONTROLLER_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QMainWindow>
#include <QString>
#include <QMovie>
#include <QTimer>
// GCOM Includes
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_message/image_untagged_message.hpp"
#include "modules/uas_antenna_tracker/antennatracker.hpp"
#include "modules/uas_image_fetcher/image_fetcher.hpp"
#include "modules/uas_interop_system/interop.hpp"

//===================================================================
// Namespace Declarations
//===================================================================
namespace Ui
{
    class GcomController;
}

//===================================================================
// Class Declarations
//===================================================================
class GcomController : public QMainWindow
{
    friend class TestGcomControllerImageFetcher;

    Q_OBJECT

public:
    explicit GcomController(QWidget *parent = 0);
    ~GcomController();

private slots:
    // UI Slots
    void on_mavlinkConnectionButton_clicked();
    void on_dcncConnectionButton_clicked();
    void on_arduinoRefreshButton_clicked();
    void on_tabMain_tabBarClicked(int index);
    void on_dcncDropGremlin_clicked();
    // MAVLinkRelay Slots
    void mavlinkRelayConnected();
    void mavlinkRelayDisconnected();
    void mavlinkTimerTimeout();
    // DCNC Slots
    void dcncConnected();
    void dcncDisconnected();
    void dcncReestablishedConnection(CommandMessage::Commands command,
                                     ResponseMessage::ResponseCodes response);
    void dcncTimerTimeout();
    void dcncSearchTimeout();
    void gremlinInfo(QString systemId, uint16_t versionNumber, bool dropped);
    void gremlinCapabilities(CapabilitiesMessage::Capabilities capabilities);
    // Antenna Tracker Slots
    void on_arduinoConnectButton_clicked();
    void on_zaberRefreshButton_clicked();
    void on_zaberConnectButton_clicked();
    void on_startTrackButton_clicked();
    void on_antennaTrackerGPSOverrideCheckBox_toggled(bool checked);
    void on_antennaTrackerCalibrateIMUButton_clicked();
    void on_antennaTrackerOverrideHeadingCheckBox_toggled(bool checked);
    void on_antennaTrackerOverrideElevationCheckBox_toggled(bool checked);
    void on_antennaTrackerOverrideElevationField_editingFinished();
    void on_antennaTrackerOverrideHeadingField_editingFinished();
    void antennaTrackerUpdateStatusGUI(float latitude, float longitude, float elevation, float heading);
    void disableAntennaTrackingGUI(bool toggle);
    // AUVSI Interop Slots
    void on_interopConnectButton_clicked();

    // Image Fetcher Slots
    void on_fetcherPathField_returnPressed();

    void on_fetcherPathField_textChanged();

    void on_fetcherPathButton_clicked();

    void on_fetcherImageTransferButton_clicked();

private:
    // Private Member Variables
    Ui::GcomController *ui;
    // Private Member Methods
    QString formatDuration(unsigned long seconds);

    // MAVLinkRelay Variables
    MAVLinkRelay *mavlinkRelay;
    QTimer *mavlinkConnectionTimer;
    unsigned long mavlinkConnectionTime;
    QMovie *mavlinkConnectingMovie;
    QMovie *mavlinkConnectedMovie;
    bool mavlinkButtonDisconnect;
    // Methods
    void restMavlinkGUI();

    // DCNC Variables
    DCNC *dcnc;
    QTimer *dcncConnectionTimer;
    QTimer *dcncSearchTimeoutTimer;
    unsigned long dcncConnectionTime;
    QMovie *dcncConnectingMovie;
    QMovie *dcncConnectedMovie;
    // Methods
    void resetDCNCGUI();

    // Antenna Tracker Variables
    AntennaTracker *tracker;
    // Methods
    void updateStartTrackerButton();

    // Image Fetcher Variables
    ImageFetcher *fetcher;
    int fetcherStatus;

    // Image Fetcher methods

    /*!
     * \brief setupImageFetcher, initialize image fetcher based on which camera the drone has
     * \param camera, camera type - with or without tags
     */
    void setupImageFetcher(CapabilitiesMessage::Capabilities camera);

    /*!
     * \brief fetcherBrowseDir, open file dialog, allow user to change directories
     */
    void fetcherBrowseDir();

    /*!
     * \brief validatePath, validate path against regex
     * \details If path is invalid, show error message and disable start image transfer button
     *          If path is valid and error message is still showing, hide error message
     *          If start image transfer button is disabled and no error messages are showing,
     *          enable it
     * \param path, path to validate
     */
    void validatePath(QString path);

    // Utility Methods
    /*!
     * \brief enableTabMain, enables or disables tabs in the tab group tabMain
     * \param tab, tab to enable or disable
     * \param enable, true to enable, false to disable
     */
    void enableTabMain(const int tab, const bool enable);

    Interop *interop;
};

#endif // GCOMCONTROLLER_HPP
