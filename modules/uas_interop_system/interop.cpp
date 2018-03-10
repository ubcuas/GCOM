#include <QDebug>
#include <QString>

#include "interop.hpp"

// Endpoint URL Strings
const QString LOGIN_ENDPOINT = "/api/login";
const QString MISSIONS_ENDPOINT = "/api/missions";
const QString OBSTACLES_ENDPOINT = "/api/obstacles";
const QString TELEMETRY_ENDPOINT = "/api/telemetry";
const QString ODLCS_ENDPOINT = "/api/odlcs";
const QString ODLC_IMAGE_ENDPOINT = "/image";


Interop::Interop()
{
    this->networkAccessManager = new QNetworkAccessManager(this);
    this->hostUrl = "http://localhost:8000";
    this->currRequest = InteropRequest::NO_REQUEST;
    this->jsonInterpreter = new InteropJsonInterpreter();
}

Interop::~Interop()
{
    // Do Nothing
}

void Interop::login(const QString url, const QString userName, const QString password)
{
    this->hostUrl = url;
    this->currRequest = InteropRequest::LOGIN;

    QString requestUrlString = this->hostUrl + LOGIN_ENDPOINT;

    QUrl requestUrl(requestUrlString);
    QByteArray postParams;
    postParams.append("username=");
    postParams.append(userName);
    postParams.append("&password=");
    postParams.append(password);

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->post(request, postParams);
}

/***********************************************************************************************
 *                                      GET Requests                                           *
 ***********************************************************************************************/
void Interop::getMissions()
{
    this->currRequest = InteropRequest::GET_MISSIONS;

    QString requestUrlString = this->hostUrl + MISSIONS_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getMission(int missionId)
{
    this->currRequest = InteropRequest::GET_MISSION_WITH_ID;

    QString missionIdPath = "/" + QString::number(missionId);
    QString requestUrlString = this->hostUrl + MISSIONS_ENDPOINT + missionIdPath;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getObstacles()
{
    this->currRequest = InteropRequest::GET_OBSTACLES;

    QString requestUrlString = this->hostUrl + OBSTACLES_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getOdlcs()
{
    this->currRequest = InteropRequest::GET_ODLCS;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getOdlc(int odlcId)
{
    this->currRequest = InteropRequest::GET_ODLCS_WITH_ID;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT + "/" + QString::number(odlcId);

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getOdlcImage(int odlcId)
{
    this->currRequest = InteropRequest::GET_ODLCS_IMAGE;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT + "/" + QString::number(odlcId) + ODLC_IMAGE_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

/***********************************************************************************************
 *                                      POST Requests                                          *
 ***********************************************************************************************/
void Interop::postTelemetry(InteropTelemetry telemetry)
{
    this->currRequest = InteropRequest::POST_TELEMETRY;

    QString requestUrlString = this->hostUrl + TELEMETRY_ENDPOINT;

    QUrl requestUrl(requestUrlString);
    QByteArray postParams;
    postParams.append("latitude=");
    postParams.append(telemetry.getLatitude());
    postParams.append("&longitude=");
    postParams.append(telemetry.getLongitude());
    postParams.append("&altitude_msl=");
    postParams.append(telemetry.getAltitudeMsl());
    postParams.append("&uas_heading=");
    postParams.append(telemetry.getUasHeading());

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->post(request, postParams);
}

void Interop::postOdlc(InteropOdlc odlc)
{
    this->currRequest = InteropRequest::POST_ODLCS;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray jsonBody = jsonInterpreter->encodeOdlc(odlc).toBinaryData();

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->post(request, jsonBody);
}

void Interop::postOdlcImage(int odlcId, QByteArray imageData)
{
    this->currRequest = InteropRequest::POST_ODLCS_IMAGE;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT + "/" + QString::number(odlcId) + ODLC_IMAGE_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->post(request, imageData);
}

/***********************************************************************************************
 *                                      PUT Requests                                           *
 ***********************************************************************************************/
void Interop::putOdlc(int odlcId, InteropOdlc odlc)
{
    this->currRequest = InteropRequest::PUT_ODLCS_WITH_ID;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT + "/" + QString::number(odlcId);

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray jsonBody = jsonInterpreter->encodeOdlc(odlc).toBinaryData();

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->put(request, jsonBody);
}

/***********************************************************************************************
 *                                     DELETE Requests                                         *
 ***********************************************************************************************/
void Interop::deleteOdlc(int odlcId)
{
    this->currRequest = InteropRequest::DELETE_ODLCS_WITH_ID;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT + "/" + QString::number(odlcId);

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->deleteResource(request);
}

void Interop::deleteOdlcImage(int odlcId)
{
    this->currRequest = InteropRequest::DELETE_ODLCS_IMAGE;

    QString requestUrlString = this->hostUrl + ODLCS_ENDPOINT + "/" + QString::number(odlcId) + ODLC_IMAGE_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->deleteResource(request);
}

void Interop::finishRequest(QNetworkReply *reply)
{
    switch(this->currRequest)
    {
        case InteropRequest::NO_REQUEST:
            qDebug() << "Currently There are no Requests to be Handled";
            break;

        case InteropRequest::LOGIN:
            finishLogin(reply);
            break;

        case InteropRequest::GET_MISSIONS:
            finishGetMissions(reply);
            break;

        case InteropRequest::GET_MISSION_WITH_ID:
            finishGetMission(reply);
            break;

        case InteropRequest::GET_OBSTACLES:
            finishGetObstacles(reply);
            break;

        case InteropRequest::POST_TELEMETRY:
            finishPostTelemetry(reply);
            break;

        case InteropRequest::POST_ODLCS:
            finishPostOdlc(reply);
            break;

        case InteropRequest::GET_ODLCS:
            finishGetOdlcs(reply);
            break;

        case InteropRequest::GET_ODLCS_WITH_ID:
            finishGetOdlc(reply);
            break;

        case InteropRequest::PUT_ODLCS_WITH_ID:
            finishPutOdlc(reply);
            break;

        case InteropRequest::DELETE_ODLCS_WITH_ID:
            finishDeleteOdlc(reply);
            break;

        case InteropRequest::GET_ODLCS_IMAGE:
            finishGetOdlcImage(reply);
            break;

        case InteropRequest::POST_ODLCS_IMAGE:
            finishPostOdlcImage(reply);
            break;

        case InteropRequest::DELETE_ODLCS_IMAGE:
            finishDeleteOdlcImage(reply);
            break;
    }
}

void Interop::finishLogin(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus requestStatus = interpretHttpStatus(statusCode);

    emit loginResponse(interpretHttpStatus(statusCode));
    reply->deleteLater();
}

/***********************************************************************************************
 *                               GET Response Handlers                                         *
 ***********************************************************************************************/
void Interop::finishGetMissions(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    QList<InteropMission*> missions;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        missions = jsonInterpreter->parseMultipleMissions(jsonDoc);
    }

    emit getMultiMissionResponse(reqStatus, missions);
    reply->deleteLater();
}

void Interop::finishGetMission(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    InteropMission* mission = nullptr;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        mission = jsonInterpreter->parseSingleMission(jsonDoc);
    }

    emit getSingleMissionResponse(reqStatus, mission);
    reply->deleteLater();
}

void Interop::finishGetObstacles(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    InteropJsonInterpreter::ObstacleSet* obstacleSet = nullptr;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        obstacleSet = jsonInterpreter->parseObstacles(jsonDoc);
    }

    emit getObstaclesResponse(reqStatus, obstacleSet);
    reply->deleteLater();
}

void Interop::finishGetOdlcs(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    QList<InteropOdlc*> odlcs;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        odlcs = jsonInterpreter->parseMultipleOldcs(jsonDoc);
    }

    emit getMultipleOdlcResponse(reqStatus, odlcs);
    reply->deleteLater();
}

void Interop::finishGetOdlc(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    InteropOdlc* odlc;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        odlc = jsonInterpreter->parseSingleOdlc(jsonDoc);
    }

    emit getSingleOdlcResponse(reqStatus, odlc);
    reply->deleteLater();
}

void Interop::finishGetOdlcImage(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    QByteArray imageData;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        imageData = reply->readAll();
    }

    emit getOdlcImageResponse(reqStatus, imageData);
    reply->deleteLater();
}

