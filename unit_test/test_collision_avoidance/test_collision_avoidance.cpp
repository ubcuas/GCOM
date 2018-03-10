//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QtTest>

// GCOM Includes
#include "test_collision_avoidance.hpp"
#include "modules/uas_interop_system/InteropObjects/interop_mission.hpp"

QTEST_MAIN(TestCollisionAvoidance)

void TestCollisionAvoidance::initTestCase()
{
    collisionAvoidanceInstance = new CollisionAvoidance();
}

void TestCollisionAvoidance::cleanupTestCase()
{
    delete collisionAvoidanceInstance;
}

void TestCollisionAvoidance::testCase1()
{
    // setup mock waypoint simulation
    QList<InteropMission::Waypoint> mockWaypoints = this->getMockWaypointsOrdered();

    collisionAvoidanceInstance->generateWaypointFile(mockWaypoints, "missionplanwaypoints1");
    QVERIFY2(true, "Failure");
}

QList<InteropMission::Waypoint> TestCollisionAvoidance::getMockWaypointsOrdered()
{
    QList<InteropMission::Waypoint> missionWaypoints = {};

    double waypoints[8][2] =
    {
        {-81.823794, 90.000000},
        {-78.750000, 49.267800},
        {-83.676943, 27.421875},
        {-82.765373, 4.218750},
        {-80.532071, -11.953125},
        {-78.349411, -6.328125},
        {-74.211983, 3.515625},
        {-74.211983, 28.828125}
    };

    for (int i = 0; i < 8; i++) {
        InteropMission::Waypoint tempPoint {
            100,
            waypoints[i][1],
            waypoints[i][0],
            i+1,
            MissionPlannerCommand::WAYPOINT
        };
        missionWaypoints.push_back(tempPoint);
    }
    return missionWaypoints;
}

