//===================================================================
// Includes
//===================================================================
// UAS Includes
#include "uas_message_tcp_framer.hpp"
#include "uas_message.hpp"
#include "request_message.hpp"
#include "system_info_message.hpp"
#include "capabilities_message.hpp"
#include "command_message.hpp"
#include "image_untagged_message.hpp"
#include "image_tagged_message.hpp"
#include "response_message.hpp"
// Qt Includes
#include <QDataStream>
#include <array>
#include <vector>
// System Includes
#include <memory>

#include <QDebug>

//===================================================================
// Constants
//===================================================================
const int FRAMED_MESG_SIZE_FIELD_SIZE = 4;
const int FRAMED_MESG_ID_FIELD_SIZE = 1;
const int FRAMED_MESG_HEADER_FIELD_SIZE = FRAMED_MESG_ID_FIELD_SIZE + FRAMED_MESG_SIZE_FIELD_SIZE;

//===================================================================
// Class Definitions
//===================================================================
UASMessageTCPFramer::UASMessageTCPFramer()
{
    initializeDefaults();
}

void UASMessageTCPFramer::initializeDefaults()
{
    framerStatus = TCPFramerStatus::INVALID_MESSAGE;
    messageData.clear();
}

bool UASMessageTCPFramer::frameMessage(UASMessage &uasMessage)
{
    size_t serializedMessageSize;

    // First we clear whatever message we was currrently stored
    initializeDefaults();
    // First we receive the serialized message and check its size
    messageData = uasMessage.serialize();
    serializedMessageSize= messageData.size();

    // We only allow messages whos size can be represented in 4 Bytes
    if (serializedMessageSize <= 0 || serializedMessageSize > 0xFFFFFFFF)
        return false;

    // Then we append its size to the front of the message (Big Endian)
    for (int sizeFieldByte = 0; sizeFieldByte < FRAMED_MESG_SIZE_FIELD_SIZE; sizeFieldByte++)
         messageData.insert(messageData.begin(), (serializedMessageSize >> (8 * sizeFieldByte ))& 0xFF);

    // Finally append the message ID to the very front of the message
    messageData.insert(messageData.begin(), static_cast<uint8_t>(uasMessage.type()));

    // Update the status and return true
    framerStatus = TCPFramerStatus::SUCCESS;
    return true;
}

std::shared_ptr<UASMessage> UASMessageTCPFramer::generateMessage()
{
    // If there is no valid message then we return a nullptr
    if (framerStatus != TCPFramerStatus::SUCCESS)
        return nullptr;

    // We know that starting from the fifth byte all data will be the message's serial contents
    std::vector<uint8_t> serialMessagePayload(messageData.begin() + FRAMED_MESG_HEADER_FIELD_SIZE,
                                              messageData.end());

    // Next we switch on the type of the message so that we can construct the appropriate object and return it
    switch (static_cast<UASMessage::MessageID>(messageData.front()))
    {
        case UASMessage::MessageID::REQUEST:
        {
            std::shared_ptr<UASMessage> message(new RequestMessage(serialMessagePayload));
            return message;
        }
        case UASMessage::MessageID::RESPONSE:
        {
            std::shared_ptr<UASMessage> message(new ResponseMessage(serialMessagePayload));
            return message;
        }
        case UASMessage::MessageID::DATA_SYSTEM_INFO:
        {
            std::shared_ptr<UASMessage> message(new SystemInfoMessage(serialMessagePayload));
            return message;
        }
        case UASMessage::MessageID::DATA_CAPABILITIES:
        {
            std::shared_ptr<UASMessage> message(new CapabilitiesMessage(serialMessagePayload));
            return message;
        }
        case UASMessage::MessageID::COMMAND:
        {
            std::shared_ptr<UASMessage> message(new CommandMessage(serialMessagePayload));
            return message;
        }
        case UASMessage::MessageID::DATA_IMAGE_UNTAGGED:
        {
            std::shared_ptr<UASMessage> message(new ImageUntaggedMessage(serialMessagePayload));
            return message;
        }
        case UASMessage::MessageID::DATA_IMAGE_TAGGED:
        {
            std::shared_ptr<UASMessage> message(new ImageTaggedMessage(serialMessagePayload));
            return message;
        }
        default:
            framerStatus = TCPFramerStatus::INVALID_MESSAGE;
            return nullptr;
    }
}

void UASMessageTCPFramer::clearMessage()
{
    messageData.clear();
    framerStatus = TCPFramerStatus::INVALID_MESSAGE;
}

UASMessageTCPFramer::TCPFramerStatus UASMessageTCPFramer::status()
{
    return framerStatus;
}

//===================================================================
// Public Operators
//===================================================================
QDataStream& operator>>(QDataStream& inputStream, UASMessageTCPFramer& uasMessageTCPFramer)
{
    uint8_t messageHeader[FRAMED_MESG_HEADER_FIELD_SIZE];
    int bytesRead;
    uint32_t messageSize;

    // Reset the state of the framer to its default state
    uasMessageTCPFramer.initializeDefaults();
    // Most of the errors are as a result of Incomplete messages
    uasMessageTCPFramer.framerStatus = UASMessageTCPFramer::TCPFramerStatus::INCOMPLETE_MESSAGE;
    // Then we need to check the state of the input stream
    if (inputStream.status() != QDataStream::Ok)
        return inputStream;

    // We then attempt to read the message header from the stream
    bytesRead = inputStream.readRawData((char *)&messageHeader, FRAMED_MESG_HEADER_FIELD_SIZE);
    if (bytesRead < FRAMED_MESG_HEADER_FIELD_SIZE)
        return inputStream;

    if (inputStream.status() != QDataStream::Ok)
        return inputStream;

    messageSize = messageHeader[1] << 24 | messageHeader[2] <<  16 | messageHeader[3] << 8 |
                  messageHeader[4];

    qDebug() << messageSize;

    // Change the message buffer to the appropriate size
    uasMessageTCPFramer.messageData.resize(FRAMED_MESG_HEADER_FIELD_SIZE + messageSize);
    uasMessageTCPFramer.messageData.insert(uasMessageTCPFramer.messageData.begin(), messageHeader,
                                           messageHeader + FRAMED_MESG_HEADER_FIELD_SIZE);


    int readLength = inputStream.readRawData((char *)uasMessageTCPFramer.messageData.data() +
                                             FRAMED_MESG_HEADER_FIELD_SIZE,
                                             messageSize);

    // Finally we check if the entire payload was read successfully
    if ((inputStream.status() != QDataStream::Ok) || (readLength != messageSize))
        return inputStream;

    //TODO Validate the ID!

    uasMessageTCPFramer.framerStatus = UASMessageTCPFramer::TCPFramerStatus::SUCCESS;
    return inputStream;
}

QDataStream& operator<<(QDataStream& outputStream, UASMessageTCPFramer& uasMessageTCPFramer)
{
    // If there is no valid message then we return straight away
    if (uasMessageTCPFramer.framerStatus != UASMessageTCPFramer::TCPFramerStatus::SUCCESS)
        return outputStream;

    // Attempt to write the message's contents to the stream
    int writtenBytes = outputStream.writeRawData((char *) uasMessageTCPFramer.messageData.data(),
                                                 uasMessageTCPFramer.messageData.size());

    // If there was an error in writing the data then set the internal flag
    // TODO Add check to verify all the data is in fact written

    if ((outputStream.status() != QDataStream::Ok) || (writtenBytes != uasMessageTCPFramer.messageData.size()))
        uasMessageTCPFramer.framerStatus = UASMessageTCPFramer::TCPFramerStatus::SEND_FAILURE;

    return outputStream;
}

