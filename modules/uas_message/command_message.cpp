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
CommandMessage::CommandMessage(Commands command, std::vector<uint8_t> args)
{
    this->command = command;
    this->args = args;
}

CommandMessage::CommandMessage(const std::vector<uint8_t> &serializedMessage)
{
    command = static_cast<Commands>(serializedMessage.front());
    args.insert(args.begin(), serializedMessage.begin() + 1, serializedMessage.end());
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
    serializedMessage.insert(serializedMessage.end(), args.begin(), args.end());

    return serializedMessage;
}
