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
    void excuteScript();

private:
    QString path;



};

#endif // IMAGE_PROCESSING_H
