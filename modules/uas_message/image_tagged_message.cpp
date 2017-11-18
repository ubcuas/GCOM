//===================================================================
// Includes
//===================================================================
// System Includes
#include <vector>
#include <algorithm>
#include <iterator>
// GCOM Includes
#include "modules/uas_message/image_tagged_message.hpp"

//===================================================================
// Defines
//===================================================================
#define PACK_LAT_LON(x) (double) (x*1e7)
#define PACK_ALT(x) (float) (x*1e3)
#define PACK_HDG(x) (float) (x*1e2)

#define UNPACK_LAT_LON(x) (double) (x/1e7)
#define UNPACK_ALT(x) (float) (x/1e3)
#define UNPACK_HDG(x) (float) (x/1e2)

//===================================================================
// Constants
//===================================================================
const int SIZE_LAT_LON = 4;
const int SIZE_ALT = 4;
const int SIZE_HDG = 2;

const int COORD_OFFSET = 1;
const int ALT_OFFSET = COORD_OFFSET + SIZE_LAT_LON * 2;
const int HDG_OFFSET = ALT_OFFSET + SIZE_ALT * 2;
const int IMAGE_DATA_OFFSET = HDG_OFFSET + SIZE_HDG;

const int LAT_RANGE = 90;
const int LON_RANGE = 180;
// assume altitude is no less than 500m under sea level
const int ALT_RANGE = 500;

//===================================================================
// Class Definitions
//===================================================================
ImageTaggedMessage::ImageTaggedMessage(uint8_t sequenceNumber, double latitude, double longitude,
                                       float altitudeAbs, float altitudeRel, float heading,
                                       uint8_t* imageData, size_t dataSize) :
                                       ImageUntaggedMessage(sequenceNumber, imageData, dataSize)
{
    this->latitudeRaw = PACK_LAT_LON(latitude);
    this->longitudeRaw = PACK_LAT_LON(longitude);
    this->altitudeAbsRaw = PACK_ALT(altitudeAbs);
    this->altitudeRelRaw = PACK_ALT(altitudeRel);
    this->headingRaw = PACK_HDG(heading);
}

ImageTaggedMessage::ImageTaggedMessage(uint8_t sequenceNumber, int32_t latitude, int32_t longitude,
                                       int32_t altitudeAbs, int32_t altitudeRel, uint16_t heading,
                                       uint8_t* imageData, size_t dataSize) :
                                       ImageUntaggedMessage(sequenceNumber, imageData, dataSize)
{
    this->latitudeRaw = latitude;
    this->longitudeRaw = longitude;
    this->altitudeAbsRaw = altitudeAbs;
    this->altitudeRelRaw = altitudeRel;
    this->headingRaw = heading;
}

ImageTaggedMessage::ImageTaggedMessage(const std::vector<uint8_t> &serializedMessage)
{
    sequenceNumber = serializedMessage.front();

    // Reconstruct the image data

    // Extract subvector holding the coordinates
    std::vector<uint8_t> serializedCoords(std::begin(serializedMessage) + COORD_OFFSET,
                     std::begin(serializedMessage) + ALT_OFFSET);

    // Extract subvector holding the altitudes
    std::vector<uint8_t> serializedAlts(std::begin(serializedMessage) + ALT_OFFSET,
                     std::begin(serializedMessage) + HDG_OFFSET);

    // Extract subvector holding the heading
    std::vector<uint8_t> serializedHdg(std::begin(serializedMessage) + HDG_OFFSET,
                     std::begin(serializedMessage) + IMAGE_DATA_OFFSET);

    uint32_t packedLat = 0;
    uint32_t packedLon = 0;
    uint32_t packedAltAbs = 0;
    uint32_t packedAltRel = 0;
    headingRaw = 0;

    for (int byte_index = 0; byte_index < SIZE_LAT_LON; byte_index++)
    {
        packedLat |= (serializedCoords[byte_index] << (8*(SIZE_LAT_LON - byte_index - 1)));
        packedLon |= (serializedCoords[SIZE_LAT_LON + byte_index] << (8*(SIZE_LAT_LON - byte_index - 1)));
    }

    for (int byte_index = 0; byte_index < SIZE_ALT; byte_index++)
    {
        packedAltAbs |= (serializedAlts[byte_index] << (8*(SIZE_ALT - byte_index - 1)));
        packedAltRel |= (serializedAlts[SIZE_ALT + byte_index] << (8*(SIZE_ALT - byte_index - 1)));
    }

    for (int byte_index = 0; byte_index < SIZE_HDG; byte_index++)
    {
        headingRaw |= (serializedHdg[byte_index] << (8*(SIZE_HDG - byte_index - 1)));
    }

    // Convert values back by shifting values back
    latitudeRaw = packedLat - PACK_LAT_LON(LAT_RANGE);
    longitudeRaw = packedLon - PACK_LAT_LON(LON_RANGE);

    altitudeAbsRaw = packedAltAbs - PACK_ALT(ALT_RANGE);
    altitudeRelRaw = packedAltRel - PACK_ALT(ALT_RANGE);

    imageData.assign(serializedMessage.begin() + IMAGE_DATA_OFFSET, serializedMessage.end());
}

