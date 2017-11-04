#ifndef MAVLINK_IMAGE_FETCHER_HPP
#define MAVLINK_IMAGE_FETCHER_HPP

#include <modules/uas_image_fetcher/image_fetcher.hpp>
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"


class MavlinkImageFetcher : public ImageFetcher
{
public:
    MavlinkImageFetcher(QString imageDir, QString tagDir, const DCNC *dcnc, const MAVLinkRelay *relay);

    ~MavlinkImageFetcher();

private:

    const MAVLinkRelay* mavlinkRelay;

private slots:
    void imageGPSTagReceived();
    void handleImageMessage(std::shared_ptr<ImageMessage> message); //FIX THIS LATER
};

#endif // MAVLINK_IMAGE_FETCHER_HPP
