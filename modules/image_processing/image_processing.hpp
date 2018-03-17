#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <iostream>
#include <QString>
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
class ImageProcessing
{
public:
    ImageProcessing(QString imageScriptLocation, QString outputLocation);
    void excuteScript(QString scriptPath);

    bool changeImageScriptDir(QString dir);
    bool changeOutputDir(QString dir);

private:
    QString imageScriptPath;
    QString outputPath;
    float rejectionRange;



};

#endif // IMAGE_PROCESSING_H
