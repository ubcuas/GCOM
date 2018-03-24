#ifndef INTEROP_HPP
#define INTEROP_HPP

#include <QDebug>
#include <QtNetwork>

#include "interop_json_interpreter.hpp"

class Interop : QObject
{
    Q_OBJECT

public:
    Interop();
    ~Interop();

    void login(const QString url, const QString userName, const QString password);
    void getMissions();
    void getMissions(int missionId);
    void getObstacles();
    void postTelemetry();

protected:

private:
    enum class InteropRequest
    {
        NO_REQUEST,
        LOGIN,
        GET_MISSIONS,
        GET_MISSION_WITH_ID,
        GET_OBSTACLES,
        POST_TELEMETRY,
        POST_ODLCS,
        GET_ODLCS,
        GET_ODLCS_WITH_ID,
        PUT_ODLCS_WITH_ID,
        DELETE_ODLCS_WITH_ID,
        GET_ODLCS_IMAGE,
        POST_ODLCS_IMAGE,
        DELETE_ODLCS_IMAGE
    };

    QNetworkAccessManager *networkAccessManager;
    QString hostUrl;
    InteropRequest currRequest;
    InteropJsonInterpreter* jsonInterpreter;

    void finishLogin(QNetworkReply *reply);
    void finishGetObstacles(QNetworkReply *reply);
    void finishGetMissions(QNetworkReply *reply);

private slots:
    void finishRequest(QNetworkReply* reply);

};

#endif
