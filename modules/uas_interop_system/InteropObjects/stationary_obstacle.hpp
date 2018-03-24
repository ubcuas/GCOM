#ifndef STATIONARY_OBSTACLE_HPP
#define STATIONARY_OBSTACLE_HPP

class StationaryObstacle
{

public:

    StationaryObstacle(const double cylinderHeight, const double cylinderRadius, const double latitude, const double longitude);
    ~StationaryObstacle();

    double getCylinderHeight();
    double getCylinderRadius();
    double getLatitude();
    double getLongitude();

private:

    double cylinderHeight;
    double cylinderRadius;
    double latitude;
    double longitude;

};

#endif // STATIONARY_OBSTACLE_HPP
