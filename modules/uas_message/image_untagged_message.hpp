#ifndef IMAGE_UNTAGGED_MESSAGE_HPP
#define IMAGE_UNTAGGED_MESSAGE_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
// GCOM Includes
#include "modules/uas_message/uas_message.hpp"

//===================================================================
// Public Class Declaration
//===================================================================
/*!
 * \brief The Image Tagger module is responsible for tagging the
 *        images sent from the drone in-flight and saving it to disk
 *
 * \details Curent implementation only consists of constructors and
 *          serialization method
 *
 */
class ImageUntaggedMessage: public UASMessage {
    public:
        // Public Member Methods
        /*!
         * \brief ImageUntaggedMessage constructor
         * \param [in] sequenceNumber a byte indicating the current image
         * \param [in] imageData[] a byte array holding the data of the current image
         * \param [in] dataSize size_t indicating the size of the array
         */
        ImageUntaggedMessage(uint8_t sequenceNumber, uint8_t* imageData, size_t dataSize);

        /*!
         * ~ImageUntaggedMessage deconstructor
         */
        ~ImageUntaggedMessage();

        /*!
         * \brief ImageUntaggedMessage constructor to initialize a message using a serialized payload
         * \param [in] serializedMessage a byte vector containing the object's serialized contents
         */
        ImageUntaggedMessage(const std::vector<uint8_t> &serializedMessage);

        /*!
         * \brief type returns the type of the message as a MeesageId
         * \return The type of the enclosed message as a MeeageId enum value
         */
        MessageID type();

        /*!
         * \brief serialize serializes the message into a unsigned char vector
         * \return A standard unsigned vector containing the message's serialized contents
         */
        std::vector<uint8_t> serialize();

        /*!
         * \brief getSequenceNumber returns the variable sequenceNumber
         */
        uint8_t sequenceNumber;

        /*!
         * \brief imageData returns the image data as a byte vector
         */
        std::vector<uint8_t> imageData;

    protected:
        /*!
         * \brief ImageUntaggedMessage default constructor for ImageTaggedMessage to call
         */
        ImageUntaggedMessage();
};

#endif // IMAGE_UNTAGGED_MESSAGE_HPP
