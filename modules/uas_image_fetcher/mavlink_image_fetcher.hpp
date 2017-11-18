#ifndef MAVLINK_IMAGE_FETCHER_HPP
#define MAVLINK_IMAGE_FETCHER_HPP

#include "modules/uas_image_fetcher/image_fetcher.hpp"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"


class MavlinkImageFetcher : public ImageFetcher
{
    Q_OBJECT
public:
    MavlinkImageFetcher(QString imageDir, QString tagDir, const DCNC *dcnc, const MAVLinkRelay *relay);

    ~MavlinkImageFetcher();
signals:
    // Data Signals
    void untaggedImage(QString filePath);
    void tag(QString filepath);

private:
    QString imagePath, tagPath;
    int imageNum;
    uint8_t prevSeqNum;
    const MAVLinkRelay* mavlinkRelay;

private slots:
    void imageGPSTagReceived();
    void handleImageUntaggedMessage(std::shared_ptr<ImageUntaggedMessage> message); //FIX THIS LATER
};

#endif // MAVLINK_IMAGE_FETCHER_HPP
