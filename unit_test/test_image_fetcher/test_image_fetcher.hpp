#ifndef TEST_IMAGE_FETCHER_HPP
#define TEST_IMAGE_FETCHER_HPP
//===================================================================
// Includes
//===================================================================
// System Includes
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
// GCOM Includes
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_message/uas_message_tcp_framer.hpp"
#include "modules/uas_image_fetcher/image_fetcher.hpp"

Q_DECLARE_METATYPE(std::shared_ptr<ImageTaggedMessage>)
Q_DECLARE_METATYPE(uint8_t)

class TestImageFetcher : public QObject
{
    Q_OBJECT
public slots:
    /*!
     * \brief compares values in taggedImage signal to expected values
     * \param filePath QString path of directory with filename
     * \param latitude of type double
     * \param longitude of type double
     * \param absolute altitude of type float
     * \param relative altitude of type float
     * \param heading of type float
     */
    void compareHandleImageTaggedMessage(QString filePath, double latitude, double longitude,
                                         float altitude_abs, float altitude_rel, float heading);
    /*!
     * \brief compares values in skippedFromSeqNumTo signal to expected values
     * \param previous sequence number
     * \param new sequence number

     */
    void compareSkippedFromSeqNumTo(uint8_t fromSeqNum, uint8_t toSeqNum);

private Q_SLOTS:
    /*!
     * \brief initializes and declares DCNC and ImageFetcher pointers, starts DCNC server, and sets up socket
     */
    void initTestCase();
    /*!
     * \brief deletes all pointers, stops the DCNC server and disconnects the socket
     */
    void cleanupTestCase();

private slots:
    /*!
     * \brief tests constructor for exceptions given invalid filepaths
     */
    void testConstructor();
    /*!
     * \brief sends ImageTaggedMessage to ImageFetcher and ensures that it was sent
     */
    void testHandleImageTaggedMessage();
    /*!
     * \brief creates a data table with the test values for handleImageTaggedMessage
     */
    void testHandleImageTaggedMessage_data();

private:
    // Connection Variables
    DCNC *dcnc;
    ImageFetcher *imageFetcher;
    QTcpSocket *socket;
    QDataStream connectionDataStream;
    UASMessageTCPFramer messageFramer;

    // Test variables
    uint8_t *expectedImageData;
    size_t expectedImageSize;
    uint8_t expectedSequenceNumber;
    uint8_t preSeqNum;
    double expectedLatitude;
    double expectedLongitude;
    float expectedAltitudeAbs;
    float expectedAltitudeRel;
    float expectedHeading;
    QString expectedFilePath;
};

#endif // TEST_IMAGE_FETCHER_HPP
