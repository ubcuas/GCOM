//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <algorithm>
#include <iterator>
// GCOM Includes
#include "modules/uas_message/image_untagged_message.hpp"
#include "modules/uas_message/uas_message.hpp"

//===================================================================
// Class Definitions
//===================================================================
ImageUntaggedMessage::ImageUntaggedMessage(uint8_t sequenceNumber,
                           uint8_t* imageData, size_t dataSize)
{
    this->sequenceNumber = sequenceNumber;
    this->imageData.assign(imageData, imageData + dataSize / sizeof(char));
}

ImageUntaggedMessage::ImageUntaggedMessage(const std::vector<uint8_t> &serializedMessage)
{
    sequenceNumber = serializedMessage.front();
    imageData.assign(serializedMessage.begin() + 1, serializedMessage.end());
}

ImageUntaggedMessage::ImageUntaggedMessage()
{

}

ImageUntaggedMessage::~ImageUntaggedMessage()
{

}

UASMessage::MessageID ImageUntaggedMessage::type()
{
    return MessageID::DATA_IMAGE_UNTAGGED;
}

std::vector<uint8_t> ImageUntaggedMessage::serialize()
{
    std::vector<uint8_t> serializedMessage;
    serializedMessage.push_back(sequenceNumber);
    serializedMessage.insert(std::end(serializedMessage),
                             std::begin(imageData), std::end(imageData));
    return serializedMessage;
}
