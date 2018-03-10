#ifndef TEST_COLLISION_AVOIDANCE_HPP
#define TEST_COLLISION_AVOIDANCE_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QtTest>

// GCOM Includes
#include "test_collision_avoidance.hpp"
#include "modules/uas_collision_avoidance/collision_avoidance.hpp"

class TestCollisionAvoidance : public QObject
{
    Q_OBJECT

private:
    CollisionAvoidance *collisionAvoidanceInstance;

private slots:
    void testCase1();
    QList<InteropMission::Waypoint> getMockWaypointsOrdered();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
};

#endif // TEST_COLLISION_AVOIDANCE_HPP
