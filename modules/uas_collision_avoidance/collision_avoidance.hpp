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

private:
    QString generateMissionPlannerCommand(InteropMission::Waypoint waypoint);

    float longitudeToX(float);
    float latitudeToY(float);

    QList<InteropMission::Waypoint> missionWaypoints;
    QList<StationaryObstacle> stationaryObstacles;

    bool collisionDetectedBetweenTwoWaypoints(InteropMission::Waypoint waypointA,
                                               InteropMission::Waypoint waypointB,
                                               QList<StationaryObstacle> obstacles);
};

#endif // COLLISION_AVOIDANCE_H
