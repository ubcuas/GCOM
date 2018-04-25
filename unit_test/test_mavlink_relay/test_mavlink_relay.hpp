#ifndef TEST_MAVLINK_RELAY_HPP
#define TEST_MAVLINK_RELAY_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
// GCOM Includes
#include "modules/mavlink_relay/mavlink_relay_tcp.hpp"

Q_DECLARE_METATYPE(mavlink_message_t)

class TestMAVLinkRelay : public QObject
{
    Q_OBJECT


private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testTriggerCamera();
    void testWriteMission();
    void testClearMission();
    void testDisconnection();

signals:
    void receivedTriggerCamera(mavlink_message_t message);
    void receivedClearMission(mavlink_message_t message);
    void receivedMissionItem(mavlink_message_t message);
    void receivedMissionCount(mavlink_message_t message);

public slots:
    void readBytes();

    void compareTriggerCamera(mavlink_message_t message);
    void compareClearMission(mavlink_message_t message);

    void handleMissionCount(mavlink_message_t message);
    void compareMissionItem(mavlink_message_t message);

private:
    bool writeData(mavlink_message_t outgoingMessage);



    QTcpServer server;
    QTcpSocket *socket;
    MAVLinkRelay mavlinkRelay;

    uint16_t waypointCount;
    int waypointNumber;
};

#endif // TEST_MAVLINK_RELAY_HPP
