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
const QString DUP = "/DUP_";
const QString IMG = "/IMG_";
const QString JPG = ".jpg";

//===================================================================
// Public Class Declaration
//===================================================================
ImageTagger::ImageTagger(QString taggedDir, QString untaggedDir, QString TagsDir, const DCNC *sender)
{
    setupTaggedDir(taggedDir);
    setupUntaggedDir(untaggedDir);
    setupTagsDir(TagsDir);
    connect(sender, &DCNC::receivedImageData,
            this, &ImageTagger::handleImageMessage);
}

ImageTagger::~ImageTagger() { }

bool ImageTagger::setupTaggedDir(QString dir)
{
    // QDir homeDirectory(QDir::homePath());
    //if (!homeDirectory.mkdir(dir))
    //     qDebug() << "Can't create taggedDir folder";
    // pathOfDir = QDir::homePath() + "/" + dir;
    return true;
}

bool ImageTagger::setupUntaggedDir(QString dir)
{
    return true;
}

bool ImageTagger::setupTagsDir(QString dir)
{
    return true;
}

void ImageTagger::saveImageToDisc(QString filePath, unsigned char *data, size_t size)
{
    QFile image(filePath);
    if (image.open(QIODevice::ReadWrite))
        image.write(reinterpret_cast<const char *>(data), size);
    image.close();
}


void ImageTagger::handleImageMessage(std::shared_ptr<ImageMessage> message)
{
    QString filePath;
    ImageMessage *imageMessage = message.get();
    unsigned char uniqueSeqNum = imageMessage->sequenceNumber;
    std::vector<unsigned char> imageData = imageMessage->imageData;

    // A pointer to the image data
    unsigned char *imageArray = &imageData[0];
    size_t sizeOfData = imageData.size();

    // If no duplicate is found
    seqNumArr.push_back(uniqueSeqNum);
    filePath = pathOfTagged + IMG + QString::number(++numOfImages) + JPG;
    saveImageToDisc(filePath, imageArray, sizeOfData);
    emit taggedImage(filePath);
}
