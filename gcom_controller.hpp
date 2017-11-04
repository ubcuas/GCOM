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
#include "modules/uas_message/image_message.hpp"
#include "modules/uas_antenna_tracker/antennatracker.hpp"
#include "modules/uas_image_tagger/image_tagger.hpp"

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
    void dcncTimerTimeout();
    void dcncSearchTimeout();
    void gremlinInfo(QString systemId, uint16_t versionNumber, bool dropped);
    void gremlinCapabilities(CapabilitiesMessage::Capabilities capabilities);
    // Antenna Tracker Slots
    void on_arduinoConnectButton_clicked();
    void on_zaberRefreshButton_clicked();
    void on_zaberConnectButton_clicked();
    void on_startTrackButton_clicked();

    // Image Tagger Slots
    void on_taggerLocationImagesField_returnPressed();
    void on_taggerLocationTagsField_returnPressed();

    void on_taggerLocationImagesButton_clicked();
    void on_taggerLocationTagsButton_clicked();

    void on_taggerImageTransferButton_clicked();

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

    // Image Tagger Variables
    ImageTagger *tagger;
    QString currentDir;

    int taggerStatus;

    // Image Tagger methods

    /*!
     * \brief setupImageFetcher, initialize image fetcher based on which camera the drone has
     * \param camera, camera type - with or without tags
     */
    void setupImageFetcher(CapabilitiesMessage::Capabilities camera);

    /*!
     * \brief taggerBrowseDir, open file dialog, allow user to change directories
     * \param locationType, type of file to save - images, tags
     */
    void taggerBrowseDir(const int locationType);

    /*!
     * \brief enableTabMain, enables or disables tabs in the tab group tabMain
     * \param tab, tab to enable or disable
     * \param enable, true to enable, false to disable
     */
    void enableTabMain(const int tab, bool enable);
};

#endif // GCOMCONTROLLER_HPP
