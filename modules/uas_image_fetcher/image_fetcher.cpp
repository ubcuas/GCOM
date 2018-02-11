//===================================================================
// Includes
//===================================================================
// System Includes
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QDebug>
#include <QDateTime>
#include <QRegExp>
// GCOM Includes
#include "image_fetcher.hpp"
//===================================================================
// Constants
//===================================================================
const QString imagePathTemplate = "%1/IMG_%2.jpg";
const QString tagPathTemplate = "%1/TAGS.txt";
const QString newSectionMessageTemplate = "\n/**NEW SESSION %1**/\n";

//===================================================================
// Public Class Declaration
//===================================================================

// Constructor
ImageFetcher::ImageFetcher(QString dir, const DCNC *sender)
{
    tagFile = nullptr;
    if(!changeDir(dir))
        throw std::invalid_argument("Invalid directory");
    QDateTime dateTime = QDateTime::currentDateTime();
    QString newSectionMessage = QString(newSectionMessageTemplate).arg(dateTime.toString());
    tagFile->write(qPrintable(newSectionMessage),newSectionMessage.length());
    updateImageNum(dir);
    prevSeqNum = 0;

    // Connects DCNC to the ImageFetcher
    connect(sender, &DCNC::receivedImageTaggedData,
        this, &ImageFetcher::handleImageTaggedMessage);
}

// Destructor
ImageFetcher::~ImageFetcher() {
    delete tagFile;
}

// Checks tag directory and updates it if its valid
inline bool ImageFetcher::changeDir(QString dir)
{
    if(!checkDir(dir))
        return false;
    if (tagFile != nullptr)
    {
        tagFile->close();
        delete tagFile;
    }
    fileDirectory = dir;
    tagFile = new QFile(QString(tagPathTemplate).arg(dir));
    tagFile->open(QIODevice::ReadWrite | QIODevice::Append);
    return true;
}
// Checks to see if the directory is valid
inline bool ImageFetcher::checkDir(QString dir)
{
    QFileInfo fileInfo{QFile{dir}};
    return (fileInfo.exists() && fileInfo.isReadable() && fileInfo.isWritable());
}

// Extracts the imageNum of the latest image and sets the current ImageNum to the next value
void ImageFetcher::updateImageNum(QString dir)
{
    imageNum = -1;
    QRegExp imageNumberRegExp("(^IMG_)(\\d+)(.jpg)");
    QDirIterator it(dir, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        QString filename(fileInfo.fileName());
        int pos = imageNumberRegExp.indexIn(filename);
        if (pos < 0)
            continue;
        QString imageNumber = imageNumberRegExp.cap(2);
        if (imageNumber.toInt() > imageNum)
            imageNum = imageNumber.toInt();
    }
    imageNum++;
}

// Saves an image to the disk
void ImageFetcher::saveToDisc(QString filePath, unsigned char *data, size_t size)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadWrite))
        file.write(reinterpret_cast<const char *>(data), size);
    file.close();
}
// Recieves imageTagged Message and saves the image and tags to the appropriate directory
void ImageFetcher::handleImageTaggedMessage(std::shared_ptr<ImageTaggedMessage> message)
{
    QString filePath;
    ImageTaggedMessage *imageTaggedMessage = message.get();
    uint8_t uniqueSeqNum = imageTaggedMessage->sequenceNumber;

    std::vector<uint8_t> imageData = imageTaggedMessage->imageData;
    uint8_t *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();
    // Compares current and previous sequence numbers to ensure uniqueness, emits signal otherwise
    if ((uniqueSeqNum == 0 && prevSeqNum != 255) || (uniqueSeqNum != prevSeqNum + 1)){
        emit skippedFromSeqNumTo(prevSeqNum, uniqueSeqNum);
    }
    filePath = QString(imagePathTemplate).arg(fileDirectory).arg(QString::number(imageNum++));
    saveToDisc(filePath, imageArray, sizeOfData);
    emit taggedImage(filePath,
                     imageTaggedMessage->latitude(),
                     imageTaggedMessage->longitude(),
                     imageTaggedMessage->altitude_abs(),
                     imageTaggedMessage->altitude_rel(),
                     imageTaggedMessage->heading());
    prevSeqNum = uniqueSeqNum;
    QString tagData = QString::number(imageNum) + ", " +
                      QString::number(uniqueSeqNum) + ": " +
                      QString::number(imageTaggedMessage->latitude()) + ',' +
                      QString::number(imageTaggedMessage->longitude()) + ',' +
                      QString::number(imageTaggedMessage->altitude_abs()) + ',' +
                      QString::number(imageTaggedMessage->altitude_rel()) + ',' +
                      QString::number(imageTaggedMessage->heading()) + "\n";
    tagFile->write(qPrintable(tagData),tagData.length());
}
