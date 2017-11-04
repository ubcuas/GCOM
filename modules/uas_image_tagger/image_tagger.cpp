//===================================================================
// Includes
//===================================================================
// System Includes
#include <QDir>
#include <QFile>
#include <QDirIterator>
// GCOM Includes
#include "image_tagger.hpp"
//===================================================================
// Constants
//===================================================================
const QString TIMG = "/TAGGED_IMG_";
const QString JPG = ".jpg";

//===================================================================
// Public Class Declaration
//===================================================================
ImageFetcher::ImageFetcher(QString Dir, const DCNC *sender)
{
    if(!changeDir(Dir))
        throw "Invalid directory";
    connect(sender, &DCNC::receivedImageData,
            this, &ImageFetcher::handleImageMessage);
}

ImageFetcher::~ImageFetcher() { }

inline bool ImageFetcher::changeDir(QString dir)
{
    if(!checkDir(dir))
        return false;
    pathOfTagged = dir;
    return true;
}

inline bool ImageFetcher::checkDir(QString dir)
{
    QFileInfo fileInfo{QFile{dir}};
    return (fileInfo.exists() && fileInfo.isReadable() && fileInfo.isWritable());
}

void ImageFetcher::saveImageToDisc(QString filePath, unsigned char *data, size_t size)
{
    QFile image(filePath);
    if (image.open(QIODevice::ReadWrite))
        image.write(reinterpret_cast<const char *>(data), size);
    image.close();
}

void ImageFetcher::handleImageMessage(std::shared_ptr<ImageMessage> message)
{
    QString filePath;
    ImageMessage *imageMessage = message.get();
    uint8_t uniqueSeqNum = imageMessage->sequenceNumber;
    std::vector<uint8_t> imageData = imageMessage->imageData;

    // A pointer to the image data
    uint8_t *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();
    if ((uniqueSeqNum == 0 && prevSeqNum == 255) || (uniqueSeqNum > prevSeqNum)){
        filePath = pathOfTagged + TIMG + QString::number(++numTagged) + JPG;
        saveImageToDisc(filePath, imageArray, sizeOfData);
        emit taggedImage(filePath);
        prevSeqNum = uniqueSeqNum;
    }
}
