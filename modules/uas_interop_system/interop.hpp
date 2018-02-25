#ifndef INTEROP_HPP
#define INTEROP_HPP

#include <QDebug>
#include <QtNetwork>

#include "interop_json_interpreter.hpp"

class Interop : public QObject
{
    Q_OBJECT

public:
    Interop();
    ~Interop();

    enum class RequestStatus
    {
        INFORMATION,
        SUCCESS,
        REDIRECTED,
        CLIENT_ERROR,
        SERVER_ERROR,
        INVALID
    };

    void login(const QString url, const QString userName, const QString password);
    void getMissions();
    void getMissions(int missionId);
    void getObstacles();
    void postTelemetry();

signals:
    void loginResponse(Interop::RequestStatus status);
    void getMissionResponse(Interop::RequestStatus status, QList<InteropMission*> missions);

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

    RequestStatus interpretHttpStatus(QVariant status);

private slots:
    void finishRequest(QNetworkReply* reply);

};

#endif
