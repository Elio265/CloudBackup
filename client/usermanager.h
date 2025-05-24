#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class usermanager : public QObject
{
    Q_OBJECT
public:   
    explicit usermanager(QObject* parent = nullptr);

    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);

signals:
    void loginSuccess();
    void loginFailed(const QString& reason);
    void registerSuccess();
    void registerFailed(const QString& reason);

private slots:
    void onLoginReply(QNetworkReply* reply);
    void onRegisterReply(QNetworkReply* reply);

private:
    QNetworkAccessManager* networkManager;
    QString m_serverBaseUrl;
};

#endif // USERMANAGER_H
