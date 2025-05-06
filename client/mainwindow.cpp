#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QtConcurrent>
#include <QFileDialog>
#include <QHttpMultiPart>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_netManager(new QNetworkAccessManager(this))
    , m_serverBaseUrl(QStringLiteral("http://192.168.48.129:8080"))
    , m_currentReply(nullptr)
{
    ui->setupUi(this);
    Init();

    // 拉取服务端列表
    refreshList();

    // UI 初始状态
    ui->progressBar->setVisible(false);
    ui->resultlabel->clear();
}

// 新增：刷新列表请求封装
void MainWindow::refreshList()
{
    QNetworkRequest listReq(QUrl(m_serverBaseUrl + "/listjson"));
    QNetworkReply *listReply = m_netManager->get(listReq);
    connect(listReply, &QNetworkReply::finished, this, [this, listReply]() {
        onListJsonFinished(listReply);
    });
}

void MainWindow::onListJsonFinished(QNetworkReply* reply)
{
    // 1. 网络错误检查
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, tr("网络错误"),
                             tr("获取备份列表失败：%1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    // 2. 读取并解析 JSON
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        QMessageBox::warning(this, tr("数据错误"), tr("返回的 JSON 格式不正确"));
        return;
    }

    QJsonArray array = doc.array();
    ui->backupfilelist->clear();  // 清除所有旧条目

    // 3. 遍历 JSON 数组
    for (auto v : array) {
        if (!v.isObject()) continue;
        QJsonObject obj = v.toObject();

        QString filename    = obj.value("filename").toString();
        QString relPath = obj.value("url").toString(); // e.g. "/download/中文.png"
        qint64  t_sec       = obj.value("last_modTime").toVariant().toLongLong();
        qint64  size_bytes  = obj.value("file_size").toVariant().toLongLong();

        // 格式化
        QString timeStr = QDateTime::fromSecsSinceEpoch(t_sec)
                          .toString("yyyy-MM-dd hh:mm:ss");
        QString sizeStr = QString::number(size_bytes / 1024.0, 'f', 2) + " KB";

        // 4 列：文件名、备份时间、文件大小、下载 按钮
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->backupfilelist);
        item->setText(0, filename);
        item->setText(1, timeStr);
        item->setText(2, sizeStr);

        // 把下载 URL 暂存到 UserRole，以备按钮槽使用
        // 构造一个 QUrl，并让它对 path 做 percent-encode
        QUrl url(m_serverBaseUrl);
        url.setPath(relPath);
        QString fullUrl = url.toString(QUrl::FullyEncoded);
        item->setData(3, Qt::UserRole, fullUrl);

        // 创建下载按钮
        QPushButton *btn = new QPushButton(tr("下载"), ui->backupfilelist);
        btn->setCursor(Qt::PointingHandCursor);
        // 点击时用浏览器打开 URL
        connect(btn, &QPushButton::clicked, [this, item]() {
            QString link = item->data(3, Qt::UserRole).toString();
            QDesktopServices::openUrl(QUrl(link));
        });

        // 把按钮放到第 3 列
        ui->backupfilelist->setItemWidget(item, 3, btn);
    }

    //  根据当前窗口宽度自适应列宽
    setupTreeColumnStretch();
}


void MainWindow::Init()
{
    // 设置窗口icon
    this->setWindowIcon(QIcon(":/images/icon.svg"));

    // 加载图片
    pixmap = QPixmap(":/images/title2.png");
    ui->imagelabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->titlelabel->setText("云备份系统");
    ui->backuptimelabel1->setText("上次备份时间：");

    // ------------------ 新增：加载最新日志显示 ------------------
    QString logsDirPath = QCoreApplication::applicationDirPath() + "/logs";
    QDir logsDir(logsDirPath);
    if (!logsDir.exists()) {
        ui->backuptimelabel2->setText("暂无备份记录");
        return;
    }

    // 获取 logs/ 目录下所有 logN.txt 文件，按编号排序
    QStringList logFiles = logsDir.entryList(QStringList() << "log*.txt", QDir::Files, QDir::Name);
    if (logFiles.isEmpty()) {
        ui->backuptimelabel2->setText("暂无备份记录");
        return;
    }

    // 选择编号最大的那个日志文件（最新）
    QString lastLogFile = logFiles.last();
    QFile file(logsDirPath + "/" + lastLogFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->backuptimelabel2->setText("日志读取失败");
        return;
    }

    // 读取最后一行
    QString lastLine;
    while (!file.atEnd()) {
        QString line = file.readLine().trimmed();
        if (!line.isEmpty())
            lastLine = line;
    }
    file.close();

    // 显示到标签
    if (!lastLine.isEmpty()) {
        ui->backuptimelabel2->setText(lastLine);
    } else {
        ui->backuptimelabel2->setText("暂无备份记录");
    }
}

