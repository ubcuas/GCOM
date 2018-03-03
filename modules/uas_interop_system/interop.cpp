#include <QDebug>
#include <QString>

#include "interop.hpp"

// Endpoint URL Strings
const QString LOGIN_ENDPOINT = "/api/login";
const QString GET_MISSIONS_ENDPOINT = "/api/missions";
const QString GET_OBSTACLES_ENDPOINT = "/api/obstacles";
const QString POST_TELEMETRY_ENDPOINT = "/api/telemetry";
const QString POST_ODLCS_ENDPOINT = "/api/odlcs";
const QString GET_ODLCS_ENDPOINT = "/api/odlcs";


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
    QString requestParams = "username=" + userName + "&password=" + password;

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

void Interop::getMissions()
{
    this->currRequest = InteropRequest::GET_MISSIONS;

    QString requestUrlString = this->hostUrl + GET_MISSIONS_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getMissions(int missionId)
{
    this->currRequest = InteropRequest::GET_MISSION_WITH_ID;

    QString missionIdPath = "/" + QString::number(missionId);
    QString requestUrlString = this->hostUrl + GET_MISSIONS_ENDPOINT + missionIdPath;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::getObstacles()
{
    this->currRequest = InteropRequest::GET_OBSTACLES;

    QString requestUrlString = this->hostUrl + GET_OBSTACLES_ENDPOINT;

    QUrl requestUrl(requestUrlString);

    QNetworkRequest request(requestUrl);

    connect(this->networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishRequest(QNetworkReply*)));
    this->networkAccessManager->get(request);
}

void Interop::finishRequest(QNetworkReply *reply)
{
    switch(this->currRequest)
    {
        case InteropRequest::NO_REQUEST:
            // probably throw exception here
            break;

        case InteropRequest::LOGIN:
            finishLogin(reply);
            break;

        case InteropRequest::GET_MISSIONS:
            finishGetMissions(reply);
            break;

        case InteropRequest::GET_MISSION_WITH_ID:
            break;

        case InteropRequest::GET_OBSTACLES:
            finishGetObstacles(reply);
            break;

        case InteropRequest::POST_TELEMETRY:
        case InteropRequest::POST_ODLCS:
        case InteropRequest::GET_ODLCS:
        case InteropRequest::GET_ODLCS_WITH_ID:
        case InteropRequest::PUT_ODLCS_WITH_ID:
        case InteropRequest::DELETE_ODLCS_WITH_ID:
        case InteropRequest::GET_ODLCS_IMAGE:
        case InteropRequest::POST_ODLCS_IMAGE:
        case InteropRequest::DELETE_ODLCS_IMAGE:
            break;
    }
}

void Interop::finishLogin(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug() << statusCode.toString();
    QByteArray replyBody = reply->readAll();
    qDebug() << QString(replyBody);
}

void Interop::finishGetObstacles(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode == 200)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        InteropJsonInterpreter::ObstacleSet* obsSet = jsonInterpreter->parseObstacles(jsonDoc);
    }
}

void Interop::finishGetMissions(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode == 200)
    {
        QByteArray replyBody = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(replyBody);
        QList<InteropMission*> missions = jsonInterpreter->parseMultipleMissions(jsonDoc);
    }
}
