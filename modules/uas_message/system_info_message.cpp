//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <string>
// GCOM Includes
#include "system_info_message.hpp"
#include "uas_message.hpp"

//===================================================================
// Constants
//===================================================================
const int SYSTEM_ID_OFFSET = 3;

//===================================================================
// Class Definitions
//===================================================================
SystemInfoMessage::SystemInfoMessage(std::string systemId, uint16_t versionNumber, bool dropped)
{
    this->systemId = systemId;
    this->versionNumber = versionNumber;
    this->dropped = dropped;
}

SystemInfoMessage::SystemInfoMessage(const std::vector<uint8_t> &serializedMessage)
{

    versionNumber = (serializedMessage[0] << 8) |  serializedMessage[1];
    dropped = serializedMessage[2];
    systemId = std::string(serializedMessage.begin() + SYSTEM_ID_OFFSET,serializedMessage.end());

}

SystemInfoMessage::~SystemInfoMessage()
{

}

UASMessage::MessageID SystemInfoMessage::type()
{
    return MessageID::DATA_SYSTEM_INFO;
}

std::vector<uint8_t> SystemInfoMessage::serialize()
{
    std::vector<uint8_t> serializedMessage;
    serializedMessage.push_back(versionNumber >> 8);
    serializedMessage.push_back(versionNumber & 0xFF);
    serializedMessage.push_back(dropped);
    serializedMessage.insert(serializedMessage.end(), systemId.begin(),systemId.end());
    return serializedMessage;
}


