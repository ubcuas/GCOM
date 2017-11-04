#include "mavlink_image_fetcher.hpp"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"

MavlinkImageFetcher::MavlinkImageFetcher(QString imageDir, QString tagDir,
                                         const DCNC *dcnc, const MAVLinkRelay *relay)
                                        : ImageFetcher(imageDir, tagDir, dcnc)
{
    this->mavlinkRelay = relay;
}

MavlinkImageFetcher::~MavlinkImageFetcher() { }
