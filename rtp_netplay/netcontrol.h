#pragma once

#include "param.h"
#include "receivemodel.h"
#include "rtpdatamanager.h"
#include "semaphore.h"
#include "socketbase.h"
#include <atomic>
#include <map>
#include <thread>

class NetControl
{
        struct BoxModel
        {
                ByteArray data;
                uint32_t timestamp;
                bool has_gps = false;
                GPSData gps_data;
                AttitudeData attitude_data;
                BoxModel() = default;
                BoxModel(const ByteArray &_data, uint32_t _tmp) : data(_data), timestamp(_tmp) {}
                BoxModel(const ByteArray &_data, uint32_t _tmp, const GPSData &_gps, const AttitudeData &_atti)
                    : data(_data), timestamp(_tmp), gps_data(_gps), attitude_data(_atti), has_gps(true) {}
        };
        struct RgbModel
        {
                std::shared_ptr<char> data;
                int h, w;
                uint32_t timestamp;
                RgbModel() = default;

                RgbModel(const char *_data, int _h, int _w, uint32_t _tmp) : h(_h), w(_w), timestamp(_tmp)
                {
                        data = std::shared_ptr<char>(new char[h * w * 3], std::default_delete<char[]>());
                        memcpy((char *)data.get(), _data, h * w * 3);
                }
        };

private:
        pf_rgb_callback pf_callback;
        RtpDataManager *pRtpManager = nullptr;
        SocketTcp *pTcpSock = nullptr;
        std::string localIp = "192.168.3.132";
        std::string dstIp = "192.168.3.61";
        unsigned dstPort = 7780;
        StreamType streamInfo;
        uint32_t receivedFrame = 0;
        uint8_t storageFrame = 5;

private:
        void *pCall = nullptr;
        std::mutex mutex_box;
        std::mutex mutex_rgb;
        ByteArray boxArray;
        std::atomic_bool is_stop = {true};
        std::Semaphore sem_msg;
        std::Semaphore sem_show;
        std::thread *pThread_receive = nullptr;
        std::thread *pThread_output = nullptr;
        std::deque<RgbModel> que_rgb;
        std::deque<BoxModel> que_box;
        std::map<uint32_t, RgbModel> mp_rgb;
        void clear_que();
        void parser_msg(ByteArray &byteArray);
        static void receive_thread(void *arg);
        static void output_thread(void *);
        static void rgb_callback(const uint8_t *pdata, int h, int w, uint32_t timestamp, void *arg);

public:
        NetControl() = default;
        NetControl(const NetControl &) = delete;
        NetControl &operator=(const NetControl &) = delete;
        ~NetControl();

public:
        void init(NETParam _param, pf_rgb_callback _pf, void *);
        bool start();
        void stop();
};