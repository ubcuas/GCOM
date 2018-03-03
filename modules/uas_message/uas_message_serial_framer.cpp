//===================================================================
// Includes
//===================================================================
// UAS Includes
#include "uas_message_serial_framer.hpp"
#include "uas_message.hpp"
#include "modules/uas_message/gps_message.hpp"
#include "modules/uas_message/imu_message.hpp"
#include "request_message.hpp"
#include "system_info_message.hpp"
// Qt Includes
#include <QDataStream>
#include <array>
#include <vector>
// System Includes
#include <memory>

//===================================================================
// Defines
//===================================================================
#define FinishBlock(X) (*code_ptr = (X), code_ptr = dst++, code = 0x01)

//===================================================================
// Constants
//===================================================================
const int SIZE_FIELD_ID = 1;
const int SIZE_FIELD_SIZE = 1;
const int SIZE_HEADER = SIZE_FIELD_ID + SIZE_FIELD_SIZE;
const int COBS_OVERHEAD = 2;

//===================================================================
// Class Definitions
//===================================================================
UASMessageSerialFramer::UASMessageSerialFramer()
{
    initializeDefaults();
}

void UASMessageSerialFramer::initializeDefaults()
{
    framerStatus = SerialFramerStatus::INVALID_MESSAGE;
    messageData.clear();
    messageData.resize(0);
}

void UASMessageSerialFramer::clearMessage()
{
    framerStatus = SerialFramerStatus::INVALID_MESSAGE;
    messageData.clear();
}

UASMessageSerialFramer::SerialFramerStatus UASMessageSerialFramer::status()
{
    return framerStatus;
}

bool UASMessageSerialFramer::frameMessage(UASMessage &uasMessage)
{
    // First we receive the serialized message and check its size
    messageData = uasMessage.serialize();
    size_t serializedMessageSize = messageData.size();
    // We only allow messages whos size can be represented in 2 Bytes
    if (serializedMessageSize <= 0 || serializedMessageSize > 0xFFFF)
        return false;
    // Then we append its size to the front of the message (Big Endian)
    messageData.insert(messageData.begin(), serializedMessageSize & 0xFF);
    // Append the message ID to the very front of the message
    messageData.insert(messageData.begin(), static_cast<unsigned char>(uasMessage.type()));
    // Calculate Flecher's 16
    appendFletchers16(messageData);
    // Apply the cobs
    encodeCOBS(messageData);
    // update the status
    framerStatus = UASMessageSerialFramer::SerialFramerStatus::SUCCESS;
    return true;
}

std::shared_ptr<UASMessage> UASMessageSerialFramer::generateMessage()
{
    if (framerStatus != UASMessageSerialFramer::SerialFramerStatus::SUCCESS)
        return nullptr;

    std::vector<unsigned char> serializedMessage(messageData.data() + SIZE_HEADER,
                                                 messageData.data() + messageData.size());
    switch((UASMessage::MessageID)messageData[0])
    {
        case UASMessage::MessageID::DATA_GPS:
        {
            std::shared_ptr<GPSMessage>returnMessage(new GPSMessage(serializedMessage));
            return returnMessage;
        }
        break;
        case UASMessage::MessageID::DATA_IMU:
        {
            std::shared_ptr<IMUMessage>returnMessage(new IMUMessage(serializedMessage));
            return returnMessage;
        }
        break;
        default:
        break;
    }

    return nullptr;
}

//===================================================================
// Utility Functions
//===================================================================

uint16_t UASMessageSerialFramer::caculateFletchers16(const std::vector<unsigned char> &messageData)
{
    auto data = messageData.begin();
    size_t bytes = messageData.size();

    uint16_t sum1 = 0xff, sum2 = 0xff;
    size_t tlen;

    while (bytes)
    {
        tlen = ((bytes >= 20) ? 20 : bytes);
        bytes -= tlen;
        do
        {
            sum2 += sum1 += *data++;
            tlen--;
        } while (tlen);

        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
    }
    // Second reduction step to reduce sums to 8 bits
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);

    return (sum2 << 8) | sum1 ;
}

void UASMessageSerialFramer::appendFletchers16(std::vector<unsigned char> &messageData)
{
    uint16_t checksum = caculateFletchers16(messageData);

    messageData.push_back(checksum >> 8);
    messageData.push_back(checksum & 0xFF);
}

