#include "camera_show.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <queue>
#ifndef _UNIX
#include <windows.h>
#endif
#include <QPainter>
#include <QTest>
#include <queue>
namespace
{
        const char *names_fptr[] = {"person", "car", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
                                    "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
                                    "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
                                    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
                                    "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
                                    "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
                                    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
                                    "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
                                    "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book",
                                    "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"};

        const int color_num = 18;

        const char *pColor[color_num] = {"220,20,60", "0,0,255", "30,144,255", "0,255,255", "0,255,0", "255,215,0", "255,165,0", "210,105,30", "255,69,0", "	255,0,0",
                                         "148,0,211", "70,130,180", "32,178,170", "85,107,47", "255,215,0", "255,140,0", "255,127,80", "0,191,255"};

        std::map<int, std::vector<unsigned char>> mp_color;

        std::mutex mutex_callData;
        std::queue<CallBackData> que_callData;
        QImage showImage;
        bool isfirst = true;
} // namespace

Q_DECLARE_METATYPE(RGBModel)
Q_DECLARE_METATYPE(BOXModel)

FrameManager::FrameManager(QObject *parent) : QThread(parent)
{
        qRegisterMetaType<RGBModel>("RGBModel");
        qRegisterMetaType<BOXModel>("BOXModel");
}

FrameManager::~FrameManager()
{
        is_stop = true;
}

void FrameManager::run()
{
        is_stop = false;
        for (;;)
        {
                if (is_stop) break;

                RGBModel rgb;
                BOXModel box;
                {
                        std::lock_guard<std::mutex> locker(mutex_callData);
                        if (que_callData.size() < 3)
                        {
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                continue;
                        }
                        rgb = que_callData.front().rgb_data;
                        box = que_callData.front().box_data;
                        que_callData.pop();
                }

                emit sigFrame(rgb, box);
                QTest::qSleep(30);
        }
}

camera_show::camera_show(QWidget *parent, const char *_ip)
    : QMainWindow(parent), dstIp(_ip)
{
        ui.setupUi(this);

        connect(ui.btnStart, &QPushButton::clicked, this, &camera_show::slot_btn);

        pFrameManager = new FrameManager;
        connect(pFrameManager, &FrameManager::sigFrame, this, &camera_show::slot_show2);
        connect(&timer, &QTimer::timeout, this, &camera_show::slot_show);
}

static std::vector<std::string> split_string(const std::string str, const char splitStr)
{
        std::vector<std::string> vecStr;
        std::string::size_type beginPos = str.find_first_not_of(splitStr, 0);
        std::string::size_type endPos = str.find_first_of(splitStr, beginPos);
        while (std::string::npos != endPos || std::string::npos != beginPos)
        {
                vecStr.push_back(str.substr(beginPos, endPos - beginPos));
                beginPos = str.find_first_not_of(splitStr, endPos);
                endPos = str.find_first_of(splitStr, beginPos);
        }

        return vecStr;
}

