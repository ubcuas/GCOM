//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
// GCOM Includes
#include "command_message.hpp"
#include "uas_message.hpp"

//===================================================================
// Class Definitions
//===================================================================
CommandMessage::CommandMessage(Commands command)
{
    this->command = command;
}

CommandMessage::CommandMessage(const std::vector<uint8_t> &serializedMessage)
{
    this->command = static_cast<Commands>(serializedMessage.front());
}

CommandMessage::~CommandMessage()
{

}

UASMessage::MessageID CommandMessage::type()
{
    return MessageID::COMMAND;
}

std::vector<uint8_t> CommandMessage::serialize()
{
    std::vector<unsigned char> serializedMessage;
    serializedMessage.push_back(static_cast<unsigned char>(command));
    return serializedMessage;
}
