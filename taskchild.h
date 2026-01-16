#ifndef TASKCHILD_H
#define TASKCHILD_H

#include "ui_taskchild.h"
#include <QWidget>
#include <QTime>

namespace Ui {
class TaskChild;
}

class TaskChild : public QWidget
{
    Q_OBJECT

public:
    explicit TaskChild(QWidget *parent = nullptr,QString TaskName = "",QTime BeingTime =QTime(),QTime EndTime = QTime(), QString MusicFolder="");
    QTime getBeginTime() const { return ui->timeEdit_Begin->time(); }
    QTime getEndTime() const { return ui->timeEdit_End->time(); }
    QString getMusicFolder() const { return ui->pushButton_MusicFolder->text(); }
    ~TaskChild();

private slots:
    void on_timeEdit_Begin_userTimeChanged(const QTime &time);
    void on_timeEdit_End_userTimeChanged(const QTime &time);

    void on_pushButton_Remove_clicked();

    void on_pushButton_MusicFolder_clicked();

private:
    Ui::TaskChild *ui;
    void SaveConfig();
    QString TaskNameL;
};

#endif // TASKCHILD_H
