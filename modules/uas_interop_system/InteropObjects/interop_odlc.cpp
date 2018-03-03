#include "interop_odlc.hpp"

InteropOdlc::InteropOdlc()
{
    //do nothing
}

InteropOdlc::~InteropOdlc()
{
    //do nothing
}

void InteropOdlc::setId(int id)
{
    this->id = id;
}

int InteropOdlc::getId()
{
    return this->id;
}

void InteropOdlc::setUser(int user)
{
    this->user = user;
}

int InteropOdlc::getUser()
{
    return this->user;
}

void InteropOdlc::setType(QString type)
{
    this->type = type;
}

QString InteropOdlc::getType()
{
    return this->type;
}

void InteropOdlc::setLatitude(double latitude)
{
    this->latitude = latitude;
}

double InteropOdlc::getLatitude()
{
    return this->latitude;
}

void InteropOdlc::setLongitude(double longitude)
{
    this->longitude = longitude;
}

double InteropOdlc::getLongitude()
{
    return this->longitude;
}

void InteropOdlc::setOrientation(QString orientation)
{
    this->orientation = orientation;
}

QString InteropOdlc::getOrientation()
{
    return this->orientation;
}

void InteropOdlc::setShape(QString shape)
{
    this->shape = shape;
}

QString InteropOdlc::getShape()
{
    return this->shape;
}

void InteropOdlc::setBackgroundColor(QString backgroundColor)
{
    this->backgroundColor = backgroundColor;
}

QString InteropOdlc::getBackgroundColor()
{
    return this->backgroundColor;
}

void InteropOdlc::setAlphanumeric(QString alphanumeric)
{
    this->alphanumeric = alphanumeric;
}

QString InteropOdlc::getAlphanumeric()
{
    return this->alphanumeric;
}

void InteropOdlc::setAlphanumericColor(QString alphanumericColor)
{
    this->alphanumericColor = alphanumericColor;
}

QString InteropOdlc::getAlphanumericColor()
{
    return this->alphanumericColor;
}

void InteropOdlc::setDescription(QString description)
{
    this->description = description;
}

QString InteropOdlc::getDescription()
{
    return this->description;
}

void InteropOdlc::setAutonomous(bool autonomous)
{
    this->autonomous = autonomous;
}

bool InteropOdlc::getAutonomous()
{
    return this->autonomous;
}
