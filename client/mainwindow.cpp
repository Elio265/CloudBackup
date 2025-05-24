#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "usermanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , userManager(new usermanager)
    , uploader(new uploadmanager)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);

    // 初始化
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(100);

    // 实时更新进度
    connect(uploader, &uploadmanager::uploadProgress, this, [=](qint64 sent, qint64 total) {
        if (total > 0)
        {
            int percent = static_cast<int>((sent * 100) / total);
            ui->progressBar->setValue(percent);
        }
    });
    connect(uploader, &uploadmanager::uploadFinished, this, [=](bool success, const QString &resp){
        QMessageBox::information(this, "上传结果", success ? "上传成功！" : ("失败: " + resp));
        if (success)
        {
            LOG_INFO_MSG("文件上传成功。");
            on_flushlistbutton_clicked();  // 上传成功后刷新文件列表
        }
        else
        {
            LOG_ERROR_MSG(QString("文件上传失败，原因：%1").arg(resp));
        }
    });

    // 设置账号框输入限制：只允许字母、数字、下划线，禁止中文、空格
    QRegularExpression regExp("^[a-zA-Z0-9_]{1,20}$");
    QValidator *validator = new QRegularExpressionValidator(regExp, this);
    ui->username->setValidator(validator);
    ui->username_2->setValidator(validator);

    connect(userManager, &usermanager::loginSuccess, this, [this]() {
        QMessageBox::information(nullptr, "提示", "登录成功！");
        ui->stackedWidget->setCurrentIndex(2);
    });
    connect(userManager, &usermanager::loginFailed, this, [](const QString& msg) {
        QMessageBox::warning(nullptr, "登录失败", msg);
    });

    connect(userManager, &usermanager::registerSuccess, this, [this]() {
        QMessageBox::information(nullptr, "提示", "注册成功！");
        ui->stackedWidget->setCurrentIndex(0);
    });
    connect(userManager, &usermanager::registerFailed, this, [](const QString& msg) {
        QMessageBox::warning(nullptr, "注册失败", msg);
    });

    on_flushlistbutton_clicked();  // 程序启动时自动刷新列表
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_registerbutton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_returnbutton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_loginbutton_clicked()
{
    QString username = ui->username->text().trimmed();
    QString password = ui->password->text().trimmed();

    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "输入错误", "账号和密码不能为空！");
        return;
    }

    // 防止重复创建进度对话框
    if (!loginDialog)
    {
        loginDialog = new QProgressDialog("登录中，请稍候...", QString(), 0, 0, this);
        loginDialog->setWindowFlags(loginDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
        loginDialog->setWindowModality(Qt::ApplicationModal);
        loginDialog->setCancelButton(nullptr);
        loginDialog->setWindowTitle("请稍候");
        loginDialog->setMinimumDuration(0);

        // 只连接一次
        connect(userManager, &usermanager::loginSuccess, this, [=]() {
            if (loginDialog) loginDialog->hide();
        });
        connect(userManager, &usermanager::loginFailed, this, [=](const QString&) {
            if (loginDialog) loginDialog->hide();
        });
    }

    loginDialog->show();

    userManager->login(username, password);
}

void MainWindow::on_registerbutton_2_clicked()
{
    QString username = ui->username_2->text().trimmed();
    QString password = ui->password_2->text().trimmed();
    QString confirmPassword = ui->password_3->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || confirmPassword.isEmpty())
    {
        QMessageBox::warning(this, "输入错误", "用户名和密码不能为空！");
        return;
    }

    if (password != confirmPassword)
    {
        QMessageBox::warning(this, "输入错误", "两次输入的密码不一致！");
        return;
    }

    // 防止重复创建进度对话框
    if (!registerDialog)
    {
        registerDialog = new QProgressDialog("注册中，请稍候...", QString(), 0, 0, this);
        registerDialog->setWindowFlags(registerDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
        registerDialog->setWindowModality(Qt::ApplicationModal);
        registerDialog->setCancelButton(nullptr);
        registerDialog->setWindowTitle("请稍候");
        registerDialog->setMinimumDuration(0);

        // 只连接一次
        connect(userManager, &usermanager::registerSuccess, this, [=]() {
            if (registerDialog) registerDialog->hide();
        });
        connect(userManager, &usermanager::registerFailed, this, [=](const QString&) {
            if (registerDialog) registerDialog->hide();
        });
    }

    registerDialog->show();

    userManager->registerUser(username, password);
}

