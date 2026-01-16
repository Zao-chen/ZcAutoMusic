#include "taskchild.h"
#include "ui_taskchild.h"

#include <QSettings>
#include <QFileDialog>

TaskChild::TaskChild(QWidget *parent,QString TaskName,QTime BginTime,QTime EndTime,QString MusicFolder)
    : QWidget(parent)
    , ui(new Ui::TaskChild)
{
    ui->setupUi(this);
    ui->timeEdit_Begin->setTime(BginTime);
    ui->timeEdit_End->setTime(EndTime);
    if(!MusicFolder.isEmpty()) ui->pushButton_MusicFolder->setText(MusicFolder);
    else ui->pushButton_MusicFolder->setText("选择文件夹");
    TaskNameL = TaskName;
    SaveConfig();
}

//保存配置函数
void TaskChild::SaveConfig()
{
    if (TaskNameL.isEmpty()) return;
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("TaskList");
    settings.beginGroup(TaskNameL);
    settings.setValue("BeginTime", ui->timeEdit_Begin->time().toString("HH:mm:ss"));
    settings.setValue("EndTime", ui->timeEdit_End->time().toString("HH:mm:ss"));
    settings.setValue("MusicFolder", ui->pushButton_MusicFolder->text());
    settings.endGroup();
    settings.endGroup();
}

TaskChild::~TaskChild()
{
    delete ui;
}

//修改后自动保存
void TaskChild::on_timeEdit_Begin_userTimeChanged(const QTime &time)
{
    SaveConfig();
}
void TaskChild::on_timeEdit_End_userTimeChanged(const QTime &time)
{
    SaveConfig();
}
//删除Task
void TaskChild::on_pushButton_Remove_clicked()
{
    if (TaskNameL.isEmpty()) return;
    //删除配置
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("TaskList");
    settings.remove(TaskNameL);
    settings.endGroup();
    //从 UI 中移除
    if (QLayout *layout = parentWidget()->layout()) layout->removeWidget(this);
    //删除自己
    this->deleteLater();
}


void TaskChild::on_pushButton_MusicFolder_clicked()
{
    //打开文件夹选择器
    QString dir = QFileDialog::getExistingDirectory(this, "选择音乐文件夹", "");
    if(!dir.isEmpty())
    {
        ui->pushButton_MusicFolder->setText(dir);
        SaveConfig();
    }
}

