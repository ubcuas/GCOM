#include "stationary_obstacle.hpp"

StationaryObstacle::StationaryObstacle(const double cylinderHeight, const double cylinderRadius, const double latitude, const double longitude)
{
    this->cylinderHeight = cylinderHeight;
    this->cylinderRadius = cylinderRadius;
    this->latitude = latitude;
    this->longitude = longitude;
}

StationaryObstacle::~StationaryObstacle()
{
    // do nothing
}

double StationaryObstacle::getLatitude()
{
    return this->latitude;
}

double StationaryObstacle::getLongitude()
{
    return this->longitude;
}

double StationaryObstacle::getCylinderHeight()
{
    return this->cylinderHeight;
}

double StationaryObstacle::getCylinderRadius()
{
    return this->cylinderRadius;
}
