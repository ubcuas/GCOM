#ifndef COLLISION_AVOIDANCE_H
#define COLLISION_AVOIDANCE_H

#include "modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp"
#include "modules/uas_interop_system/InteropObjects/interop_mission.hpp"

class CollisionAvoidance
{
public:
    enum class MissionPlannerCommand : uint8_t
    {
        WAYPOINT = 16,
        SPLINE_WAYPOINT = 82,
        LOITER_TURNS = 18,
        LOITER_TIME = 19,
        LOITER_UNLIM = 17,
        RETURN_TO_LAUNCH = 20,
        LAND = 21,
        TAKEOFF = 22,
        DELAY = 93,
        GUIDED_ENABLED = 92,
        PAYLOAD_PLACE = 94,
        DO_GUIDED_LIMITS = 222,
        DO_SET_ROI = 201,
        CONDITION_DELAY = 112,
        CONDITION_CHANGE_ALT = 113,
        CONDITION_DISTANCE = 114,
        CONDITION_YAW = 115,
        DO_JUMP = 177,
        DO_CHANGE_SPEED = 178,
        DO_GRIPPER = 211,
        DO_PARACHUTE = 208,
        DO_SET_CAM_TRIGG_DIST = 206,
        DO_SET_RELAY = 181,
        DO_REPEAT_RELAY = 182,
        DO_SET_SERVO = 183,
        DO_REPEAT_SERVO = 184,
        DO_DIGICAM_CONFIGURE = 202,
        DO_DIGICAM_CONTROL = 203,
        DO_MOUNT_CONTROL = 205
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
