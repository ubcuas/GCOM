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

Q_DECLARE_METATYPE(CapabilitiesMessage::Capabilities)

class TestGcomControllerImageFetcher : public QObject
{
    Q_OBJECT

signals:
    void receivedCommand(CommandMessage::Commands command);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testConnection();

public slots:

    // Receive handlers
    void handleClientData();
    void handleClientMessage(std::shared_ptr<UASMessage> message);

private:
    void checkInitialStatus();
    void startServer();
    void stopServer();
    void connectSocket();
    void disconnectSocket();
    /*!
     * \brief Sends capabilities to dcnc
     * \param capabilities, the bit field of capabilities
     * \param num, number of capabilities sent
     */
    void sendCapabilities(CapabilitiesMessage::Capabilities capabilities, int num);

    GcomController gcom;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;
    QTcpSocket* socket;

};

#endif // TEST_GCOM_CONTROLLER_IMAGE_FETCHER_HPP
