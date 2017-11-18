//===================================================================
// Includes
//===================================================================
// System Includes
#include <QDir>
#include <QFile>
#include <QDirIterator>
// GCOM Includes
#include "mavlink_image_fetcher.hpp"
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"
//===================================================================
// Constants
//===================================================================
const QString IMG = "/IMG_";
const QString JPG = ".jpg";
const QString TAG = "/TAG_";
const QString TXT = ".txt";

MavlinkImageFetcher::MavlinkImageFetcher(QString imageDir, QString tagDir,
                                         const DCNC *dcnc, const MAVLinkRelay *relay)
                                        : ImageFetcher(imageDir, tagDir, dcnc)
{
    this->mavlinkRelay = relay;
    connect(dcnc, &DCNC::receivedImageUntaggedData,
        this, &MavlinkImageFetcher::handleImageUntaggedMessage);
}

MavlinkImageFetcher::~MavlinkImageFetcher() { }

void MavlinkImageFetcher::handleImageUntaggedMessage(std::shared_ptr<ImageUntaggedMessage> message)
{
    QString filePath;
    ImageUntaggedMessage *imageUntaggedMessage = message.get();
    uint8_t uniqueSeqNum = imageUntaggedMessage->sequenceNumber;
    std::vector<uint8_t> imageData = imageUntaggedMessage->imageData;

    // A pointer to the image data
    uint8_t *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();
    if ((uniqueSeqNum == 0 && prevSeqNum == 255) || (uniqueSeqNum > prevSeqNum)){
        filePath = imagePath + IMG + QString::number(++imageNum) + JPG;
        saveToDisc(filePath, imageArray, sizeOfData);
        emit untaggedImage(filePath);
        prevSeqNum = uniqueSeqNum;
    }
}

void MavlinkImageFetcher::imageGPSTagReceived() {

}
