#ifndef IMAGETAGGER_HPP
#define IMAGETAGGER_HPP

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
 * \brief The ImageTagger class is designed to tag images and save them to disk
 * \details Current implementation receives signals and saves images to disk
 */
class ImageTagger : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief ImageTagger constructor that takes in the directory names for tagged images, untagged images and Tags
     * \param TaggedDir QString path of directory for tagged images
     * \param UntaggedDir QString path of directory for untagged images
     * \param TagsDir QString path of directory for Tags text file
     * \param DCNC pointer to constant data indicating sender of signal
     */
    ImageTagger(QString taggedDir, QString untaggedDir, QString TagsDir, const DCNC *sender);

    /*!
     *  \brief ImageTagger deconstructor
     */
    ~ImageTagger();

    /*!
     * \brief setupTaggedDir helper function to setup path name for tagged images
     * \param dir QString path of directory for tagged images
     */
    bool setupTaggedDir(QString dir);
    /*!
     * \brief setupTaggedDir helper function to setup path name for tagged images
     * \param dir QString path of directory for tagged images
     */
    bool setupUntaggedDir(QString dir);
    /*!
     * \brief setupTaggedDir helper function to setup path name for tagged images
     * \param dir QString path of directory for tagged images
     */
    bool setupTagsDir(QString dir);

    bool checkDir(QString dir);

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
     *        the tagged image's file name
     */
    void handleImageMessage(std::shared_ptr<ImageMessage> message);
private:
    QString pathOfTagged;
    QString pathOfUntagged;
    QString pathOfTags;
    int numOfImages;
    int numOfDuplicates;
    int gpsDataAvailable;
    std::vector<unsigned char> seqNumArr;
};

#endif // IMAGETAGGER_HPP
