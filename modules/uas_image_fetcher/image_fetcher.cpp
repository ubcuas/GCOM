//===================================================================
// Includes
//===================================================================
// System Includes
#include <QDir>
#include <QFile>
#include <QDirIterator>
// GCOM Includes
#include "image_fetcher.hpp"
//===================================================================
// Constants
//===================================================================
const QString IMG = "/IMG_";
const QString JPG = ".jpg";

//===================================================================
// Public Class Declaration
//===================================================================
ImageFetcher::ImageFetcher(QString imageDir, QString tagDir, const DCNC *sender)
{
    if(!changeImageDir(imageDir))
        throw "Invalid image directory";
    if(!changeImageDir(tagDir))
        throw "Invalid tag directory";
    connect(sender, &DCNC::receivedImageTaggedData,
        this, &ImageFetcher::handleImageTaggedMessage);
}

ImageFetcher::~ImageFetcher() { }

inline bool ImageFetcher::changeImageDir(QString imageDir)
{
    if(!checkDir(imageDir))
        return false;
    imagePath = imageDir;
    return true;
}

inline bool ImageFetcher::changeTagDir(QString tagDir)
{
    if(!checkDir(tagDir))
        return false;
    tagPath = tagDir;
    return true;
}

inline bool ImageFetcher::checkDir(QString dir)
{
    QFileInfo fileInfo{QFile{dir}};
    return (fileInfo.exists() && fileInfo.isReadable() && fileInfo.isWritable());
}

void ImageFetcher::saveToDisc(QString filePath, unsigned char *data, size_t size)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadWrite))
        file.write(reinterpret_cast<const char *>(data), size);
    file.close();
}

void ImageFetcher::handleImageTaggedMessage(std::shared_ptr<ImageTaggedMessage> message)
{
    QString filePath;
    ImageTaggedMessage *imageTaggedMessage = message.get();
    uint8_t uniqueSeqNum = imageTaggedMessage->sequenceNumber;
    std::vector<uint8_t> imageData = imageTaggedMessage->imageData;

    // A pointer to the image data
    uint8_t *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();
    if ((uniqueSeqNum == 0 && prevSeqNum == 255) || (uniqueSeqNum > prevSeqNum)){
        filePath = imagePath + IMG + QString::number(++imageNum) + JPG;
        saveToDisc(filePath, imageArray, sizeOfData);
        emit taggedImage(filePath);
        prevSeqNum = uniqueSeqNum;
    }
}
