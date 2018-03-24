#include <QString>
#include <QtTest>
#include <QDebug>
const QString IP_ADDRESS = "127.0.0.1";
const int PORT = 4206;
#include "sendcommandmessagetest.h"
#include "modules/uas_message/command_message.hpp"
SendCommandMessageTest::SendCommandMessageTest()
{
}

void SendCommandMessageTest::initTestCase()
{
    dcnc = new DCNC();
    dcnc->startServer(IP_ADDRESS, PORT);
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    socket->connectToHost(IP_ADDRESS, PORT);

    // Make sure socket connects
    QVERIFY(socket->waitForConnected(5000));

    QSignalSpy receivedConnectionSpy(dcnc, SIGNAL(receivedConnection()));
    QVERIFY(receivedConnectionSpy.wait());
    QCOMPARE(receivedConnectionSpy.count(), 1);

}

void SendCommandMessageTest::cleanupTestCase()
{
    disconnect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    disconnect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    disconnect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    delete socket;
    delete dcnc;
}
// dcnc sent command message
void SendCommandMessageTest::sendCommand()
{
    socket->flush();
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    QSignalSpy spy(socket, SIGNAL(readyRead()));
    spy.wait(5000);
    CommandMessage* outgoingMessage = new CommandMessage(CommandMessage::Commands::IMAGE_RELAY_START,CommandMessage::Triggers::TIME,0xFF);
    bool sendStatus = dcnc->sendUASMessage(std::shared_ptr<UASMessage>(outgoingMessage));

    QVERIFY(sendStatus);
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(socket,  SIGNAL(readyRead()), &loop, SLOT(quit()) );
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(50000);
    loop.exec();
}

void SendCommandMessageTest::socketConnected(){
    qInfo() << "Socket Connected";
    connectionDataStream.resetStatus();
    connectionDataStream.setDevice(socket);
}

void SendCommandMessageTest::socketDisconnected(){
    qInfo() << "Socket Disconnected";
    connectionDataStream.resetStatus();
    connectionDataStream.unsetDevice();
}

void SendCommandMessageTest::readyRead()
{
     qDebug() << ("message ready to read");
    messageFramer.clearMessage();
    while (messageFramer.status() != UASMessageTCPFramer::TCPFramerStatus::INCOMPLETE_MESSAGE)
    {
        connectionDataStream.startTransaction();
        connectionDataStream >> messageFramer;
        if (messageFramer.status() == UASMessageTCPFramer::TCPFramerStatus::SUCCESS)
        {
            handleClientMessage(messageFramer.generateMessage());
            qDebug() <<("the message received success");
            connectionDataStream.commitTransaction();
            break;
        }
        else
        {
             qDebug() <<("the message received failed");
            connectionDataStream.abortTransaction();
            break;
        }
    }
}

void SendCommandMessageTest::handleClientMessage(std::shared_ptr<UASMessage> message)
{
    switch(message->type())
    {
        case UASMessage::MessageID::COMMAND:
            {
              std::shared_ptr<CommandMessage> commandMesg = std::static_pointer_cast<CommandMessage>(message);
              qDebug() <<("This is a command message");
              QCOMPARE(commandMesg->command,CommandMessage::Commands::IMAGE_RELAY_START);
              QCOMPARE(commandMesg->trigger,CommandMessage::Triggers::TIME);
              QCOMPARE(commandMesg->parameter,(uint8_t)0xFF);
              qDebug()<<("Command message received succeed");
              QVERIFY(true);
              break;
            }
    case UASMessage::MessageID::DATA_SYSTEM_INFO:
            { qDebug() <<("This is a system info message");
              break;
            }
    default:
        qDebug() <<("this is not a message");
//        QVERIFY(false);
        break;
    }
}
QTEST_MAIN(SendCommandMessageTest)

