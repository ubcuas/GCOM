#include "capabilities_message.hpp"
#include "modules/uas_message/uas_message.hpp"

CapabilitiesMessage::CapabilitiesMessage(Capabilities capabilities)
{
    this->capabilities = capabilities;
}

CapabilitiesMessage::CapabilitiesMessage(const std::vector<unsigned char> &serializedMessage)
{
    uint32_t serializedCapabilities = serializedMessage[0] << 24 | serializedMessage[1] <<  16 |
                                      serializedMessage[2] << 8  | serializedMessage[3];
    this->capabilities = static_cast<CapabilitiesMessage::Capabilities>(serializedCapabilities);
}

UASMessage::MessageID CapabilitiesMessage::type()
{
    return  UASMessage::MessageID::DATA_CAPABILITIES;
}

CapabilitiesMessage::~CapabilitiesMessage()
{

}

std::vector<uint8_t> CapabilitiesMessage::serialize()
{
    std::vector<uint8_t> serializedMessage;
    serializedMessage.push_back(static_cast<uint8_t>(capabilities >> 24) & 0xFF);
    serializedMessage.push_back(static_cast<uint8_t>(capabilities >> 16) & 0xFF);
    serializedMessage.push_back(static_cast<uint8_t>(capabilities >> 8) & 0xFF);
    serializedMessage.push_back(static_cast<uint8_t>(capabilities) & 0xFF);
    return serializedMessage;
}

//===================================================================
// Enum Operator Definitions
//===================================================================
CapabilitiesMessage::Capabilities operator|(const CapabilitiesMessage::Capabilities &a,
                                            const CapabilitiesMessage::Capabilities &b)
{
    return static_cast<CapabilitiesMessage::Capabilities>(static_cast<uint32_t>(a) |
                                                          static_cast<uint32_t>(b));
}

CapabilitiesMessage::Capabilities operator&(const CapabilitiesMessage::Capabilities &a,
                                            const CapabilitiesMessage::Capabilities &b)
{
    return static_cast<CapabilitiesMessage::Capabilities>(static_cast<uint32_t>(a) &
                                                          static_cast<uint32_t>(b));
}

CapabilitiesMessage::Capabilities operator>>(const CapabilitiesMessage::Capabilities &a,
                                             const int &shift_num)
{
    return static_cast<CapabilitiesMessage::Capabilities>(static_cast<uint32_t>(a) >> shift_num);
}

CapabilitiesMessage::Capabilities operator<<(const CapabilitiesMessage::Capabilities &a,
                                             const int &shift_num)
{
    return static_cast<CapabilitiesMessage::Capabilities>(static_cast<uint32_t>(a) << shift_num);
}
