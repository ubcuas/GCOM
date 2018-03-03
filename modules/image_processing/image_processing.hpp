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
    ImageProcessing();
    void excuteScript(QString scriptPath);

private:
    QString ImageScriptPath;
    QString OutputPath;
    float rejectionRange;



};

#endif // IMAGE_PROCESSING_H
