#ifndef UASMESSAGE_HPP
#define UASMESSAGE_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <cstdint>

//===================================================================
// Public Class Declarations
//===================================================================
/*!
 * \brief UASMessage is an interface that any UAS message must inherit inorder to be handled by G-COM or Gremlin
 * \details The UASMessage interface should be subclassed by all message objects regardless of the way they will be transported
 *          UASMessage enforces that each implementation at least have a way to serialize its contents into a byte vector and a way to
 *          reconstruct it using the given array. The vector is specified to be std::vector<uchar> to ensure portability
 *          However, for the sake of simplicity on any given platform additional serialize methods that return other array types can be included.
 */
class UASMessage
{
    public:

        /*!
         * \brief The MeeageIds enum holds all the possible message IDs that UAS's G-COM should be able to handle
         */
        enum class MessageID : uint8_t
        {
            REQUEST                 = 0x0A,
            COMMAND                 = 0x0B,
            RESPONSE                = 0x0C,
            DATA_SYSTEM_INFO        = 0x10,
            DATA_IMU                = 0x11,
            DATA_GPS                = 0x12,
            DATA_IMAGE_UNTAGGED     = 0x13,
            DATA_CAPABILITIES       = 0x14,
            DATA_IMAGE_TAGGED       = 0x15,
            DEBUG                   = 0xFF,
            UNSPECIFIED             = 0xFF
        };

        /*!
         * \brief ~UASMessage a virtual destructor that must be implemented in order for proper polymorphic behavior
         */
        virtual ~UASMessage(){}

        /*!
         * \brief Pure virtual function that returns the type of the message as a MeesageId
         * \return The type of the enclosed message as a MeeageId enum value
         */
        virtual MessageID type()=0;

        /*!
         * \brief Pure virtual function that serializes the message into a unsigned char vector
         * \return An standard unsigned vector containing the message's serialized contents
         */
        virtual std::vector<uint8_t> serialize()=0;
};
#endif // UAS_MESSAGE_HPP
