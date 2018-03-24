#ifndef INTEROP_MISSION_HPP
#define INTEROP_MISSION_HPP

#include <QList>

class InteropMission
{

public:

    InteropMission();
    ~InteropMission();

    struct Position
    {
        double latitude;
        double longitude;
    };

    struct Waypoint
    {
        double altitudeMsl;
        double latitude;
        double longitude;
        int order;
    };

    struct BoundaryPoint
    {
        double latitude;
        double longitude;
        int order;
    };

    struct FlyZone
    {
        double altitudeMslMax;
        double altitudeMslMin;
        QList<BoundaryPoint> bondaryPtList;
    };

    void setId(int id);
    void setActive(bool active);
    void setAirDropPos(Position airDropPos);
    void setFlyZones(QList<FlyZone> flyZones);
    void setHomePosition(Position homePos);
    void setMissionWaypoints(QList<Waypoint> missionWaypoints);
    void setOffAxisOdlcPos(Position offAxisOdlcPos);
    void setEmergentLastKnownPos(Position emergentLastKnownPos);
    void setSearchGridPoints(QList<Waypoint> searchGridPoints);

    int getId();
    bool getActive();
    Position getAirDropPos();
    QList<FlyZone> getFlyZones();
    Position getHomePosition();
    QList<Waypoint> getMissionWaypoints();
    Position getOffAxisOdlcPos();
    Position getEmergentLastKnownPos();
    QList<Waypoint> getSearchGridPoints();

private:
    int id;
    bool active;
    Position airDropPos;
    QList<FlyZone> flyZones;
    Position homePos;
    QList<Waypoint> missionWaypoints;
    Position offAxisOdlcPos;
    Position emergentLastKnownPos;
    QList<Waypoint> searchGridPoints;

};

#endif // INTEROP_MISSION_HPP
