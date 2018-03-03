#include "image_processing.hpp"
#include "gcom_controller.hpp"


ImageProcessing::ImageProcessing()
{
    ImageScriptPath = "?";

}
void ImageProcessing::excuteScript(QString scriptPath){
    qDebug() << "Start script";

    QStringList  args = QStringList() << scriptPath;
    int exitCode = QProcess::execute( "python", args );
        qDebug() << exitCode << " End script";
}