/***********************************************************************************************
 *                               POST Response Handlers                                        *
 ***********************************************************************************************/
void Interop::finishPostTelemetry(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    emit postTelemetryResponse(interpretHttpStatus(statusCode));
    reply->deleteLater();
}

void Interop::finishPostOdlc(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    InteropOdlc* odlc = nullptr;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        odlc = jsonInterpreter->parseSingleOdlc(jsonDoc);
    }

    emit postOdlcResponse(reqStatus, odlc);
    reply->deleteLater();
}

void Interop::finishPostOdlcImage(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    emit postOdlcImageResponse(interpretHttpStatus(statusCode));
    reply->deleteLater();
}

/***********************************************************************************************
 *                               PUT Response Handlers                                         *
 ***********************************************************************************************/
void Interop::finishPutOdlc(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    RequestStatus reqStatus = interpretHttpStatus(statusCode);
    InteropOdlc* odlc = nullptr;
    if(reqStatus == RequestStatus::SUCCESS)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        odlc = jsonInterpreter->parseSingleOdlc(jsonDoc);
    }

    emit putOldcResponse(reqStatus, odlc);
    reply->deleteLater();
}

/***********************************************************************************************
 *                               DELETE Response Handlers                                      *
 ***********************************************************************************************/
void Interop::finishDeleteOdlc(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    emit deleteOdlcResponse(interpretHttpStatus(statusCode));
    reply->deleteLater();
}

void Interop::finishDeleteOdlcImage(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    emit deleteOdlcImageResponse(interpretHttpStatus(statusCode));
    reply->deleteLater();
}

Interop::RequestStatus Interop::interpretHttpStatus(QVariant status)
{
    if(status >= 100 && status < 200)
    {
        return RequestStatus::INFORMATION;
    }
    else if(status >= 200 && status < 300)
    {
        return RequestStatus::SUCCESS;
    }
    else if(status >= 300 && status < 400)
    {
        return RequestStatus::REDIRECTED;
    }
    else if(status >= 400 && status < 500)
    {
        return RequestStatus::CLIENT_ERROR;
    }
    else if(status >= 500 && status < 600)
    {
        return RequestStatus::INVALID;
    }
}
