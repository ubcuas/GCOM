#ifndef TEST_GCOM_CONTROLLER_IMAGE_FETCHER_HPP
#define TEST_GCOM_CONTROLLER_IMAGE_FETCHER_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
// GCOM Includes
#include "gcom_controller.hpp"
#include "modules/uas_message/uas_message_tcp_framer.hpp"

class TestGcomControllerImageFetcher : public QObject
{
    Q_OBJECT

signals:
    void receivedCommand(CommandMessage::Commands command);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private slots:


public slots:
    // Socket slots
    void socketConnected();
    void socketDisconnected();

    // Receive handlers
    void handleClientData();
    void handleClientMessage(std::shared_ptr<UASMessage> message);

private:
    GcomController gcom;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;
    QTcpSocket* socket;

};

#endif // TEST_GCOM_CONTROLLER_IMAGE_FETCHER_HPP
