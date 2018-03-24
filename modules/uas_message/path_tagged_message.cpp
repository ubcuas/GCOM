//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <QDir>
#include <QFile>
// GCOM Includes
#include "path_tagged_message.hpp"
#include "modules/uas_message/image_tagged_message.hpp"
UASMessage::MessageID PathTaggedMessage::type()
{
    return MessageID::DATA_PATH_TAGGED;
}

std::vector<uint8_t> PathTaggedMessage::serialize()
{
    throw std::invalid_argument("Cannot serialize TaggedPathMessage");
}

std::shared_ptr<ImageTaggedMessage> PathTaggedMessage::toImageTaggedMessage()
{
    QString filePath = reinterpret_cast<const char *>(this->imageData.data());
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray imageArray = file.readAll();
        std::vector<uint8_t> imageData = std::vector<uint8_t>(imageArray.begin(), imageArray.end());
    }
    size_t dataSize = imageData.size();
    std::shared_ptr<ImageTaggedMessage> message(new ImageTaggedMessage(this->sequenceNumber,
                                                                       this->latitudeRaw,
                                                                       this->longitudeRaw,
                                                                       this->altitudeAbsRaw,
                                                                       this->altitudeRelRaw,
                                                                       this->headingRaw,
                                                                       const_cast<uint8_t *>(imageData.data()),
                                                                       dataSize));
    return message;
}
