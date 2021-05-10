#pragma once
#include "rtpmodel.h"
#include "semaphore.h"
#include <atomic>
#include <cstring>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
class FFmpegDecoder
{
        struct NALU
        {
                char *naluData = nullptr;
                int naluLen = 0;
                uint32_t timestamp = 0;
                NALU(const char *_data, int _len, uint32_t _t) : naluLen(_len), timestamp(_t) { naluData = new char[naluLen], memcpy(naluData, _data, naluLen); }
                NALU() = default;
        };

private:
        FFmpegDecoder() = default;
        ~FFmpegDecoder();

public:
        static FFmpegDecoder *getInstance();
        bool init(CODEC_TYPE type, std::function<void(const uint8_t *, int, int, uint32_t, void *)> _rgb_callback, void *);
        void start();
        void stop();
        void push(const char *data, int len, uint32_t timestamp);

private:
        void *pCall_this = nullptr;
        std::thread *thread_decode = nullptr;
        std::function<void(const uint8_t *, int, int, uint32_t, void *)> pf_rgb_callback;
        std::queue<NALU> que_nalu;
        std::mutex mutex_nalu;
        std::Semaphore sem_nalu;
        std::atomic_bool is_stop = {true};

private:
        static void decode_thread(void *);
};