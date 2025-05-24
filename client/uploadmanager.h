#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class uploadmanager : public QObject
{
    Q_OBJECT
public:
    explicit uploadmanager(QObject *parent = nullptr);
    void uploadFile(const QString &filePath, const QUrl &url);

signals:
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void uploadFinished(bool success, const QString &response);

private slots:
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onReplyFinished();

private:
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QFile file;
};

#endif // UPLOADMANAGER_H
