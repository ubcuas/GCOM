//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QtTest>

// GCOM Includes
#include "test_collision_avoidance.hpp";
#include "../../modules/uas_interop_system/InteropObjects/interop_mission.hpp";

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
    QList<InteropMission::Waypoint> mockWaypoints = this->getMockWaypoints();

    collisionAvoidanceInstance->generateWaypointFile(mockWaypoints);
    QVERIFY2(true, "Failure");
}

QList<InteropMission::Waypoint> TestCollisionAvoidance::getMockWaypoints()
{
    QList<InteropMission::Waypoint> missionWaypoints = {};

    double waypoints[8][2] =
    {
        {49.037868, -81.562500},
        {49.267805,	-82.617188},
        {49.267805,	-78.750000},
        {33.431441,	-81.914063},
        {32.546813,	-90.000000},
        {32.842674,	-104.765625},
        {29.993002,	-141.679688},
        {22.593726,	-147.128906}
    };

    for (int i = 0; i < 8; i++) {
        InteropMission::Waypoint tempPoint {100, waypoints[i][0], waypoints[i][1], i+1};
        missionWaypoints.push_back(tempPoint);
    }
    return missionWaypoints;
}

