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
    void testClearMission();
    void testTriggerCamera();

signals:
    void receivedTriggerCamera(mavlink_message_t message);
    void receivedClearMission(mavlink_message_t message);


public slots:
    void readBytes();

    void compareTriggerCamera(mavlink_message_t message);
    void compareClearMission(mavlink_message_t message);


private:
    bool writeData(mavlink_message_t outgoingMessage);

    QTcpServer server;
    QTcpSocket *socket;
    MAVLinkRelay mavlinkRelay;
};

#endif // TEST_MAVLINK_RELAY_HPP