// 解析服务端JSON数据
QList<BackupItem> MainWindow::parseBackupListJson(const QByteArray &jsonData)
{
    QList<BackupItem> items;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) return items;

    QJsonArray array = doc.array();
    for (const QJsonValue &value : array)
    {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();
        BackupItem item;
        item.filename = obj["filename"].toString();
        item.url = obj["url"].toString();
        item.lastModTime = obj["last_modTime"].toVariant().toLongLong();
        item.fileSize = obj["file_size"].toVariant().toLongLong();
        items.append(item);
    }
    return items;
}

// 更新表格视图
void MainWindow::updateBackupTable(const QList<BackupItem> &items)
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(items.size());

    // 保证第4列（下载按钮）可以自动填充宽度
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    // 行高设置（防止按钮太扁）
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(45);

    QString btnStyle = R"(
    QPushButton#uploadbutton {
        font-family: "Microsoft YaHei", "Arial";
        font-size: 16px;
        font-weight: bold;
        color: white;
        background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                          stop:0 #3399FF, stop:1 #66CCFF);
        border: none;
        border-radius: 10px;
        padding: 6px 14px;
    }
    QPushButton#uploadbutton:hover {
        background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                          stop:0 #66CCFF, stop:1 #99DDFF);
    }
    QPushButton#uploadbutton:pressed {
        background-color: #2D8CDB;
    }
    )";

    for (int i = 0; i < items.size(); ++i)
    {
        const BackupItem &item = items[i];
        QTableWidgetItem *nameItem = new QTableWidgetItem(item.filename);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem *timeItem = new QTableWidgetItem(QDateTime::fromSecsSinceEpoch(item.lastModTime).toString("yyyy-MM-dd hh:mm:ss"));
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(QString::number(item.fileSize / 1024.0, 'f', 2) + " KB");
        sizeItem->setFlags(sizeItem->flags() & ~Qt::ItemIsEditable);

        QPushButton *btn = new QPushButton("下载");
        btn->setStyleSheet(btnStyle);
        btn->setObjectName("uploadbutton");
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(btn, &QPushButton::clicked, this, [=]()
        {
            QDesktopServices::openUrl(QUrl("http://192.168.1.152:8080" + item.url)); // 默认浏览器下载
        });

        ui->tableWidget->setItem(i, 0, nameItem);
        ui->tableWidget->setItem(i, 1, timeItem);
        ui->tableWidget->setItem(i, 2, sizeItem);
        ui->tableWidget->setCellWidget(i, 3, btn);
    }
}

void MainWindow::on_flushlistbutton_clicked()
{
    QNetworkRequest request(QUrl("http://192.168.1.152:8080/listjson"));
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]()
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            QString errorMsg = "获取列表失败：" + reply->errorString();
            QMessageBox::warning(this, "网络错误", errorMsg);
            LOG_ERROR_MSG(errorMsg);  // 添加日志
            reply->deleteLater();
            return;
        }

        QList<BackupItem> list = parseBackupListJson(reply->readAll());
        updateBackupTable(list);

        reply->deleteLater();
    });
}

void MainWindow::on_uploadbutton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择上传文件");
    if (filePath.isEmpty()) return;

    QUrl url("http://192.168.1.152:8080/upload");
    LOG_INFO_MSG(QString("开始上传文件: %1").arg(filePath));
    uploader->uploadFile(filePath, url);
}
