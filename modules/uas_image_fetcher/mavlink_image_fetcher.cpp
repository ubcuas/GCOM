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
const QString imagePathTemplate = "%1/IMG_%2.jpg";
const QString tagPathTemplate = "%1/TAGS.txt";

MavlinkImageFetcher::MavlinkImageFetcher(QString imageDir, QString tagDir,
                                         const DCNC *dcnc, const MAVLinkRelay *relay)
                                        : ImageFetcher(imageDir, tagDir, dcnc)
{
    this->mavlinkRelay = relay;
    connect(dcnc, &DCNC::receivedImageUntaggedData,
        this, &MavlinkImageFetcher::handleImageUntaggedMessage);
}

MavlinkImageFetcher::~MavlinkImageFetcher() {
    if(tagFile)
        delete tagFile;
    if(mavlinkRelay)
        delete mavlinkRelay;
}

void MavlinkImageFetcher::handleImageUntaggedMessage(std::shared_ptr<ImageUntaggedMessage> message)
{
    ImageUntaggedMessage *imageUntaggedMessage = message.get();
    uint8_t uniqueSeqNum = imageUntaggedMessage->sequenceNumber;

    std::vector<uint8_t> imageData = imageUntaggedMessage->imageData;
    uint8_t *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();
    if ((uniqueSeqNum == 0 && prevSeqNum == 255) || (uniqueSeqNum > prevSeqNum)){
        filePath = QString(imagePathTemplate).arg(imagePath).arg(QString::number(++imageNum));
        saveToDisc(filePath, imageArray, sizeOfData);
        prevSeqNum = uniqueSeqNum;
        imageQueue.enqueue(message);
        handleQueue();
    }
}
void MavlinkImageFetcher::imageGPSTagReceived(std::shared_ptr<mavlink_camera_feedback_t>message){
    tagQueue.enqueue(message);
    QString tagData = QString::number(tagQueue.head()->lat) + ',' +
                      QString::number(tagQueue.head()->lng) + ',' +
                      QString::number(tagQueue.head()->alt_msl) + ',' +
                      QString::number(tagQueue.head()->alt_rel) + ',' +
                      QString::number(tagQueue.head()->yaw);
    tagFile->write(qPrintable(tagData),tagData.length());
    handleQueue();
}

void MavlinkImageFetcher::handleQueue(){
    if(!imageQueue.isEmpty() && !tagQueue.isEmpty()){
        imageQueue.dequeue();
        emit untaggedImage(filePath,
                          tagQueue.head()->lat,
                          tagQueue.head()->lng,
                          tagQueue.head()->alt_msl,
                          tagQueue.head()->alt_rel,
                          tagQueue.head()->yaw);
        tagQueue.dequeue();
    }
}
