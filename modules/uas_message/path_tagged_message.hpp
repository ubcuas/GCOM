#ifndef TAGGED_PATH_MESSAGE_HPP
#define TAGGED_PATH_MESSAGE_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <memory>
// GCOM Includes
#include "modules/uas_message/uas_message.hpp"
#include "modules/uas_message/image_tagged_message.hpp"
#include "modules/uas_message/image_untagged_message.hpp"

class PathTaggedMessage : public ImageTaggedMessage {
    public:
    /*!
     * \brief type returns the type of the message as a MessageID
     * \return The type of the enclosed message as a MessageID enum value
     */
    MessageID type();

    /*!
     * \brief serialize serializes the message into a unsigned char vector
     * \return A standard unsigned vector containing the message's serialized contents
     */
    std::vector<uint8_t> serialize();

    std::shared_ptr<ImageTaggedMessage> toImageTaggedMessage();

};

#endif // TAGGED_PATH_MESSAGE_HPP
