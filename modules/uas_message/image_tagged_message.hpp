#ifndef IMAGE_TAGGED_MESSAGE_HPP
#define IMAGE_TAGGED_MESSAGE_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
// GCOM Includes
#include "modules/uas_message/uas_message.hpp"
#include "modules/uas_message/image_untagged_message.hpp"

//===================================================================
// Public Class Declaration
//===================================================================
/*!
 * \brief The Image Tagger module is responsible for tagging the
 *        images sent from the drone in-flight and saving it to disk
 *
 * \details ImageTaggedMessage includes GPS coordinates of the image
 * \details Curent implementation only consists of constructors and
 *          serialization method
 * \details GPS coordinates as int32_t are converted from doubles by a factor of 1e7
 * \details Altitudes as int32_t are converted from floats by a factor of 1e3
 * \details Heading as uint16_t is converted from float by a factor of 1e2
 *
 */
class ImageTaggedMessage : public ImageUntaggedMessage {
    public:
        // Public Member Methods
        /*!
         * \brief ImageTaggedMessage constructor
         * \param [in] sequenceNumber a byte indicating the current image
         * \param [in] latitude of the current image
         * \param [in] longitude of the current image
         * \param [in] absolute altitude of the current image
         * \param [in] relative altitude of the current image
         * \param [in] heading of the current image
         * \param [in] imageData[] a byte array holding the data of the current image
         * \param [in] dataSize size_t indicating the size of the array
         */
        ImageTaggedMessage(uint8_t sequenceNumber, double latitude, double longitude,
                           float altitudeAbs, float altitudeRel, float heading,
                           uint8_t* imageData, size_t dataSize);

        /*!
         * \brief ImageTaggedMessage constructor
         * \param [in] sequenceNumber a byte indicating the current image
         * \param [in] latitude of the current image as a converted int32_t
         * \param [in] longitude of the current image as a converted int32_t
         * \param [in] absolute altitude of the current image as a converted int32_t
         * \param [in] relative altitude of the current image as a converted int32_t
         * \param [in] heading of the current image as a converted uint16_t
         * \param [in] imageData[] a byte array holding the data of the current image
         * \param [in] dataSize size_t indicating the size of the array
         */
        ImageTaggedMessage(uint8_t sequenceNumber, int32_t latitude, int32_t longitude,
                           int32_t altitudeAbs, int32_t altitudeRel, uint16_t heading,
                           uint8_t* imageData, size_t dataSize);

        /*!
         * ~ImageTaggedMessage deconstructor
         */
        ~ImageTaggedMessage();

        /*!
         * \brief ImageTaggedMessage constructor to initialize a message using a serialized payload
         * \param [in] serializedMessage a byte vector containing the object's serialized contents
         */
        ImageTaggedMessage(const std::vector<uint8_t> &serializedMessage);

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
         * \brief Converts latitude to double
         * \return latitude as a double
         */
        double latitude();

        /*!
         * \brief Converts longitude to double
         * \return longitude as a double
         */
        double longitude();

        /*!
         * \brief Converts absolute altitude to float
         * \return absolute altitude as float
         */
        float altitude_abs();

        /*!
         * \brief Converts relative altitude to float
         * \return relative altitude as float
         */
        float altitude_rel();

        /*!
         * \brief Converts heading to float
         * \return heading as float
         */
        float heading();

        // Public member variables

        /*!
         * \brief The latitude of the image in degrees (-90 to 90) as int32_t packed using 1e7
         */
        int32_t latitudeRaw;

        /*!
         * \brief The longitude of the image in degrees (-180 to 180) as int32_t packed using 1e7
         */
        int32_t longitudeRaw;

        /*!
         * \brief The altitude of the image above mean sea level in metres
         *        as int32_t packed using 1e3
         */
        int32_t altitudeAbsRaw;

        /*!
         * \brief The altitude of the image above ground in metres as int32_t packed using 1e3
         */
        int32_t altitudeRelRaw;

        /*!
         * \brief The heading (yaw angle) in degrees (0.0-359.99) as uint16_t packed using 1e2
         * \details If unknown, will be set to UINT16_MAX
         */
        uint16_t headingRaw;
};

#endif // IMAGE_TAGGED_MESSAGE_HPP
