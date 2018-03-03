#include "image_processing.hpp"

ImageProcessing::ImageProcessing()
{
    path = "?";

}
void ImageProcessing::excuteScript(){
    qDebug() << "Start script";

    QStringList  args = QStringList() << "C:/Users/yanyi/Documents/GitHub/realGCOM/GCOM/modules/image_processing/image_processing_main.py";
    int exitCode = QProcess::execute( "python", args );
        qDebug() << exitCode << " ENd script";
}