ImageTaggedMessage::~ImageTaggedMessage()
{

}

UASMessage::MessageID ImageTaggedMessage::type()
{
    return MessageID::DATA_IMAGE_TAGGED;
}

std::vector<uint8_t> ImageTaggedMessage::serialize()
{
    std::vector<uint8_t> serializedMessage = ImageUntaggedMessage::serialize();

    uint32_t packedLat;
    uint32_t packedLon;
    uint32_t packedAltAbs;
    uint32_t packedAltRel;

    // Pack image data by shifting values so they are unsigned
    packedLat = latitudeRaw + PACK_LAT_LON(LAT_RANGE);
    packedLon = longitudeRaw + PACK_LAT_LON(LON_RANGE);

    packedAltAbs = altitudeAbsRaw + PACK_ALT(ALT_RANGE);
    packedAltRel = altitudeRelRaw + PACK_ALT(ALT_RANGE);

    // Serialize the coordinates
    std::vector<uint8_t> serializedCoords;
    serializedCoords.resize(SIZE_LAT_LON * 2);

    for (int byte_index = 0; byte_index < SIZE_LAT_LON; byte_index++)
    {
        serializedCoords[byte_index] = (packedLat >> (8*(SIZE_LAT_LON - byte_index - 1))) & 0xFF;
        serializedCoords[SIZE_LAT_LON + byte_index] =
                (packedLon >> (8*(SIZE_LAT_LON - byte_index - 1))) & 0xFF;
    }

    // Serialize the altitudes
    std::vector<uint8_t> serializedAlts;
    serializedAlts.resize(SIZE_ALT * 2);

    for (int byte_index = 0; byte_index < SIZE_ALT; byte_index++)
    {
        serializedAlts[byte_index] = (packedAltAbs >> (8*(SIZE_ALT - byte_index - 1))) & 0xFF;
        serializedAlts[SIZE_LAT_LON + byte_index] =
                (packedAltRel >> (8*(SIZE_ALT - byte_index - 1))) & 0xFF;
    }

    // Serialize the heading
    std::vector<uint8_t> serializedHdg;
    serializedHdg.resize(SIZE_HDG);

    for (int byte_index = 0; byte_index < SIZE_HDG; byte_index++)
    {
        serializedHdg[byte_index] = (headingRaw >> (8*(SIZE_HDG - byte_index - 1))) & 0xFF;
    }

    // Insert serialized coordinates into byte vector created by superclass
    serializedMessage.insert(std::begin(serializedMessage) + COORD_OFFSET,
                             std::begin(serializedCoords), std::end(serializedCoords));

    // Insert serialized altitudes
    serializedMessage.insert(std::begin(serializedMessage) + ALT_OFFSET,
                             std::begin(serializedAlts), std::end(serializedAlts));

    // Insert serialized heading
    serializedMessage.insert(std::begin(serializedMessage) + HDG_OFFSET,
                             std::begin(serializedHdg), std::end(serializedHdg));

    return serializedMessage;
}

//===================================================================
// Tag Methods
//===================================================================

double ImageTaggedMessage::latitude() {
    return UNPACK_LAT_LON(latitudeRaw);
}

double ImageTaggedMessage::longitude() {
    return UNPACK_LAT_LON(longitudeRaw);
}

float ImageTaggedMessage::altitude_abs() {
    return UNPACK_ALT(altitudeAbsRaw);
}

float ImageTaggedMessage::altitude_rel() {
    return UNPACK_ALT(altitudeRelRaw);
}

float ImageTaggedMessage::heading() {
    return UNPACK_HDG(headingRaw);
}
