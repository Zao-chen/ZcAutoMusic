#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "taskchild.h"
#include <QMainWindow>
#include <QMediaPlayer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_AddTask_clicked();

    void on_pushButton_Next_clicked();

    void on_checkBox_clicked(bool checked);

private:
    Ui::MainWindow *ui;
    QMediaPlayer *player; // 播放音乐
    QTimer *timer; // 定时检查 Task
    void checkTasks(); //检查任务函数
    QAudioOutput *audioOutput; // 必须是成员变量
    QHash<TaskChild*, QString> m_taskChosenFile; //每个任务在当前时间段锁定的歌曲
    TaskChild* m_activeTask = nullptr;    // 当前正在控制播放器的任务
    QString m_activeFile;                 // 当前正在播放的文件（可选）
    QString pickRandomFile(QString folder);
    void playNextRandomIfNeeded();
};
#endif // MAINWINDOW_H
