#include "interop_mission.hpp"

InteropMission::InteropMission() {
    //do nothing
}

InteropMission::~InteropMission() {
    //do nothing
}

void InteropMission::setId(int id)
{
    this->id = id;
}

int InteropMission::getId()
{
    return this->id;
}

void InteropMission::setActive(bool active)
{
    this->active = active;
}

bool InteropMission::getActive()
{
    return this->active;
}

void InteropMission::setAirDropPos(Position airDropPos)
{
    this->airDropPos = airDropPos;
}

InteropMission::Position InteropMission::getAirDropPos()
{
    return this->airDropPos;
}

void InteropMission::setFlyZones(QList<InteropMission::FlyZone> flyZones)
{
    this->flyZones = flyZones;
}

QList<InteropMission::FlyZone> InteropMission::getFlyZones()
{
    return this->flyZones;
}

void InteropMission::setHomePosition(Position homePos)
{
    this->homePos = homePos;
}

InteropMission::Position InteropMission::getHomePosition()
{
    return this->homePos;
}

void InteropMission::setMissionWaypoints(QList<Waypoint> missionWaypoints)
{
    this->missionWaypoints = missionWaypoints;
}

QList<InteropMission::Waypoint> InteropMission::getMissionWaypoints()
{
    return this->missionWaypoints;
}

void InteropMission::setOffAxisOdlcPos(InteropMission::Position offAxisOdlcPos)
{
    this->offAxisOdlcPos = offAxisOdlcPos;
}

InteropMission::Position InteropMission::getOffAxisOdlcPos()
{
    return this->offAxisOdlcPos;
}

void InteropMission::setEmergentLastKnownPos(InteropMission::Position emergentLastKnownPos)
{
    this->emergentLastKnownPos = emergentLastKnownPos;
}

InteropMission::Position InteropMission::getEmergentLastKnownPos()
{
    return this->emergentLastKnownPos;
}

void InteropMission::setSearchGridPoints(QList<Waypoint> searchGridPoints)
{
    this->searchGridPoints = searchGridPoints;
}

QList<InteropMission::Waypoint> InteropMission::getSearchGridPoints()
{
    return this->searchGridPoints;
}
