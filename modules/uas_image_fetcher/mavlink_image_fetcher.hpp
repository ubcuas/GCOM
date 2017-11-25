#ifndef MAVLINK_IMAGE_FETCHER_HPP
#define MAVLINK_IMAGE_FETCHER_HPP

#include "modules/uas_image_fetcher/image_fetcher.hpp"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"
#include <QQueue>

class MavlinkImageFetcher : public ImageFetcher
{
    Q_OBJECT

public:
    MavlinkImageFetcher(QString imageDir, QString tagDir, const DCNC *dcnc, const MAVLinkRelay *relay);

    ~MavlinkImageFetcher();

    void handleQueue();

signals:
    // Data Signals
    void untaggedImage(QString filePath, double latitude, double longitude,
                       float altitude_abs, float altitude_rel, float heading);

private slots:
    void imageGPSTagReceived(std::shared_ptr<mavlink_camera_feedback_t>message);
    void handleImageUntaggedMessage(std::shared_ptr<ImageUntaggedMessage> message);

private:
    QString filePath;
    QQueue<std::shared_ptr<ImageUntaggedMessage>> imageQueue;
    QQueue<std::shared_ptr<mavlink_camera_feedback_t>> tagQueue;
    const MAVLinkRelay* mavlinkRelay;
};


#endif // MAVLINK_IMAGE_FETCHER_HPP
