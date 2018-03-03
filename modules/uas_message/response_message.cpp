//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
// GCOM Includes
#include "uas_message.hpp"
#include "command_message.hpp"
#include "response_message.hpp"

//===================================================================
// Constants
//===================================================================
const int COMMAND_FILED_INDEX = 0;
const int RESPONSE_FIELD_INDEX = 1;

//===================================================================
// Class Definitions
//===================================================================
ResponseMessage::ResponseMessage(CommandMessage::Commands command, ResponseCodes responseCode)
{
    this->command = command;
    this->response = responseCode;
}

ResponseMessage::ResponseMessage(const std::vector<uint8_t> &serializedMessage)
{
    command = static_cast<CommandMessage::Commands>(serializedMessage[COMMAND_FILED_INDEX]);
    response = static_cast<ResponseCodes>(serializedMessage[RESPONSE_FIELD_INDEX]);
}

ResponseMessage::~ResponseMessage()
{

}

UASMessage::MessageID ResponseMessage::type()
{
    return MessageID::RESPONSE;
}

std::vector<uint8_t> ResponseMessage::serialize()
{
    std::vector<uint8_t> serializedMessage;
    serializedMessage.push_back((uint8_t)command);
    serializedMessage.push_back((uint8_t)response);
    return serializedMessage;
}
