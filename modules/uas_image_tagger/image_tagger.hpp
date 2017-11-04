#ifndef IMAGEFETCHER_HPP
#define IMAGEFETCHER_HPP

//===================================================================
// Includes
//===================================================================
// System Includes
#include <QString>
#include <QObject>
#include <QQueue>
#include <assert.h>
// GCOM Includes
#include "modules/uas_dcnc/dcnc.hpp"
#include "modules/uas_message/image_message.hpp"

//===================================================================
// Public Class Declaration
//===================================================================
/*!
 * \brief The ImageFetcher class is designed to tag images and save them to disk
 * \details Current implementation receives signals and saves images to disk
 */
class ImageFetcher : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief ImageFetcher constructor that takes in the directory names for tagged images, untagged images and Tags
     * \param TaggedDir QString path of directory for tagged images
     * \param UntaggedDir QString path of directory for untagged images
     * \param TagsDir QString path of directory for Tags text file
     * \param DCNC pointer to constant data indicating sender of signal
     */
    ImageFetcher(QString Dir, const DCNC *sender);

    /*!
     *  \brief ImageFetcher deconstructor
     */
    ~ImageFetcher();

    /*!
     * \brief Checks to see if the directory exists, if it is writable, and if it is readable
     * \param dir QString path of directory
     */
    bool checkDir(QString dir);

    /*!
     * \brief Checks to see if the new directory exists, if it is writable, and if it is readable
     * \brief updates the working directory for tagged images
     * \param dir QString path of the new tagged images directory
     */
    bool changeDir(QString dir);

    /*!
     * \brief saveImageToDisc helper function that does the saving
     * \param filePath QString path of directory with filename
     * \param data unsigned char pointer to image data
     * \param size size_t the number of bytes to write to the file
     */
    void saveImageToDisc(QString filePath, unsigned char *data, size_t size);
signals:
    // Data Signals
    void taggedImage(QString filePath);
private slots:
    /*!
     * \brief handleImageMessage saves image to disc and sends a signal with
     *        the tagged image's file namer
     */
    void handleImageMessage(std::shared_ptr<ImageMessage> message);
private:
    QString pathOfTagged, pathOfUntagged, pathOfTags;
    bool err;
    int numTagged, numUntagged, numTags;
    uint8_t prevSeqNum;
};

#endif // IMAGEFETCHER_HPP
