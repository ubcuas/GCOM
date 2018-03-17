#include "image_processing.hpp"
#include "gcom_controller.hpp"
#include "modules/uas_utility/uas_utility.hpp"


ImageProcessing::ImageProcessing(QString imageScriptLocation, QString outputLocation)
{
    if(!changeImageScriptDir(imageScriptLocation))
        throw std::invalid_argument("Invalid directory");
    if(!changeOutputDir(outputLocation))
        throw std::invalid_argument("Invalid directory");

}
void ImageProcessing::excuteScript(QString scriptPath){
    qDebug() << "Start script";

    QStringList  args = QStringList() << scriptPath;
    int exitCode = QProcess::execute( "python", args );
        qDebug() << exitCode << " End script";
}

// Checks image script directory and updates it if its valid
inline bool ImageProcessing::changeImageScriptDir(QString dir)
{
    if(!Utility::checkDir(dir))
        return false;
    imageScriptPath = dir;
}

// Checks image script directory and updates it if its valid
inline bool ImageProcessing::changeOutputDir(QString dir)
{
    if(!Utility::checkDir(dir))
        return false;
    outputPath = dir;
}
