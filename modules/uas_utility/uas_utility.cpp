#include "uas_utility.h"

//===================================================================
// Includes
//===================================================================
// QT Includes
#include <QString>
#include <QList>
#include <QtMath>
#include <QDebug>

// System Includes
#include <vector>
#include <math.h>

//===================================================================
// Constants
//===================================================================
const float RADIUS_EARTH = 6378137;

float Utility::calcHorizontal(std::shared_ptr<mavlink_global_position_int_t> droneGPSData, float yawIMU,
                              float latBase, float lonBase, float headingBase)
{
    // ====================================================================
    // Calculations from http://www.movable-type.co.uk/scripts/latlong.html
    // ====================================================================

    //Grabbing individual pieces of data from gpsData
    const float droneLat = qDegreesToRadians(((float) droneGPSData->lat)/ 10000000);
    const float droneLon = qDegreesToRadians(((float) droneGPSData->lon)/ 10000000);

    const float yDiff = (droneLon-lonBase);

    const float y = sin(yDiff) * cos(droneLat);
    const float x = (cos(latBase) * sin(droneLat)) - (sin(latBase) * cos(droneLat) * cos(yDiff));

    const float horizAngle = fmod((qRadiansToDegrees(atan2(y,x)) + 360), 360);

    // Find the quickest angle to reach the point
    float horzAngleDiff = horizAngle - (yawIMU - headingBase);

    if(horzAngleDiff > 180) {
        horzAngleDiff -= 360;
    }
    else if(horzAngleDiff < -180) {
        horzAngleDiff += 360;
    }
    else if(horzAngleDiff > -1 && horzAngleDiff < 1) {
        // don't move if angle is too small to reduce drifting
        horzAngleDiff = 0;
    }

    return horzAngleDiff;
}

float Utility::calcVertical(std::shared_ptr<mavlink_global_position_int_t> droneGPSData, float pitchIMU,
                             float latBase, float lonBase, float elevationBase)
{
    // ====================================================================
    // Calculations from http://www.movable-type.co.uk/scripts/latlong.html
    // ====================================================================

    const float droneLat = qDegreesToRadians(((float) droneGPSData->lat)/ 10000000);
    const float droneLon = qDegreesToRadians(((float) droneGPSData->lon)/ 10000000);

    const float droneRelativeAlt = ((float)droneGPSData->relative_alt / 1000);

    const float xDiff = (droneLat-latBase);
    const float yDiff = (droneLon-lonBase);

    const float a = pow(sin(xDiff/2),2)+cos(latBase) * cos(droneLat)*pow(sin(yDiff/2),2);
    const float d = 2 * atan2(sqrt(a), sqrt(1-a));
    const float distance = RADIUS_EARTH * d;

    const float vertAngle = atan((droneRelativeAlt-elevationBase)/distance)*180/M_PI;

    float vertAngleDiff = vertAngle - pitchIMU;

    if(vertAngleDiff > -1 && vertAngleDiff < 1) {
        // don't move if angle is too small to reduce drifting
        vertAngleDiff = 0;
    }

    return vertAngleDiff;
}
