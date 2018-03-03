#ifndef INTEROP_ODLC_HPP
#define INTEROP_ODLC_HPP

#include <QString>

class InteropOdlc
{

public:

    InteropOdlc();
    ~InteropOdlc();

    void setId(int id);
    void setUser(int user);
    void setType(QString type);
    void setLatitude(double latitude);
    void setLongitude(double longitude);
    void setOrientation(QString orientation);
    void setShape(QString shape);
    void setBackgroundColor(QString backgroundColor);
    void setAlphanumeric(QString alphanumeric);
    void setAlphanumericColor(QString alphanumericColor);
    void setDescription(QString description);
    void setAutonomous(bool autonomous);

    int getId();
    int getUser();
    QString getType();
    double getLatitude();
    double getLongitude();
    QString getOrientation();
    QString getShape();
    QString getBackgroundColor();
    QString getAlphanumeric();
    QString getAlphanumericColor();
    QString getDescription();
    bool getAutonomous();

private:
    int id;
    int user;
    QString type;
    double latitude;
    double longitude;
    QString orientation;
    QString shape;
    QString backgroundColor;
    QString alphanumeric;
    QString alphanumericColor;
    QString description;
    bool autonomous;

};

#endif // INTEROP_ODLC_HPP
