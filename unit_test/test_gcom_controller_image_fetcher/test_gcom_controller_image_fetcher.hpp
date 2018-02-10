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
Q_DECLARE_METATYPE(CommandMessage::Commands)

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
    void testRegexValidPaths_data();
    void testRegexValidPaths();
    void testRegexInvalidPaths_data();
    void testRegexInvalidPaths();
    void testImageTransfer();

public slots:
    // Receive handlers
    void handleClientData();
    void handleClientMessage(std::shared_ptr<UASMessage> message);

    void compareStartImageTransfer(CommandMessage::Commands command);
    void compareStopImageTransfer(CommandMessage::Commands command);

private:
    void checkDCNCInitialStatus();
    void startServer();
    void stopServer();
    void connectSocket();
    void disconnectSocket();
    void connectSocketNoCapabilities();
    void checkFetcherStatus(bool transferButtonEnabled, bool imagesInvalidHidden,
                            bool tagsInvalidHidden, QString invalidLabel);
    void fetcherSetPaths(QString imagesPath, QString tagsPath);
    /*!
     * \brief Sends capabilities to dcnc
     * \param capabilities, the bit field of capabilities
     * \param num, number of capabilities sent
     */
    void sendCapabilities(CapabilitiesMessage::Capabilities capabilities, int num);
    void startImageTransferSuccess();
    void startImageTransferFail(bool imagesInvalidHidden, bool tagsInvalidHidden);
    void stopImageTransfer();

    GcomController* gcom;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;
    QTcpSocket* socket;

};

#endif // TEST_GCOM_CONTROLLER_IMAGE_FETCHER_HPP
