#ifndef COLLISION_AVOIDANCE_H
#define COLLISION_AVOIDANCE_H

#include "modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp"
#include "modules/uas_interop_system/InteropObjects/interop_mission.hpp"

class CollisionAvoidance
{
public:
    enum class MissionPlannerCommands : uint8_t
    {
        // Mission Planner Commands
        WAYPOINT = 16,
        SPLINE_WAYPOINT = 82,
        LOITER_TURNS = 18,
        LOITER_TIME = 19,
        LOITER_UNLIM = 17,
        RETURN_TO_LAUNCH = 20,
        LAND = 21,
        TAKEOFF = 22,
        DELAY = 93,
        GUIDED_ENABLE = 92,
        PAYLOAD_PLACE = 94,
        DO_CHANGE_SPEED = 178
    };

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
