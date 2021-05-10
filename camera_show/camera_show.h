#pragma once

#include "rtp_netplay.h"
#include "ui_camera_show.h"
#include <QObject>
#include <QPaintEvent>
#include <QThread>
#include <QTimer>
#include <QtWidgets/QMainWindow>
struct CallBackData
{
        RGBModel rgb_data;
        BOXModel box_data;

        CallBackData(RGBModel &rgb, BOXModel &box) : rgb_data(rgb), box_data(box) {}
};

class FrameManager : public QThread
{
        Q_OBJECT
public:
        FrameManager(QObject *parent = nullptr);
        ~FrameManager();
        FrameManager(const FrameManager &) = delete;
        FrameManager &operator=(const FrameManager &) = delete;

private:
        std::atomic_bool is_stop = {true};

protected:
        virtual void run();

public:
        void stop() { is_stop = true; }
public slots:

signals:
        void sigFrame(RGBModel, BOXModel);
};

class camera_show : public QMainWindow
{
        Q_OBJECT

public:
        camera_show(QWidget *parent = Q_NULLPTR, const char *_ip = NULL);

public:
        Ui::camera_showClass ui;

private slots:
        void slot_btn();
        void slot_show();
public slots:
        void slot_show2(RGBModel, BOXModel);

private:
        const char *dstIp;

protected:
        virtual void paintEvent(QPaintEvent *e);

private:
        QTimer timer;
        bool is_play = false;
        FrameManager *pFrameManager = nullptr;
};
