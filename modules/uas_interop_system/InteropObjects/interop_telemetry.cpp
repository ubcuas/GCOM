#include "interop_telemetry.hpp"

InteropTelemetry::InteropTelemetry(const double latitude, const double longitude, const double altitudeMsl, const double uasHeading)
{
    this->latitude = latitude;
    this->longitude = longitude;
    this->altitudeMsl = altitudeMsl;
    this->uasHeading = uasHeading;
}

double InteropTelemetry::getLatitude()
{
    return this->latitude;
}

double InteropTelemetry::getLongitude()
{
    return this->longitude;
}

double InteropTelemetry::getAltitudeMsl()
{
    return this->altitudeMsl;
}

double InteropTelemetry::getUasHeading()
{
    return this->uasHeading;
}
