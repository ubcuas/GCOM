#ifndef MOVING_OBSTACLE_HPP
#define MOVING_OBSTACLE_HPP

class MovingObstacle
{

public:

    MovingObstacle(const double altitudeMsl, const double latitude, const double longitude, const double sphereRadius);
    ~MovingObstacle();

    double getLatitude();
    double getLongitude();
    double getAltitudeMsl();
    double getSphereRadius();

private:

    double altitudeMsl;
    double latitude;
    double longitude;
    double sphereRadius;
};

#endif // MOVING_OBSTACLE_HPP
