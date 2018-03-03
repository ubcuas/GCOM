#ifndef COMMANDMESSAGE_H
#define COMMANDMESSAGE_H

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
// GCOM Includes
#include "uas_message.hpp"

//===================================================================
// Public Class Declarations
//===================================================================
class CommandMessage : public UASMessage
{
    public:

        // Public Definitions
        /*!
         * \brief The Commands enum describes all possible commands that can be sent to the Gremlin.
         *        The meanings of the commands can be found in the relavent confluence pages
         */
        enum class Commands : uint8_t
        {
            SYSTEM_RESET            = 0x01,
            SYSTEM_PAUSE            = 0x02,
            SYSTEM_RESUME           = 0x03,
            IMAGE_RELAY_START    = 0x04,
            IMAGE_RELAY_STOP     = 0x05,
            // Special Command used for responses to requests
            DATA_REQUEST            = 0xFA
        };

        //Public Methods
        /*!
         * \brief CommandMessage's constructor creates a command message to send to the drone
         * \param [in] command, The command to send to the Gremlin
         */
        CommandMessage(Commands command);

        /*!
         * \brief CommandMessage constructor designed to initialize a message using a serialized payload
         * \param [in] serializedMessage a byte vector containing the object's serialized contents
         */
        CommandMessage(const std::vector<uint8_t> &serializedMessage);

        /*!
         * \brief ~CommandMessage destroys the message and frees all internally allocated memory
         */
        ~CommandMessage();


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
         * \brief The type of command that was requested
         */
        Commands command;
};

#endif // COMMANDMESSAGE_H
