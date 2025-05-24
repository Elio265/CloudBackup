#include "uploadmanager.h"
#include <QHttpMultiPart>
#include <QFileInfo>
#include <QDebug>


uploadmanager::uploadmanager(QObject *parent)
    : QObject(parent), reply(nullptr)
{
}

void uploadmanager::uploadFile(const QString &filePath, const QUrl &url)
{
    // 动态分配 QFile，让 multiPart 管理它的生命周期
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit uploadFinished(false, "Failed to open file.");
        delete file;
        return;
    }

    QFileInfo info(filePath);

    // 构造 multipart/form-data
    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"" + info.fileName() + "\""));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);          // multiPart 负责删除 file
    multiPart->append(filePart);

    // 发起 POST 请求
    QNetworkRequest request(url);
    reply = manager.post(request, multiPart);
    multiPart->setParent(reply);         // reply 负责删除 multiPart

    // 连接信号
    connect(reply, &QNetworkReply::uploadProgress, this, &uploadmanager::onUploadProgress);
    connect(reply, &QNetworkReply::finished, this, &uploadmanager::onReplyFinished);
}

void uploadmanager::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    emit uploadProgress(bytesSent, bytesTotal);
}

void uploadmanager::onReplyFinished()
{
    bool success = (reply->error() == QNetworkReply::NoError);
    QString response = reply->readAll();
    reply->deleteLater();
    emit uploadFinished(success, response);
}
