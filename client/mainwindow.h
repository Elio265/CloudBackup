#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>    // 用于打开下载链接
#include <QUrl>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void Init();
    void resizeEvent(QResizeEvent *event);
    void setupTreeColumnStretch();
    void showEvent(QShowEvent *event);
    void updatePixmap();
    void refreshList();

private slots:
    void on_pushButton_clicked();
    // JSON 拉取完成回调
    void onListJsonFinished(QNetworkReply* reply);

    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onUploadFinished();

    void on_flushfilelistbutton_clicked();

private:
    Ui::MainWindow *ui;
    QPixmap pixmap;
    QNetworkAccessManager *m_netManager;
    QString m_serverBaseUrl;
    QNetworkReply        *m_currentReply;
    QString m_uploadFileName;

};
#endif // MAINWINDOW_H