void MainWindow::updatePixmap()
{
    if (!pixmap.isNull())
    {
        // 根据调整后的 imagelabel 大小进行缩放
        QPixmap scaled = pixmap.scaled(
            ui->imagelabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        ui->imagelabel->setPixmap(scaled);
    }
}

void MainWindow::setupTreeColumnStretch()
{
    QHeaderView *header = ui->backupfilelist->header();
    header->setSectionResizeMode(QHeaderView::Interactive); // 允许手动设置列宽

    int total = ui->backupfilelist->viewport()->width();
    if (total <= 0) return;

    // 比例分配：2/10, 3/10, 3/10, 2/10
    header->resizeSection(0, total * 2 / 10);
    header->resizeSection(1, total * 3 / 10);
    header->resizeSection(2, total * 3 / 10);
    header->resizeSection(3, total * 2 / 10);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    setupTreeColumnStretch();
    updatePixmap();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    updatePixmap();
    setupTreeColumnStretch();
    QMainWindow::resizeEvent(event);
}

MainWindow::~MainWindow()
{
    delete ui;
}


// 按钮点击：先在界面上记录时间，然后异步写文件
void MainWindow::on_pushButton_clicked()
{
    // 1. 选择文件
    QString srcPath = QFileDialog::getOpenFileName(
        this,
        tr("选择要备份的文件"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        tr("All Files (*.*)"));
    if (srcPath.isEmpty())
        return;

    // 文件信息与名称
    QFileInfo fileInfo(srcPath);
    m_uploadFileName = fileInfo.fileName();

    // 创建 multipart 容器
    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
    QHttpPart part;

    // 设置文件上传部分
    QFile *uploadFile = new QFile(srcPath, multi);  // 交由 multi 管理生命周期
    if (!uploadFile->open(QIODevice::ReadOnly)) {
        ui->resultlabel->setText(tr("无法打开文件：%1").arg(srcPath));
        delete uploadFile;
        return;
    }

    // 使用 RFC 5987 编码方式（支持非 ASCII 文件名）
    QByteArray fnEnc = QUrl::toPercentEncoding(m_uploadFileName, QByteArray(), QByteArray());
    QString cdHeader = QStringLiteral("form-data; name=\"file\"; filename*=UTF-8''%1")
                       .arg(QString::fromUtf8(fnEnc));
    part.setHeader(QNetworkRequest::ContentDispositionHeader, cdHeader);
    part.setBodyDevice(uploadFile);
    multi->append(part);

    // 重置 UI
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->resultlabel->setText(tr("正在上传 %1 ...").arg(uploadFile->fileName()));
    ui->pushButton->setEnabled(false);


    // 发起上传
    QNetworkRequest request(QUrl(m_serverBaseUrl + "/upload"));
    m_currentReply = m_netManager->post(request, multi);

    connect(m_currentReply, &QNetworkReply::uploadProgress,
            this, &MainWindow::onUploadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &MainWindow::onUploadFinished);
}

void MainWindow::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percent = int(double(bytesSent) / bytesTotal * 100);
        ui->progressBar->setValue(percent);
    }
}

void MainWindow::onUploadFinished()
{
    ui->pushButton->setEnabled(true);
    ui->progressBar->setVisible(false);
    ui->progressBar->setValue(0);

    if (!m_currentReply) return;

    bool success = (m_currentReply->error() == QNetworkReply::NoError);
    QString errorStr = m_currentReply->errorString();
    QString fileName = m_uploadFileName;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 【改动】只有当上传成功时才写日志并刷新列表
    if (success) {
        // 异步写日志
        QFuture<bool> future = QtConcurrent::run([currentTime, fileName]() {
            QString logsDirPath = QCoreApplication::applicationDirPath() + "/logs";
            QDir logsDir(logsDirPath);
            if (!logsDir.exists() && !logsDir.mkpath(".")) return false;
            const qint64 maxSize = 1024 * 1024;
            QString logFilePath;
            int index = 1;
            while (true) {
                QString candidate = logsDirPath + QString("/log%1.txt").arg(index);
                QFile f(candidate);
                if (!f.exists() || f.size() < maxSize) {
                    logFilePath = candidate;
                    break;
                }
                ++index;
            }
            QFile logFile(logFilePath);
            if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
                return false;
            QTextStream out(&logFile);
            out.setCodec("UTF-8");  // 设置编码
            out << "[" << currentTime << "] " << fileName << "\n";
            logFile.close();
            return true;
        });

        QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
            bool logOk = watcher->result();
            // 更新最后备份时间
            ui->backuptimelabel2->setText(QString("[%1] %2").arg(currentTime, fileName));
            // 刷新列表
            refreshList();
            // 【改动】仅在上传成功时显示日志写入状态
            if (logOk) {
                ui->resultlabel->setText(tr("上传成功：%1").arg(fileName));
            } else {
                ui->resultlabel->setText(tr("上传成功，但日志写入失败"));
            }
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    } else {
        // 上传失败，不写日志
        ui->resultlabel->setText(tr("上传失败：%1").arg(errorStr));  // 【改动】失败时提示
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void MainWindow::on_flushfilelistbutton_clicked()
{
    // 调用 refreshList 方法重新获取并展示备份列表
    refreshList();
    ui->resultlabel->setText(tr("已刷新备份列表"));
}
