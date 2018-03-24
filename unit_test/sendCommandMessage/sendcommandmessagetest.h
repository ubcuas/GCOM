#ifndef SENDCOMMANDMESSAGETEST_H
#define SENDCOMMANDMESSAGETEST_H
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_message/uas_message_tcp_framer.hpp"
#include "modules/uas_message/uas_message.hpp"
class SendCommandMessageTest : public QObject
{
    Q_OBJECT

public:
    SendCommandMessageTest();

public slots:
    void socketConnected();
    void socketDisconnected();
    void readyRead();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void sendCommand();

private:
    void handleClientMessage(std::shared_ptr<UASMessage> message);
    DCNC *dcnc;
    QTcpSocket *socket;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;
};

#endif // SENDCOMMANDMESSAGETEST_H
