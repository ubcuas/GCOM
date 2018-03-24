#include "moving_obstacle.hpp"

MovingObstacle::MovingObstacle(const double altitudeMsl, const double latitude, const double longitude, const double sphereRadius)
{
    this->altitudeMsl = altitudeMsl;
    this->latitude = latitude;
    this->longitude = longitude;
    this->sphereRadius = sphereRadius;
}

MovingObstacle::~MovingObstacle()
{
    // do nothing
}

double MovingObstacle::getLatitude()
{
    return this->latitude;
}

double MovingObstacle::getLongitude()
{
    return this->longitude;
}

double MovingObstacle::getAltitudeMsl()
{
    return this->altitudeMsl;
}

double MovingObstacle::getSphereRadius()
{
    return this->sphereRadius;
}