static void draw_box(cv::Mat &imgdata, void *_pBox, int boxNum)
{
#if 1
        BoxInfo *pBox = (BoxInfo *)_pBox;

        for (int num = 0; num < boxNum; num++)
        {
                BoxInfo *pBoxData = pBox + num;

                int x1 = pBoxData->left;
                int y1 = pBoxData->top;
                int x2 = pBoxData->right;
                int y2 = pBoxData->bottom;

                float score = pBoxData->score;
                int class_no = pBoxData->classNo;

                cv::Rect r(x1, y1, x2 - x1, y2 - y1);

                cv::Scalar cvColor;
                auto ite = mp_color.find(class_no);
                if (ite == mp_color.end())
                {
                        const char *color = pColor[num % color_num];
                        auto vec = split_string(std::string(color), ',');

                        std::vector<unsigned char> vec2;
                        vec2.resize(3);
                        vec2[0] = std::atoi(vec[0].c_str());
                        vec2[1] = std::atoi(vec[1].c_str());
                        vec2[2] = std::atoi(vec[2].c_str());

                        mp_color[class_no] = vec2;
                        cvColor = cv::Scalar(vec2[0], vec2[1], vec2[2]);
                }
                else
                {
                        cvColor = cv::Scalar(ite->second[0], ite->second[1], ite->second[2]);
                }

                cv::rectangle(imgdata, r, cvColor, 2, 1, 0);

                //std::cout << "---------------draw box :" << x1 << " " << y1 << " " << x2 - x1 << " " << y2 - y1 << " " << std::endl;

                int fontFace = 0;
                double fontScale = 0.4; //�������ű�
                int thickness = 1;
                int baseline = 0;

                cv::Size textSize = cv::getTextSize(std::string(names_fptr[pBoxData->classNo]) + std::to_string(score), fontFace, fontScale, thickness, &baseline);
                cv::rectangle(imgdata, cv::Point(x1 - 1, y1), cv::Point(x1 + textSize.width + thickness, y1 - textSize.height - 4), cvColor, -1, 1, 0);
                cv::Point txt_org(x1 - 1, y1 - 4);
                cv::putText(imgdata, std::string(names_fptr[pBoxData->classNo]) + " " + std::to_string(score), txt_org, fontFace, fontScale, cv::Scalar(255, 255, 255));
        }
#endif
}

static void call_back(RGBModel rgb_data, BOXModel box_data, void *arg)
{
        camera_show *pThis = static_cast<camera_show *>(arg);
#if 1
        //static int num = 0;
        //num++;
        //static auto t1 = QDateTime::currentMSecsSinceEpoch();
        //if (num % 25 == 0) {
        //	auto t2 = QDateTime::currentMSecsSinceEpoch();
        //	fprintf(stderr, "---------------fps:%f pic_index:%d,box_index:%d\n", 250.f / (t2 - t1)*1000
        //	,rgb_data.seq,box_data.seq);
        //	t1 = t2;
        //}

        {
                std::lock_guard<std::mutex> locker(mutex_callData);
                que_callData.push(CallBackData(rgb_data, box_data));
                //if (que_callData.size() > 2) {
                //	fprintf(stderr, "frame show delay :%d\n", que_callData.size());
                //}
        }
#endif

#if 0
	static bool is_first = true;
	static auto t1 = QDateTime::currentMSecsSinceEpoch();
	if (!is_first)
	{
		auto t2= QDateTime::currentMSecsSinceEpoch();
		qDebug() << "frame diff:" << t2 - t1;
		t1 = t2;
	}
	is_first = false;

	cv::Mat pic_data(rgb_data.h, rgb_data.w, CV_8UC3, cv::Scalar(0, 0, 0));
	pic_data.data = (uint8_t*)rgb_data.rgbData.get();

	if (box_data.num > 0)
	{
		draw_box(pic_data, box_data.boxData.get(), box_data.num);
	}

	QImage img((uchar*)pic_data.data, pic_data.cols, pic_data.rows, QImage::Format_RGB888);
	pThis->ui.lbePic->setPixmap(QPixmap::fromImage(img));
	pThis->update();

#endif
}

void camera_show::paintEvent(QPaintEvent *e)
{
        static int num = 0;
        num++;
        static auto t1 = QDateTime::currentMSecsSinceEpoch();
        if (num % 250 == 0)
        {
                auto t2 = QDateTime::currentMSecsSinceEpoch();
                fprintf(stderr, "---------------fps:%f\n", 250.f / (t2 - t1) * 1000);
                t1 = t2;
        }

        QPainter painter(this);

        //painter.setBrush(Qt::black);
        //painter.drawRect(0, 0, this->width(), this->height()); //�Ȼ��ɺ�ɫ

        //if (mImage.size().width() <= 0) return;

        ///��ͼ�񰴱������ųɺʹ���һ����С
        //QImage img = mImage.scaled(this->size(), Qt::KeepAspectRatio);

        //int x = this->width() - img.width();
        //int y = this->height() - img.height();

        int x = ui.lbePic->x();
        int y = ui.lbePic->y();

        //x /= 2;
        //y /= 2;

        painter.drawImage(QPoint(x, y), showImage);
}

