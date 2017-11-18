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
const QString imagePathTemplate = "%1/IMG_%2.jpg";
const QString tagPathTemplate = "%1/TAGS.txt";

//===================================================================
// Public Class Declaration
//===================================================================
ImageFetcher::ImageFetcher(QString imageDir, QString tagDir, const DCNC *sender)
{
    if(!changeImageDir(imageDir))
        throw "Invalid image directory";
    if(!changeImageDir(tagDir))
        throw "Invalid tag directory";
    tagFile = new QFile(QString(tagPathTemplate).arg(tagPath));
    tagFile->open(QIODevice::ReadWrite);

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
    tagFile->close();
    delete tagFile;
    tagPath = tagDir;
    tagFile = new QFile(QString(tagPathTemplate).arg(tagPath));
    tagFile->open(QIODevice::ReadWrite);
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
    uint8_t *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();
    if ((uniqueSeqNum == 0 && prevSeqNum == 255) || (uniqueSeqNum > prevSeqNum)){
        filePath = QString(imagePathTemplate).arg(imagePath).arg(QString::number(++imageNum));
        saveToDisc(filePath, imageArray, sizeOfData);
        emit taggedImage(filePath,
                         imageTaggedMessage->latitude(),
                         imageTaggedMessage->longitude(),
                         imageTaggedMessage->altitude_abs(),
                         imageTaggedMessage->altitude_rel(),
                         imageTaggedMessage->heading());
        prevSeqNum = uniqueSeqNum;
    }
    QString tagData = QString::number(imageTaggedMessage->latitude()) + ',' +
                      QString::number(imageTaggedMessage->longitude()) + ',' +
                      QString::number(imageTaggedMessage->altitude_abs()) + ',' +
                      QString::number(imageTaggedMessage->altitude_rel()) + ',' +
                      QString::number(imageTaggedMessage->heading());
    tagFile->write(qPrintable(tagData),tagData.length());

}
