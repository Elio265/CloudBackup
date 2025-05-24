#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QJsonDocument>
#include <QRegularExpressionValidator>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QDateTime>
#include <QFileDialog>

#include "usermanager.h"
#include "uploadmanager.h"
#include "logger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct BackupItem
{
    QString filename;
    QString url;
    qint64 lastModTime;
    qint64 fileSize;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    QList<BackupItem> parseBackupListJson(const QByteArray &jsonData);
    void updateBackupTable(const QList<BackupItem> &items);
    ~MainWindow();

private slots:
    void on_registerbutton_clicked();

    void on_returnbutton_clicked();

    void on_loginbutton_clicked();

    void on_registerbutton_2_clicked();

    void on_flushlistbutton_clicked();

    void on_uploadbutton_clicked();

private:
    Ui::MainWindow *ui;
    usermanager* userManager;
    uploadmanager *uploader;
    QProgressDialog* loginDialog = nullptr;
    QProgressDialog* registerDialog = nullptr;
};

#endif // MAINWINDOW_H