void camera_show::slot_show2(RGBModel rgb, BOXModel box)
{
        //static bool is_first = true;
        //static auto t1 = QDateTime::currentMSecsSinceEpoch();
        //if (!is_first)
        //{
        //	auto t2 = QDateTime::currentMSecsSinceEpoch();
        //	std::cout << "----time :" << t2 - t1 << std::endl;
        //	t1 = t2;
        //	fflush(stdout);
        //}
        //is_first = false;

        cv::Mat pic_data(rgb.h, rgb.w, CV_8UC3, cv::Scalar(0, 0, 0));
        pic_data.data = (uint8_t *)rgb.rgbData.get();

        if (box.num > 0)
        {
                draw_box(pic_data, box.boxData.get(), box.num);
        }

        QImage img((uchar *)pic_data.data, pic_data.cols, pic_data.rows, QImage::Format_RGB888);
        ui.lbePic->setPixmap(QPixmap::fromImage(img));
        update();
}

void camera_show::slot_show()
{
        //static bool is_first = true;
        //static auto t1 = QDateTime::currentMSecsSinceEpoch();
        //if (!is_first)
        //{
        //	auto t2 = QDateTime::currentMSecsSinceEpoch();
        //	std::cout << "----time :" << t2 - t1 << std::endl;
        //	t1 = t2;
        //	fflush(stdout);
        //}
        //is_first = false;

        RGBModel rgb;
        BOXModel box;
        {
                //std::lock_guard<std::mutex> locker(mutex_callData);
                std::lock_guard<std::mutex> locker(mutex_callData);
                if (isfirst)
                {
                        if (que_callData.size() < 5)
                        {
                                return;
                        }
                        else
                        {
                                isfirst = false;
                        }
                }
                if (que_callData.size() < 2)
                {
                        fprintf(stderr, "wait frame\n");
                        isfirst = true;
                        return;
                }

                rgb = que_callData.front().rgb_data;
                box = que_callData.front().box_data;

                que_callData.pop();
        }

        cv::Mat pic_data(rgb.h, rgb.w, CV_8UC3, cv::Scalar(0, 0, 0));
        pic_data.data = (uint8_t *)rgb.rgbData.get();

        if (box.num > 0)
        {
                draw_box(pic_data, box.boxData.get(), box.num);
        }

        //QImage img((uchar*)pic_data.data, pic_data.cols, pic_data.rows, QImage::Format_RGB888);
        //
        //ui.lbePic->setPixmap(QPixmap::fromImage(img));
        //update();

        QImage img((uchar *)pic_data.data, pic_data.cols, pic_data.rows, QImage::Format_RGB888);
        showImage = img.copy();
        update();
}

void camera_show::slot_btn()
{
        static rphandle handle = rtp_instance();
        if (!is_play)
        {
                while (!que_callData.empty())
                        que_callData.pop();
                //pFrameManager->start();

                NETParam param;
                memcpy(param.dstIp, dstIp, strlen(dstIp) + 1);
                memcpy(param.localIp, "192.168.3.132", sizeof("192.168.3.132"));
                param.dstPort = 7780;
                param.storageFrame = 5;

                rtp_netplay_init(handle, param, call_back, this);
                bool res = rtp_netplay_start(handle);
                if (!res)
                {
                        QMessageBox::warning(this, "waring", "paly failed");
                        return;
                }
                timer.setTimerType(Qt::PreciseTimer);
                timer.start(40); //ˢ��ʱ�価���̣ܶ���ʱȡ����������ʾ
                is_play = true;
        }
        else
        {
                rtp_netplay_stop(handle);

                //pFrameManager->stop();
                //pFrameManager->quit();
                //pFrameManager->wait();

                ui.lbePic->clear();
                update();

                timer.stop();
                is_play = false;
        }
}
