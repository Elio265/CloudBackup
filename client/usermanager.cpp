#include "usermanager.h"

#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

usermanager::usermanager(QObject* parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , m_serverBaseUrl(QStringLiteral("http://192.168.1.152:8080"))
{}


void usermanager::login(const QString& username, const QString& password)
{
    QUrl url(m_serverBaseUrl + "/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);

    QNetworkReply* reply = networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [=]() { onLoginReply(reply); });
}

void usermanager::registerUser(const QString& username, const QString& password)
{
    QUrl url(m_serverBaseUrl + "/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);

    QNetworkReply* reply = networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [=]() { onRegisterReply(reply); });
}

void usermanager::onLoginReply(QNetworkReply* reply)
{
    QByteArray response = reply->readAll();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();

    if (obj["result"] == "success") {
        emit loginSuccess();
    }
    else
    {
        emit loginFailed(obj["message"].toString());
    }
}

void usermanager::onRegisterReply(QNetworkReply* reply)
{
    QByteArray response = reply->readAll();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit registerFailed(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object();

    if (obj["result"] == "success") {
        emit registerSuccess();
    }
    else
    {
        emit registerFailed(obj["message"].toString());
    }
}
