//===================================================================
// Includes
//===================================================================
// GCOM Includes
#include "uas_message.hpp"
#include "gps_message.hpp"
// System Includes
#include <vector>

#include <QDebug>

//===================================================================
// Defines
//===================================================================
#define SECONDS_LAT_LON(x) (float)(((x % 10000) / 1e4) * 60)
#define MINUTES_LAT_LON(x) (float) ((x/10000) % 100)
#define DEGREES_LAT_LON(x) (float) (x/1000000)

#define PACK_LAT_LON(x) (float) (x*1e4)

//===================================================================
// Constants
//===================================================================
const int SIZE_LAT_LON = 4;

//===================================================================
// Class Definitions
//===================================================================
GPSMessage::GPSMessage(const std::vector<uint8_t> &serializedMessage)
{
    // TODO Size Check

    // Reconstruct the latitude and longitude
    int32_t packedLat = 0;
    int32_t packedLon = 0;
    for (int byte_index = 0; byte_index < SIZE_LAT_LON; byte_index++)
    {
        packedLat |= (serializedMessage[byte_index] << (8*(SIZE_LAT_LON - byte_index - 1)));
        packedLon |= (serializedMessage[SIZE_LAT_LON + byte_index] << (8*(SIZE_LAT_LON - byte_index - 1)));
    }

    // Unpacks DD:MM:SS to decimal GPS format.
    lat = DEGREES_LAT_LON(packedLat) + (MINUTES_LAT_LON(packedLat)/60) + (SECONDS_LAT_LON(packedLat)/3600);
    lon = DEGREES_LAT_LON(packedLon) + (MINUTES_LAT_LON(packedLon)/60) + (SECONDS_LAT_LON(packedLon)/3600);
}

UASMessage::MessageID GPSMessage::type()
{
    return UASMessage::MessageID::DATA_GPS;
}

std::vector<uint8_t> GPSMessage::serialize()
{
    std::vector<uint8_t> serializedMessage;
    serializedMessage.resize(SIZE_LAT_LON * 2);
    // Pack the latitude and longitude
    int32_t packedLat = PACK_LAT_LON(lat);
    int32_t packedLon = PACK_LAT_LON(lon);

    for (int byte_index = 0; byte_index < SIZE_LAT_LON; byte_index++)
    {
        serializedMessage[byte_index] = (packedLat >> (8*(SIZE_LAT_LON - byte_index - 1))) & 0xFF;
        serializedMessage[SIZE_LAT_LON + byte_index] = (packedLon >> (8*(SIZE_LAT_LON - byte_index - 1))) & 0xFF;
    }

    return serializedMessage;
}
