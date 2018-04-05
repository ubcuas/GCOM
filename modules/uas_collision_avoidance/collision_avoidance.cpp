//===================================================================
// Includes
//===================================================================
// System Includes
#include <Qfile>
#include <QTextStream>
#include "math.h"
#include <QtAlgorithms>

// GCOM Includes
#include "collision_avoidance.hpp"
#include "mission_planner_command.hpp"

// Strings
const QString QGC_VERISON = "QGC WPL 110";

#define earthRadiusKm 6371.0

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

    if(file.open(QFile::ReadWrite|QFile::Truncate)) {
        QTextStream stream(&file);
        stream << QGC_VERISON << endl;
        stream << "0	1	0	0	0	0	0	0	0	0	0	1" << endl;
        for (int waypointIndex = 0; waypointIndex < waypoints.length(); ++waypointIndex) {
            stream << this->generateMissionPlannerCommand(waypoints[waypointIndex]) << endl;
        }
    }
}

QString CollisionAvoidance::generateMissionPlannerCommand(InteropMission::Waypoint waypoint) {
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
            "1"; // auto continue (true/false)
}

void CollisionAvoidance::sortMissionWaypoints(InteropMission::Waypoint waypoints[], int size) {
    int n = size - 1;
    for(int i=0;i<n;++i) {
        for(int j=0;j<n-i;j++) {
            if(waypoints[j].order > waypoints[j+1].order) {
                this->swapWaypoints(waypoints[j], waypoints[j+1]);
            }
        }
    }
}

// 1. Create a sort method for the mission waypoints [done]
// 2. Create a method that iterates through a the list of waypoints, flags where collisions have
//      occured
// 2.5 Given a collision, create a method that calculates the new waypoint that will avoid the
//      object to avoid

//===================================================================
// COLLISION DETECTION RELATED
//===================================================================

bool CollisionAvoidance::collisionDetectedBetweenTwoWaypoints(InteropMission::Waypoint waypointA,
                                           InteropMission::Waypoint waypointB,
                                           QList<StationaryObstacle> obstacles) {
    return false;
}

// calculations
double CollisionAvoidance::longitudeToX(double lon) {
    return lon * 6371000.0 * M_PI / 180;
}

double CollisionAvoidance::latitudeToY(double lat) {
    return log(tan(M_PI_4 + (lat * M_PI / 360))) * 6371000.0;
}

double CollisionAvoidance::distanceBetweenTwoPoints(double Ax, double Ay, double Bx, double By) {
    return sqrt(pow((Ax - Bx),2) + pow((Ay - By),2));
}

double deg2rad(double deg) {
  return (deg * M_PI / 180);
}

double rad2deg(double rad) {
  return (rad * 180 / M_PI);
}

double CollisionAvoidance::distanceOfTwoCoordsKm(double lat1d, double lon1d, double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = deg2rad(lat1d);
  lon1r = deg2rad(lon1d);
  lat2r = deg2rad(lat2d);
  lon2r = deg2rad(lon2d);
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

void CollisionAvoidance::swapWaypoints(InteropMission::Waypoint &a,InteropMission::Waypoint &b) {
    InteropMission::Waypoint temp;
    temp = a;
    a = b;
    b = temp;
}
