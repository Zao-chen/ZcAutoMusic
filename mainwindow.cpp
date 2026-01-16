#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QSettings>
#include <QDir>
#include <Qregularexpression>
#include <QTimer>
#include <QRandomGenerator>
#include <QAudioOutput>

#include "taskchild.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //窗口名称设置
    this->setWindowTitle("ZcAutoMusic - 自动音乐播放器 V1.0");
    /*读取配置文件*/
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("TaskList");
    QStringList taskNames = settings.childGroups();
    int index = 0;
    for (const QString &taskName : taskNames)
    {
        settings.beginGroup(taskName);
        //读取文件
        QTime beginTime = QTime::fromString(settings.value("BeginTime").toString(),"HH:mm:ss");
        QTime endTime = QTime::fromString(settings.value("EndTime").toString(),"HH:mm:ss");
        QString musicFolder = settings.value("MusicFolder").toString();
        settings.endGroup();
        //创建窗口
        TaskChild *taskChild = new TaskChild(
            this,
            taskName,
            beginTime,
            endTime,
            musicFolder
            );
        //加入布局
        ui->verticalLayout_TaskList->addWidget(taskChild);
        index++;
    }
    settings.endGroup();
    /*初始化开机自启动*/
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    ui->checkBox->setChecked(reg.contains("ZcAutoMusic"));
    /*初始化播放器和定时器*/
    player = new QMediaPlayer(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::checkTasks);
    timer->start(1000);  // 每秒检查一次

    if (!player->audioOutput()) {
        qDebug()<<"设置!";
        QAudioOutput *audioOutput = new QAudioOutput(this);
        player->setAudioOutput(audioOutput);
        audioOutput->setVolume(1.0); // 设置音量 0.0 - 1.0
    }
    //播放音乐
    player->setLoops(QMediaPlayer::Infinite);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus st){
        if (st == QMediaPlayer::EndOfMedia) {
            playNextRandomIfNeeded(); // 播完就尝试随机下一首
        }
    });
    //播放进度
    connect(player, &QMediaPlayer::durationChanged, this, [this](qint64 dur){
        // 例如：进度条范围设为总时长
        ui->progressBar->setRange(0, (dur > 0 ? (int)dur : 0));
    });

    connect(player, &QMediaPlayer::positionChanged, this, [this](qint64 pos){
        // 例如：进度条跟随当前位置
        ui->progressBar->setValue((int)pos);

        qint64 dur = player->duration();
        int percent = (dur > 0) ? int(pos * 100 / dur) : 0;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_AddTask_clicked()
{
    //获取最大Task
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("TaskList");
    QStringList groups = settings.childGroups();
    int maxIndex = 0;
    QRegularExpression re("^Task(\\d+)$");
    for (const QString &name : groups)
    {
        QRegularExpressionMatch match = re.match(name);
        if (match.hasMatch())
        {
            int num = match.captured(1).toInt();
            maxIndex = qMax(maxIndex, num);
        }
    }
    settings.endGroup();
    //创建窗口
    TaskChild *taskChild = new TaskChild(
        this,
        "Task"+QString::number(maxIndex + 1),
        QTime(),
        QTime(),
        ""
        );
    //加入布局
    ui->verticalLayout_TaskList->addWidget(taskChild);
}
/*计时器*/
void MainWindow::checkTasks()
{
    QTime now = QTime::currentTime();
    //先找出当前应该播放的任务
    TaskChild* shouldTask = nullptr;
    for (int i = 0; i < ui->verticalLayout_TaskList->count(); ++i)
    {
        QWidget *w = ui->verticalLayout_TaskList->itemAt(i)->widget();
        TaskChild *task = qobject_cast<TaskChild*>(w);
        if (!task) continue;

        //搜索任务
        QTime begin = task->getBeginTime();
        QTime end   = task->getEndTime();
        QString folder = task->getMusicFolder();
        if (folder.isEmpty()) continue;

        bool inTime = (now >= begin && now <= end);
        if (inTime) { shouldTask = task; break; } //设置任务
    }
    //没有任务需要播放：如果当前在播就停
    if (!shouldTask)
    {
        if (player->playbackState() == QMediaPlayer::PlayingState)
            player->stop();

        m_activeTask = nullptr;
        m_activeFile.clear();
        ui->label_Statu->setText("当前播放状态：等待中");
        return;
    }

    // 如果切换了任务：立即换到新任务的随机首歌
    if (m_activeTask != shouldTask)
    {
        m_activeTask = shouldTask;
        m_activeFile.clear(); // 强制重新选歌
        playNextRandomIfNeeded();
        return;
    }

    // 任务没变
    ui->label_Statu->setText(
        player->playbackState() == QMediaPlayer::PlayingState ? "当前播放状态：播放中"
                                                              : "当前播放状态：等待中"
        );
    // 如果当前没在播（比如刚进入时间段但尚未播放/或者被手动暂停），可以启动一次
    if (player->playbackState() != QMediaPlayer::PlayingState)
        playNextRandomIfNeeded();
}
void MainWindow::playNextRandomIfNeeded()
{
    if (!m_activeTask) return;

    QTime now = QTime::currentTime();
    QTime begin = m_activeTask->getBeginTime();
    QTime end   = m_activeTask->getEndTime();

    bool inTime = (now >= begin && now <= end);
    if (!inTime) {
        player->stop();
        m_activeTask = nullptr;
        m_activeFile.clear();
        ui->label_Statu->setText("当前播放状态：等待中");
        return;
    }

    QString folder = m_activeTask->getMusicFolder();
    QString nextFile = pickRandomFile(folder);
    if (nextFile.isEmpty()) return;

    m_activeFile = nextFile;
    ui->label_NowPlay->setText("当前歌曲："+nextFile.split("/")[nextFile.split("/").length()-1]);
    player->setSource(QUrl::fromLocalFile(nextFile));
    player->play();
    ui->label_Statu->setText("当前播放状态：播放中");
}
QString MainWindow::pickRandomFile(QString folder)
{
    QDir dir(folder);
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.ogg" << "*.flac";
    QStringList files = dir.entryList(filters, QDir::Files);
    if (files.isEmpty()) return {};

    int idx = QRandomGenerator::global()->bounded(files.size());
    return dir.absoluteFilePath(files.at(idx));
}
//切歌
void MainWindow::on_pushButton_Next_clicked()
{
    playNextRandomIfNeeded();
}


void MainWindow::on_checkBox_clicked(bool checked)
{
    //开机自启动
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    // 当前用户：HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);

    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString value = QString("\"%1\"").arg(appPath); // 路径带空格需加引号

    if (checked) reg.setValue("ZcAutoMusic", value);
    else reg.remove("ZcAutoMusic");
}

