#ifndef RESPONSEMESSAGE_H
#define RESPONSEMESSAGE_H

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <cstdint>
// GCOM Includes
#include "uas_message.hpp"
#include "command_message.hpp"

//===================================================================
// Public Class Declarations
//===================================================================
class ResponseMessage : public UASMessage
{
    public:
        // Public Definitions
        /*!
         * \brief The ResponseCodes enum describes all the possible responses to any command
         *        message
         */
        enum class ResponseCodes : uint8_t
        {
            NO_ERROR         =   0x00,
            INVALID_COMMAND  =   0x01,
            INVALID_REQUEST  =   0x02
        };

        /*!
         * \brief ResponseMessage
         * \param command
         * \param responseCode
         */
        ResponseMessage(CommandMessage::Commands command, ResponseCodes responseCode);

        /*!
         * \brief RequestMessage constructor designed to initialize a message using a serialized payload
         * \param [in] serializedMessage a byte vector containing the object's serialized contents
         */
        ResponseMessage(const std::vector<uint8_t> &serializedMessage);

        /*!
         * \brief ~ResponseMessage destroys the message and frees all internally allocated memory
         */
        ~ResponseMessage();

        /*!
         * \brief type returns the type of the message as a MeesageId
         * \return The type of the enclosed message as a MeeageId enum value
         */
        MessageID type();

        /*!
         * \brief serialize serializes the message into a unsigned char vector
         * \return An standard unsigned vector containing the message's serialized contents
         */
        std::vector<uint8_t> serialize();

        /*!
         * \brief command, returns the command that this response is targeted towards
         */
        CommandMessage::Commands command;

        /*!
         * \brief response, returns the response code
         */
        ResponseCodes response;
};

#endif // RESPONSEMESSAGE_H