void UASMessageSerialFramer::encodeCOBS(std::vector<unsigned char> &messageData)
{
    std::vector<unsigned char> encodedMessageData;
    encodedMessageData.resize(messageData.size() + COBS_OVERHEAD);

    uint8_t *dst = encodedMessageData.data();
    uint8_t *ptr = messageData.data();
    const uint8_t *end = ptr + messageData.size();
    uint8_t *code_ptr = dst++;
    uint8_t code = 0x01;

    while (ptr < end)
    {
      if (*ptr == 0)
        FinishBlock(code);
      else
      {
        *dst++ = *ptr;
        if (++code == 0xFF)
          FinishBlock(code);
      }
      ptr++;
    }

    FinishBlock(code);
    messageData.assign(encodedMessageData.begin(), encodedMessageData.end());
}

bool UASMessageSerialFramer::decodeCOBS(std::vector<unsigned char> &messageData)
{
    std::vector<unsigned char> decodedMessageData;
    decodedMessageData.resize(messageData.size());
    const uint8_t *ptr = messageData.data();
    uint8_t *dst = decodedMessageData.data();
    const uint8_t *end = ptr + messageData.size();


    while (ptr < end)
    {
        int code = *ptr++;
        for (int i = 1; ptr < end && i < code; i++)
          *dst++ = *ptr++;
        if (code < 0xFF)
          *dst++ = 0;
    }

    if (ptr > end)
        return false;

    messageData.assign(decodedMessageData.begin(), decodedMessageData.end());
    messageData.resize(messageData.size() - COBS_OVERHEAD);
    return true;
}


//===================================================================
// Class Operators
//===================================================================
QDataStream& operator<<(QDataStream& outputStream, UASMessageSerialFramer& uasMessageSerialFramer)
{
    // If there is no valid message then we return straight away
    if (uasMessageSerialFramer.framerStatus != UASMessageSerialFramer::SerialFramerStatus::SUCCESS)
        return outputStream;

    // Attempt to write the message's contents to the stream
    int writtenBytes = outputStream.writeRawData(reinterpret_cast<char*>(uasMessageSerialFramer.messageData.data()),
                                                 uasMessageSerialFramer.messageData.size());

    // If there was an error in writing the data then set the internal flag
    if ((outputStream.status() != QDataStream::Ok) ||
        (writtenBytes != uasMessageSerialFramer.messageData.size()))
        uasMessageSerialFramer.framerStatus = UASMessageSerialFramer::SerialFramerStatus::SEND_FAILURE;

    uasMessageSerialFramer.framerStatus = UASMessageSerialFramer::SerialFramerStatus::SUCCESS;
    return outputStream;
}

QDataStream& operator>>(QDataStream& inputStream, UASMessageSerialFramer& uasMessageSerialFramer)
{
    int bytesRead;
    char messageByte = 0;
    uint16_t retrievedChecksum, calculatedChecksum;
    // Reset the state of the framer to its default state
    uasMessageSerialFramer.initializeDefaults();
    // Most of the errors are as a result of Incomplete messages
    uasMessageSerialFramer.framerStatus = UASMessageSerialFramer::SerialFramerStatus::INCOMPLETE_MESSAGE;
    // Then we need to check the state of the input stream
    if (inputStream.status() != QDataStream::Ok)
        return inputStream;

    do
    {
        bytesRead = inputStream.readRawData(&messageByte, 1);
        if (bytesRead < 1)
            return inputStream;

        if (inputStream.status() != QDataStream::Ok)
            return inputStream;

        uasMessageSerialFramer.messageData.push_back(messageByte);

    } while(messageByte != 0);

    // Attempt to decode the COBS
    if(!UASMessageSerialFramer::decodeCOBS(uasMessageSerialFramer.messageData))
    {
        uasMessageSerialFramer.framerStatus = UASMessageSerialFramer::SerialFramerStatus::COBS_FAILURE;
        return inputStream;
    }

    // Validate the checksum
    retrievedChecksum = uasMessageSerialFramer.messageData.back();
    uasMessageSerialFramer.messageData.pop_back();
    retrievedChecksum |= uasMessageSerialFramer.messageData.back() << 8;
    uasMessageSerialFramer.messageData.pop_back();

    calculatedChecksum =  UASMessageSerialFramer::caculateFletchers16(uasMessageSerialFramer.messageData);
    if(calculatedChecksum  !=retrievedChecksum)
    {
        uasMessageSerialFramer.framerStatus = UASMessageSerialFramer::SerialFramerStatus::FAILED_FLETCHERS;
        return inputStream;
    }

    // TODO: Validate that the message ID falls within valid range of messages
    uasMessageSerialFramer.framerStatus = UASMessageSerialFramer::SerialFramerStatus::SUCCESS;
    return inputStream;
}
