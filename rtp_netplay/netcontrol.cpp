#include "netcontrol.h"
#include "byteArray.h"
#include "ffmpegdecoder.h"
#include "receivemodel.h"
#ifndef _UNIX
#include <windows.h>
#endif
#include <iostream>

#if 0
static LARGE_INTEGER getFILETIMEoffset()
{
        SYSTEMTIME s;
        FILETIME f;
        LARGE_INTEGER t;

        s.wYear = 1970;
        s.wMonth = 1;
        s.wDay = 1;
        s.wHour = 0;
        s.wMinute = 0;
        s.wSecond = 0;
        s.wMilliseconds = 0;
        SystemTimeToFileTime(&s, &f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
        return (t);
}

static void clock_gettime(struct timeval *tv)
{
        LARGE_INTEGER t;
        FILETIME f;
        double microseconds;
        static LARGE_INTEGER offset;
        static double frequencyToMicroseconds;
        static int initialized = 0;
        static BOOL usePerformanceCounter = 0;

        if (!initialized)
        {
                LARGE_INTEGER performanceFrequency;
                initialized = 1;
                usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
                if (usePerformanceCounter)
                {
                        QueryPerformanceCounter(&offset);
                        frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
                }
                else
                {
                        offset = getFILETIMEoffset();
                        frequencyToMicroseconds = 10.;
                }
        }
        if (usePerformanceCounter)
                QueryPerformanceCounter(&t);
        else
        {
                GetSystemTimeAsFileTime(&f);
                t.QuadPart = f.dwHighDateTime;
                t.QuadPart <<= 32;
                t.QuadPart |= f.dwLowDateTime;
        }

        t.QuadPart -= offset.QuadPart;
        microseconds = (double)t.QuadPart / frequencyToMicroseconds;
        t.QuadPart = microseconds;
        tv->tv_sec = t.QuadPart / 1000000;
        tv->tv_usec = t.QuadPart % 1000000;
}

static uint64_t msec()
{
        struct timeval tv;
        clock_gettime(&tv);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif

#define BOX_FIRST 1
#define FRAME_FIRST 0
#define ONLY_SHOW 0

NetControl::~NetControl()
{
}

static void rtp_callback(const char *pdata, int len, bool mark, uint32_t timestamp)
{
        FFmpegDecoder::getInstance()->push(pdata, len, timestamp);
}

static bool send_request(SocketTcp *pSock, PacketType _type)
{
        PacketHead head;
        PacketEnd end;
        head.frameType = _type;
        char buffer[PACKET_END_LEN + PACKET_HEAD_LEN];
        memcpy(buffer, &head, PACKET_HEAD_LEN);
        memcpy(buffer + PACKET_HEAD_LEN, &end, PACKET_END_LEN);

        return pSock->sendTo(buffer, PACKET_HEAD_LEN + PACKET_END_LEN);
}

static bool find_pos(const char *data, int size, int &begin, int &end)
{
        PacketHead *pHead = (PacketHead *)data;
        int body_len = pHead->frameSize;
        auto c = PACKET_HEAD_LEN;
        auto c2 = PACKET_END_LEN;
        if (size >= (body_len + PACKET_HEAD_LEN + PACKET_END_LEN))
        {
                for (int i = 0; i < size && size >= (i + body_len + PACKET_HEAD_LEN + PACKET_END_LEN); i++)
                {
                        if ((PACKET_HEAD_FLAG == (uint8_t)data[i]) && (PACKET_END_FLAG == (uint8_t)data[i + body_len + PACKET_HEAD_LEN + PACKET_END_LEN - 1]))
                        {
                                begin = i;
                                end = i + body_len + PACKET_HEAD_LEN + PACKET_END_LEN - 1;

                                return true;
                        }
                }
        }

        return false;
}

static uint64_t get_current_msec(int year, int mon, int day, int hour, int min, int sec, int msec)
{
        if (0 >= (mon -= 2))
        {
                mon += 12;
                year -= 1;
        }
        return ((uint64_t)((((uint32_t)(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) + year * 365 - 719499) * 24 + hour) * 60 + min) * 60 + sec) * 1000 + msec;
}

void NetControl::rgb_callback(const uint8_t *pdata, int h, int w, uint32_t timestamp, void *arg)
{
        LOG_DEBUG("rgb callback:%d", h * w * 3);

        NetControl *pThis = (NetControl *)arg;

#ifdef _DEBUG
        static uint32_t last = 1;
        if (last == timestamp && last != 1)
        {
                LOG_ERR("same timestamp!:%d", timestamp);
                assert(false);
        }
        last = timestamp;

#endif

        {
                std::lock_guard<std::mutex> locker(pThis->mutex_rgb);

#if ONLY_SHOW
                pThis->que_rgb.push_back(RgbModel((char *)pdata, h, w, timestamp));
                pThis->sem_show.post();
#elif BOX_FIRST
                pThis->mp_rgb.insert(std::make_pair(timestamp, RgbModel((char *)pdata, h, w, timestamp)));
#elif FRAME_FIRST
                pThis->mp_rgb.insert(std::make_pair(timestamp, RgbModel((char *)pdata, h, w, timestamp)));
                //pThis->receivedFrame++;
                //if(pThis->receivedFrame>5)
                pThis->sem_show.post();
#endif
        }
}

void NetControl::clear_que()
{
        while (!que_rgb.empty())
        {
                que_rgb.pop_front();
        }
        while (!que_box.empty())
        {
                que_box.pop_front();
        }
        mp_rgb.clear();
}

void NetControl::parser_msg(ByteArray &byteArray)
{
        if (byteArray.size() >= PACKET_HEAD_LEN)
        {
                int bpos, epos;
                bool res = find_pos(byteArray.data(), byteArray.size(), bpos, epos);
                if (res)
                {
                        char *pdata = byteArray.data() + bpos;
                        PacketHead *pHead = (PacketHead *)(pdata);
                        //PacketEnd* pEnd = (PacketEnd*)(pdata + PACKET_HEAD_LEN + pHead->frameSize);
                        if (BOX == pHead->frameType)
                        {
                                boxArray.append(pdata + PACKET_HEAD_LEN, pHead->frameSize);
                                if (pHead->curFrame == pHead->totFrame)
                                {
#if ONLY_SHOW == 0
                                        mutex_box.lock();
                                        que_box.push_back(BoxModel(boxArray, pHead->picID));
                                        mutex_box.unlock();
#endif
                                        boxArray.clear();
#if BOX_FIRST
                                        sem_show.post();
#endif
                                }
                                byteArray.remove(0, epos + 1);
                        }
                        else if (GPS == pHead->frameType)
                        {
                        }
                        else if (STREAMPARAM == pHead->frameType)
                        {
                                streamInfo = *(StreamType *)(pdata + PACKET_HEAD_LEN);
                                LOG_INFO("get stream info:%s", streamInfo.type == 0 ? "h264" : "h265");
                                sem_msg.post();
                                byteArray.clear();
                        }
                        else
                        {
                                LOG_WARN("unknow pactet");
                                byteArray.clear();
                        }
                }
                else if (byteArray.size() > 1024)
                {
                        LOG_WARN("pactet parser failed! len:%d", byteArray.size());
                        byteArray.clear();
                }
        }
}

void NetControl::receive_thread(void *arg)
{
        NetControl *pThis = static_cast<NetControl *>(arg);
        char receive_buffer[1024];
        ByteArray full_packet;
        pThis->pTcpSock->setRecvTimeout(0, 20);
        for (;;)
        {
                if (pThis->is_stop) break;

                int ret = pThis->pTcpSock->read(receive_buffer, 1024);
                if (ret <= 0)
                {
                        if (SocketTcp::CONNECTED == pThis->pTcpSock->getConnectStatus())
                        {
                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                continue;
                        }
                        LOG_ERR("connect break!");
                        break;
                }
                full_packet.append(receive_buffer, ret);
                pThis->parser_msg(full_packet);
        }
}

void NetControl::output_thread(void *arg)
{
        NetControl *pThis = static_cast<NetControl *>(arg);
        bool isFirst = true;
        for (;;)
        {
                pThis->sem_show.wait();

                if (pThis->is_stop) break;

#if ONLY_SHOW
                RgbModel rgb_model;
                {
                        std::lock_guard<std::mutex> locker(pThis->mutex_rgb);
                        if (pThis->que_rgb.empty()) continue;

                        rgb_model = pThis->que_rgb.front();
                        pThis->que_rgb.pop_front();
                }

                RGBModel rgb(rgb_model.data, rgb_model.h, rgb_model.w, 0);
                int boxnum = 0;
                BOXModel box((BoxInfo *)nullptr, boxnum, 0);
                pThis->pf_callback(rgb, box, pThis->pCall);

#elif BOX_FIRST
                BoxModel box_model;
                RgbModel rgb_model;
                bool has_box = false;
                bool has_frame = true;
                {
                        std::lock_guard<std::mutex> locker(pThis->mutex_rgb);
                        if (pThis->mp_rgb.empty()) continue;

                        pThis->mutex_box.lock();
                        box_model = pThis->que_box.front();
                        pThis->que_box.pop_front();
                        pThis->mutex_box.unlock();

                        auto ite = pThis->mp_rgb.lower_bound(box_model.timestamp);

                        if (ite == pThis->mp_rgb.end())
                        {
                                // auto last = pThis->mp_rgb.end();
                                // if (std::abs((int)((--last)->first - box_model.timestamp)) > 10 * 3600)
                                // {
                                //         LOG_ERR("------------------box too more!");
                                //         assert(false);
                                // }

                                // pThis->mutex_box.lock();
                                // pThis->que_box.push_front(box_model);
                                // pThis->mutex_box.unlock();

                                LOG_WARN("------------------box wait pic!");

                                continue;
                        }
                        else if (box_model.timestamp == ite->first)
                        {
                                has_box = true;
                                rgb_model = ite->second;
                        }
                        else
                        {
                                has_frame = false;
                                LOG_WARN("------------------no frame!");
                        }
                        pThis->mp_rgb.erase(pThis->mp_rgb.begin(), ite);
                }
                if (has_frame)
                {
                        RGBModel rgb(rgb_model.data, rgb_model.h, rgb_model.w, 0);
                        int boxnum = has_box ? box_model.data.size() / sizeof(BoxInfo) : 0;
                        BOXModel box((BoxInfo *)box_model.data.data(), boxnum, 0);

                        pThis->pf_callback(rgb, box, pThis->pCall);
                }
#elif FRAME_FIRST
                BoxModel box_model;
                RgbModel rgb_model;
                bool has_box = false;
                {
                        std::lock_guard<std::mutex> locker(pThis->mutex_box);
                        if (pThis->que_box.empty())
                        {
                                isFirst = false;
                                continue;
                        }
                        box_model = pThis->que_box.front();
                        pThis->que_box.pop_front();
                }

                {
                        std::lock_guard<std::mutex> locker(pThis->mutex_rgb);
                        auto ite = pThis->mp_rgb.lower_bound(box_model.timestamp);

                        if (pThis->mp_rgb.cend() == ite)
                        {
                                LOG_WARN("box wait frame!");
                        }
                        else if (ite->second.timestamp == box_model.timestamp)
                        {
                                has_box = true;
                                rgb_model = ite->second;
                        }
                        else
                        {
                                rgb_model = ite->second;

                                LOG_WARN("no match frame! frame :%d,box :%d", ite->second.timestamp, box_model.timestamp);
                        }
                        pThis->mp_rgb.erase(pThis->mp_rgb.begin(), ite);

                        FSERVO_CHECK(pThis->mp_rgb.size() < 50);
                }

                RGBModel rgb(rgb_model.data, rgb_model.h, rgb_model.w, 0);
                int boxnum = has_box ? box_model.data.size() / sizeof(BoxInfo) : 0;
                BOXModel box((BoxInfo *)box_model.data.data(), boxnum, 0);

                pThis->pf_callback(rgb, box, pThis->pCall);
#endif
        }
}

void NetControl::init(NETParam _param, pf_rgb_callback _pf, void *arg)
{
        localIp = _param.localIp;
        dstIp = _param.dstIp;
        dstPort = _param.dstPort;

        pf_callback = _pf;
        storageFrame = _param.storageFrame;
        pCall = arg;
}

bool NetControl::start()
{
        stop();

        boxArray.clear();
        sem_msg.init(0);
        sem_show.init(0);
        clear_que();

        receivedFrame = 0;

        pTcpSock = new SocketTcp;
        bool res = pTcpSock->connetToServer(dstIp, dstPort);
        if (!res)
        {
                LOG_ERR("connect to %s fail!", dstIp.c_str());
                return false;
        }

        is_stop = false;
        pThread_receive = new std::thread(receive_thread, this);
        pThread_output = new std::thread(output_thread, this);

        res = send_request(pTcpSock, STREAMPARAM);
        if (!res)
        {
                LOG_ERR("send request fail!");
                return false;
        }
        res = sem_msg.wait(1000);
        if (!res)
        {
                LOG_ERR("get stream param time out!");
                return false;
        }
        auto type = streamInfo.type == 0 ? CODEC_TYPE::H264 : CODEC_TYPE::H265;
        res = FFmpegDecoder::getInstance()->init(type, rgb_callback, this);
        if (!res)
        {
                return false;
        }
        FFmpegDecoder::getInstance()->start();

        pRtpManager = new RtpDataManager;
        pRtpManager->init(localIp.c_str(), 8880, rtp_callback, type);
        pRtpManager->start();

        res = send_request(pTcpSock, BOX);
        if (!res)
        {
                LOG_ERR("send request fail!");
        }
        return res;
}

void NetControl::stop()
{
        is_stop = true;
        sem_show.post();

        if (pRtpManager)
        {
                pRtpManager->stop();
                delete pRtpManager;
                pRtpManager = nullptr;
        }

        if (pThread_receive)
        {
                pThread_receive->join();
                delete pThread_receive;
                pThread_receive = nullptr;
        }

        if (pThread_output)
        {
                pThread_output->join();
                delete pThread_output;
                pThread_output = nullptr;
        }

        if (pTcpSock)
        {
                pTcpSock->closeSocket();
                delete pTcpSock;
                pTcpSock = nullptr;
        }

        FFmpegDecoder::getInstance()->stop();
}