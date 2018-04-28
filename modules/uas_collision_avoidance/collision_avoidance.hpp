#ifndef COLLISION_AVOIDANCE_H
#define COLLISION_AVOIDANCE_H

#include "modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp"
#include "modules/uas_interop_system/InteropObjects/interop_mission.hpp"

class CollisionAvoidance
{
public:

    struct Position
    {
        double latitude;
        double longitude;
    };

    CollisionAvoidance();
    ~CollisionAvoidance();

    void generateWaypointFile(QList<InteropMission::Waypoint> waypoints, QString fileNameTag);

private:
    QList<StationaryObstacle> stationaryObstacles;
    QList<InteropMission::Waypoint> missionWaypoints;
    QList<InteropMission::Waypoint> collisionDetectedBetweenTwoWaypoints(InteropMission::Waypoint waypointA,
                                               InteropMission::Waypoint waypointB,
                                               QList<StationaryObstacle> obstacles);
    QList<InteropMission::Waypoint>generateAvoidingWaypoints(Position posA, Position posB,
                                                             StationaryObstacle obstacle);

    bool vectorABInsideObstacleRadius(Position posA, Position posB, StationaryObstacle obstacle);
    QString generateMissionPlannerCommand(InteropMission::Waypoint waypoint);
    void sortMissionWaypoints(InteropMission::Waypoint waypoints[], int size);
    void swapWaypoints(InteropMission::Waypoint &a, InteropMission::Waypoint &b);
    double bearingPosXtoY(Position posA, Position posB);
    double distanceOfTwoCoordsKm(Position posA, Position posB);
    double deg2rad(double deg);
    double rad2deg(double rad);
};

#endif // COLLISION_AVOIDANCE_H
