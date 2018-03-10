//===================================================================
// Includes
//===================================================================
// System Includes
#include <Qfile>
#include <QTextStream>
#include "math.h"

// GCOM Includes
#include "collision_avoidance.hpp"
#include "mission_planner_command.hpp"

// Strings
const QString QGC_VERISON = "QGC WPL 110";

//===================================================================
// Constructor / Deconstructor
//===================================================================

CollisionAvoidance::CollisionAvoidance() {
    // retrieve mission waypoints from interop
    // missionWaypoints = mission.getMissionWaypoints();

    // will also eventually retrieve from interop
    stationaryObstacles = {};
}

CollisionAvoidance::~CollisionAvoidance() {
    // do nothing
}

//===================================================================
// Methods
//===================================================================

void CollisionAvoidance::generateWaypointFile(QList<InteropMission::Waypoint> waypoints, QString fileNameTag) {
    // waypoint file format:
    // QGC WPL <VERSION>
    // <INDEX> <CURRENT WP> <COORD FRAME> <COMMAND> <PARAM1> <PARAM2> <PARAM3> <PARAM4> <PARAM5/X/LONGITUDE> <PARAM6/Y/LATITUDE> <PARAM7/Z/ALTITUDE> <AUTOCONTINUE>

    QString filename = fileNameTag + ".waypoints";
    QFile file(filename);

    if(file.open(QFile::ReadWrite)) {
        QTextStream stream(&file);
        stream << QGC_VERISON << endl;
        stream << "0	1	0	0	0	0	0	0	0	0	0	1" << endl;
        for (int waypointIndex = 0; waypointIndex < waypoints.length(); ++waypointIndex) {
            stream << this->generateMissionPlannerPlan(waypoints[waypointIndex]) << endl;
        }
    }
}

QString CollisionAvoidance::generateMissionPlannerPlan(InteropMission::Waypoint waypoint) {
    return QString::number(waypoint.order) + "\t"
            "0" + "\t" + // current wp (true/false)
            "3" + "\t" + // coord frame
            QString::number(waypoint.command) + "\t" + // command
            "0" + "\t" + // param1
            "0" + "\t" + // param2
            "0" + "\t" + // param3
            "0" + "\t" + // param4
            QString::number(waypoint.longitude) + "\t" +
            QString::number(waypoint.latitude) + "\t" +
            QString::number(waypoint.altitudeMsl) + "\t" +
            "1";
}

//===================================================================
// COLLISION DETECTION RELATED
//===================================================================

bool CollisionAvoidance::collisionDetectedBetweenTwoWaypoints(InteropMission::Waypoint waypointA,
                                           InteropMission::Waypoint waypointB,
                                           QList<StationaryObstacle> obstacles) {
    return false;
}

// calculations
float CollisionAvoidance::longitudeToX(float lon) {
    return lon * 6371000.0 * M_PI / 180;
}

float CollisionAvoidance::latitudeToY(float lat) {
    return log(tan(M_PI_4 + (lat * M_PI / 360))) * 6371000.0;
}

double distanceBetweenTwoPoints(double Ax, double Ay, double Bx, double By) {
    return sqrt(pow((Ax - Bx),2) + pow((Ay - By),2));
}

// def is_between(a,c,b):
//    return distance(a,c) + distance(c,b) == distance(a,b)
