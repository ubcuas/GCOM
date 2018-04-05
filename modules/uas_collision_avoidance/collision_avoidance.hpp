#ifndef COLLISION_AVOIDANCE_H
#define COLLISION_AVOIDANCE_H

#include "modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp"
#include "modules/uas_interop_system/InteropObjects/interop_mission.hpp"

class CollisionAvoidance
{
public:
    CollisionAvoidance();
    ~CollisionAvoidance();

    void generateWaypointFile(QList<InteropMission::Waypoint> waypoints, QString fileNameTag);

    QString generateMissionPlannerCommand(InteropMission::Waypoint waypoint);
    static double longitudeToX(double);
    static double latitudeToY(double);
    static double distanceBetweenTwoPoints(double Ax, double Ay, double Bx, double By);
    bool collisionDetectedBetweenTwoWaypoints(InteropMission::Waypoint waypointA,
                                               InteropMission::Waypoint waypointB,
                                               QList<StationaryObstacle> obstacles);
    void sortMissionWaypoints(InteropMission::Waypoint waypoints[], int size);
    double distanceOfTwoCoordsKm(double lat1d, double lon1d, double lat2d, double lon2d);
    void swapWaypoints(InteropMission::Waypoint &a, InteropMission::Waypoint &b);

private:
    QList<InteropMission::Waypoint> missionWaypoints;
    QList<StationaryObstacle> stationaryObstacles;
};

#endif // COLLISION_AVOIDANCE_H
