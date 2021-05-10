/*
 * @Author: xtt
 * @Date: 2020-12-24 13:45:29
 * @Description: ...
 * @LastEditTime: 2021-05-05 20:59:08
 */
#pragma once
#include "byteArray.h"
#include "rtpmodel.h"
#include <cstring>
#include <deque>
#include <queue>
#include <stdint.h>
#include <vector>
#ifndef _UNIX
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif
#define RTP_PACKET_LEN 1500

struct RtpModel
{
        char *pdata = nullptr;
        int len = -1;
        unsigned short seq = -1;
        bool isValid = false;
        RtpModel(char *_data, int _len) : len(_len)
        {
                pdata = new char[_len];
                memcpy(pdata, _data, len);

                seq = ntohs(*(unsigned short *)&(pdata[2]));
                isValid = true;
        }
        RtpModel &operator=(const RtpModel &_rtpdata)
        {
                pdata = _rtpdata.pdata;
                len = _rtpdata.len;
                isValid = _rtpdata.isValid;
                seq = _rtpdata.seq;
                return *this;
        }
        RtpModel() = default;
};

class SocketUdp;

namespace std
{
        class thread;
}

typedef void (*pf_rtp_callback)(const char *, int, bool, uint32_t);

class RtpDataManager
{

public:
        RtpDataManager();
        ~RtpDataManager();

public:
        void init(const char *_ip, unsigned short _port, pf_rtp_callback, CODEC_TYPE _type);
        void start();
        void stop();
        void sendRtp() = delete;

private:
        static void receive_thread(void *arg);
        static void parser_thread(void *arg);
        bool parserRtpH264(char *pdata, int len, ByteArray &full_packet_data, bool &mark, uint32_t &timestamp);
        bool parserRtpH265(char *pdata, int len, ByteArray &full_packet_data, bool &mark, uint32_t &timestamp);

private:
        uint16_t last_seq = 0;
        bool is_start = false;
        SocketUdp *pSocket = nullptr;
        //SocketTcp* pSocketTcp = nullptr;
        std::thread *pThread_receive = nullptr;
        std::thread *pThread_parser = nullptr;
        std::vector<RtpModel> vec_rtp;
        pf_rtp_callback pf_callback = nullptr;
        CODEC_TYPE TYPE;
};
