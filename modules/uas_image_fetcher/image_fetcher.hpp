#ifndef IMAGEFETCHER_HPP
#define IMAGEFETCHER_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QObject>
#include <QFile>
#include <QQueue>
#include <assert.h>
// GCOM Includes
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_message/image_tagged_message.hpp"

//===================================================================
// Public Class Declaration
//===================================================================
/*!
 * \brief The ImageFetcher class is designed to tag images and save them to disk
 * \details Current implementation receives signals and saves images and tags to disk
 */
class ImageFetcher : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief ImageFetcher constructor that takes in the default directories for
     *        images and tags, then checks and creates them
     * \param dir QString path to where images and tags shoudl be saved
     * \param DCNC pointer to constant data indicating sender of signal
     */
    ImageFetcher(QString dir, const DCNC *sender);

    /*!
     *  \brief ImageFetcher deconstructor
     */
    ~ImageFetcher();

    /*!
     * \brief checks to see if the directory exists, is writable, and is readable
     * \param dir QString path of directory
     */
    bool checkDir(QString dir);
    void updateImageNum(QString dir);

    /*!
     * \brief calls checkDir to check the new directory
     * \brief updates the working directory for images and tags
     * \brief creates a new txt file and QFile pointer to point to it
     * \param dir QString path of to where the files should be saved
     */
    bool changeDir(QString dir);

    /*!
     * \brief saveImageToDisc helper function that does the saving
     * \param filePath QString path of directory with filename
     * \param data unsigned char pointer to image data
     * \param size size_t the number of bytes to write to the file
     */
    void saveToDisc(QString filePath, unsigned char *data, size_t size);

signals:
    /*!
     * \brief TaggedImage is emitted for every image and contains the image directory and tags
     */
    void taggedImage(QString filePath, double latitude, double longitude,
                     float altitude_abs, float altitude_rel, float heading);
    /*!
     * \brief skippedFromSeqNumTo is emitted every time there is a jump in the sequence number
     *        and contains the pervious and new sequence numbers
     */
    void skippedFromSeqNumTo(uint8_t fromSeqNum, uint8_t toSeqNum);

private slots:
    /*!
     * \brief handleImageTaggedMessage saves image and corresponding tags to disc
     *        and sends a signal with the image directory and individual tags and
     *        emits a signal if there is a jump in the sequence number
     */
    void handleImageTaggedMessage(std::shared_ptr<ImageTaggedMessage> message);

protected:
    QFile* tagFile;
    QString fileDirectory;
    int imageNum;
    uint8_t prevSeqNum;
};

#endif // IMAGEFETCHER_HPP
