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

    //GET Requests
    void getMissions();
    void getMission(int missionId);
    void getObstacles();
    void getOdlcs();
    void getOdlc(int odlcId);
    void getOdlcImage(int odlcId);

    //POST Requests
    void postTelemetry(InteropTelemetry *telemetry);
    void postOdlc(InteropOdlc *odlc);
    void postOdlcImage(int odlcId, QByteArray imageData);

    //PUT Requests
    void putOdlc(int odlcId, InteropOdlc *odlc);

    //DELETE Requests
    void deleteOdlc(int odlcId);
    void deleteOdlcImage(int odlcId);

signals:
    void loginResponse(Interop::RequestStatus status);

    //GET Responses
    void getMultiMissionResponse(Interop::RequestStatus status, QList<InteropMission*> missions);
    void getSingleMissionResponse(Interop::RequestStatus status, InteropMission* mission);
    void getObstaclesResponse(Interop::RequestStatus status, InteropJsonInterpreter::ObstacleSet* obstacleSet);
    void getMultipleOdlcResponse(Interop::RequestStatus status, QList<InteropOdlc*> odlcs);
    void getSingleOdlcResponse(Interop::RequestStatus status, InteropOdlc *odlc);
    void getOdlcImageResponse(Interop::RequestStatus status, QByteArray imageData);

    //POST Responses
    void postTelemetryResponse(Interop::RequestStatus status);
    void postOdlcResponse(Interop::RequestStatus status, InteropOdlc *odlc);
    void postOdlcImageResponse(Interop::RequestStatus status);

    //PUT Responses
    void putOldcResponse(Interop::RequestStatus status, InteropOdlc *odlc);

    //DELETE Responses
    void deleteOdlcResponse(Interop::RequestStatus status);
    void deleteOdlcImageResponse(Interop::RequestStatus status);

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

    //GET Response Handlers
    void finishGetMissions(QNetworkReply *reply);
    void finishGetMission(QNetworkReply *reply);
    void finishGetObstacles(QNetworkReply *reply);
    void finishGetMultiOdlcs(QNetworkReply *reply);
    void finishGetSingleOdlc(QNetworkReply *reply);
    void finishGetOdlcImage(QNetworkReply *reply);

    //POST Response Handlers
    void finishPostTelemetry(QNetworkReply *reply);
    void finishPostOdlc(QNetworkReply *reply);
    void finishPostOdlcImage(QNetworkReply *reply);

    //PUT Response Handlers
    void finishPutOdlc(QNetworkReply *reply);

    //DELETE Response Handlers
    void finishDeleteOdlc(QNetworkReply *reply);
    void finishDeleteOdlcImage(QNetworkReply *reply);

    RequestStatus interpretHttpStatus(QVariant status);

private slots:
    void finishRequest(QNetworkReply* reply);

};

#endif
