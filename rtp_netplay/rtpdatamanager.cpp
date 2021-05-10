#include "rtpdatamanager.h"
#include "log.h"
#include "rtpdatamodel.h"
#include "semaphore.h"
#include "socketbase.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
namespace
{
        std::atomic_bool is_stop = {true};
        std::atomic_bool can_send = {false};
        std::mutex buffer_mutex;
        std::mutex que_send_mutex;
        std::Semaphore sem_buffer;
        std::Semaphore sem_rtp;
        std::mutex mutex_rtp;
} // namespace

#define BUFFER_SIZE 4096

RtpDataManager::RtpDataManager()
{
}

RtpDataManager::~RtpDataManager()
{
        stop();
}

void RtpDataManager::init(const char *_ip, unsigned short _port, pf_rtp_callback _pf, CODEC_TYPE _type)
{
        pf_callback = _pf;
        TYPE = _type;
        pSocket = new SocketUdp;
        bool res = pSocket->createUdpServer(_ip, _port);

        //pSocketTcp = new SocketTcp;
        //pSocketTcp->createTcpServer(_ip, _port);

        FSERVO_CHECK(res);
}

void RtpDataManager::start()
{
        vec_rtp.resize(BUFFER_SIZE);
        is_stop = false;
        is_start = false;
        last_seq = 0;
        sem_buffer.init(0);
        pThread_receive = new std::thread(receive_thread, (void *)this);
        pThread_parser = new std::thread(parser_thread, (void *)this);
}

void RtpDataManager::stop()
{
        is_stop = true;
        sem_buffer.post();

        if (pThread_parser)
        {
                pThread_parser->join();
                delete pThread_parser;
                pThread_parser = nullptr;
        }

        if (pThread_receive)
        {
                pThread_receive->join();
                delete pThread_receive;
                pThread_receive = nullptr;
        }

        for (int i = 0; i < vec_rtp.size(); i++)
        {
                if (vec_rtp[i].isValid)
                {
                        delete[] vec_rtp[i].pdata;
                        vec_rtp[i].isValid = false;
                }
        }

        if (pSocket)
        {
                char *ptemp = new char[1500 * 10];
                pSocket->read(ptemp, 1500 * 10);
                pSocket->closeSocket();
                delete pSocket;
                pSocket = nullptr;
                delete[] ptemp;
        }
}

bool RtpDataManager::parserRtpH264(char *pdata, int len, ByteArray &full_packet_data, bool &mark, uint32_t &timestamp) //收齐了一帧返回true，否则返回false
{
        RTP_HEADER rtp_header = getRtpHeader(pdata);
        mark = rtp_header.mark;
        timestamp = rtp_header.timestamp;

#if 1
        auto seq = rtp_header.seq;
        //LOG_INFO("parser seq=%d", seq);
        //static int last_seq = 0;
        if (seq - last_seq != 1 && last_seq != 0 && last_seq != 65535)
        {
                is_start = false;
                LOG_ERR("seq err:current:%u,last:%u", seq, last_seq);
        }
        last_seq = seq;
#endif

        unsigned char key = pdata[12];

        if (key == 0x67 && !is_start)
        {
                full_packet_data.clear();
                is_start = true;
        }

        if (is_start)
        {
                unsigned char packet_type = key & 0x1f;
                static unsigned int iden = htonl(0x00000001);
                if (packet_type < 24) //单个NAL包
                {
                        // FSERVO_CHECK(rtp_header.mark == 1);
                        full_packet_data.clear();

                        full_packet_data.append((char *)&iden, sizeof(int));
                        full_packet_data.append(pdata + 12, len - 12);
                        return true;
                }
                else if (packet_type == 28) //分片模式
                {
                        unsigned char fu_header = pdata[13];
                        unsigned char S = (fu_header & 0x80) >> 7;
                        unsigned char E = (fu_header & 0x40) >> 6;
                        unsigned char R = (fu_header & 0x20) >> 5;

                        unsigned char nalu_header = 0;
                        nalu_header = nalu_header | (key & 0xe0);
                        nalu_header = nalu_header | (fu_header & 0x1f);

                        bool res = false;
                        if (S == 1)
                        {
                                full_packet_data.clear();

                                FSERVO_CHECK(rtp_header.mark == 0);
                                full_packet_data.append((char *)&iden, sizeof(int));
                                full_packet_data.append(nalu_header);
                                full_packet_data.append(pdata + 14, len - 14);
                        }
                        else if (E == 1)
                        {
                                FSERVO_CHECK(rtp_header.mark == 1);
                                full_packet_data.append(pdata + 14, len - 14);
                                res = true;
                        }
                        else
                        {
                                full_packet_data.append(pdata + 14, len - 14);
                        }
                        return res;
                        //return true;
                }
                else
                {
                        FSERVO_CHECK(false);
                }
        }
        return false;
}

bool RtpDataManager::parserRtpH265(char *pdata, int len, ByteArray &full_packet_data, bool &mark, uint32_t &timestamp)
{

        RTP_HEADER rtp_header = getRtpHeader(pdata);
        mark = rtp_header.mark;
        timestamp = rtp_header.timestamp;

#if 1
        int seq = rtp_header.seq;
        //static int last_seq = 0;
        if (seq - last_seq != 1 && last_seq != 0 && last_seq != 65535)
        {
                is_start = false;
                LOG_ERR("seq err:current:%d,last:%d", seq, last_seq);
        }
        last_seq = seq;
#endif
        //RtspDataManager::getInstance()->setSSRC(rtp_header.ssrc);
        //RtspDataManager::getInstance()->setSeq(rtp_header.seq);
        //RtspDataManager::getInstance()->setRtpVersion(rtp_header.version);
        /***********************************************
                h265 三种模式，单一NAL、分片、组合
                nal unit type 48 为组合，49为分片,
                1、19 、32、33为单一
                32 视频参数集vps
                33 序列参数集sps
                34 序列参数集pps
                39 增强信息sei
        ************************************************/

        //static bool is_start = false;

        unsigned char key = pdata[12];

        unsigned char unit_type = (key >> 1) & 0x3f;

        if (key == 0x40 && !is_start)
        {
                full_packet_data.clear();
                is_start = true;
        }

        if (is_start)
        {
                static unsigned int iden = htonl(0x00000001);

                if (49 == unit_type) //分片
                {
                        unsigned char Fu_Header = pdata[14];
                        unsigned char S = (Fu_Header >> 7) & 0x01;
                        unsigned char E = (Fu_Header >> 6) & 0x01;
                        unsigned char FuType = Fu_Header & 0x1f;

                        unsigned char nalu_header_1 = 0;
                        unsigned char nalu_header_2 = pdata[13];

                        nalu_header_1 = (nalu_header_1 | (Fu_Header & 0x3f)) << 1;
                        nalu_header_1 = nalu_header_1 | (key & 0x80);

                        bool res = false;
                        if (1 == S) //分片开始
                        {
                                full_packet_data.clear();
                                full_packet_data.append((char *)&iden, sizeof(int));
                                full_packet_data.append(nalu_header_1);
                                full_packet_data.append(nalu_header_2);
                                full_packet_data.append(pdata + 15, len - 15);

                                FSERVO_CHECK(rtp_header.mark == 0);
                        }
                        else if (1 == E) //分片结束
                        {
                                full_packet_data.append(pdata + 15, len - 15);
                                res = true;
                                FSERVO_CHECK(rtp_header.mark == 1);
                        }
                        else
                        {
                                full_packet_data.append(pdata + 15, len - 15);
                        }
                        return res;
                        //return true;
                }
                else if (48 == unit_type) //组合
                {
                        LOG_ERR("not support this unit_type:%d", 48);
                        FSERVO_CHECK(false);
                }
                else //单一
                {
                        full_packet_data.clear();
                        full_packet_data.append((char *)&iden, sizeof(int));
                        full_packet_data.append(pdata + 12, len - 12);

                        return true;
                }
        }

        return false;
}

void RtpDataManager::receive_thread(void *arg)
{
        LOG_INFO("rtp receive thread start! id:%lu", std::this_thread::get_id());

        RtpDataManager *pThis = static_cast<RtpDataManager *>(arg);
        char *receive_buffer = new char[1500];

        uint64_t index = 0;
        int times = 0;

        for (;;)
        {
                if (is_stop) break;

                int ret = pThis->pSocket->read(receive_buffer, 1500);
                if (ret <= 0)
                {
                        LOG_WARN("packet lost!!!");
                        std::this_thread::sleep_for(std::chrono::milliseconds(1)); //1毫秒
                        continue;
                }

                RtpModel rtpData(receive_buffer, ret);
                unsigned short seq = ntohs(*(unsigned short *)&(receive_buffer[2]));
                //LOG_INFO("receive seq:%d",seq);
                static unsigned short first_seq;
                if (0 == index)
                {
                        first_seq = seq;
                }

                int pos = ((uint64_t)(seq + 65536 * times) - first_seq) % BUFFER_SIZE;

                if (UINT16_MAX == seq)
                {
                        times++;
                }

                buffer_mutex.lock();

                if (pThis->vec_rtp[pos].isValid == true)
                {
                        LOG_WARN("packet is not use!!! seq:%d", pThis->vec_rtp[pos].seq);
                        delete[] pThis->vec_rtp[pos].pdata;
                }

                pThis->vec_rtp[pos] = rtpData;

                buffer_mutex.unlock();

                index++;

                if (index > 30)
                {
                        sem_buffer.post();
                }
        }
        delete[] receive_buffer;
        LOG_INFO("rtp receive thread exit");
}

void RtpDataManager::parser_thread(void *arg)
{
        LOG_INFO("rtp parser thread start! id:%lu", std::this_thread::get_id());
        RtpDataManager *pThis = static_cast<RtpDataManager *>(arg);
        uint64_t index = 0; //注意此处，对应起始seq

        for (;;)
        {
                if (is_stop) break;

                //int ret = sem_wait(&sem_buffer);
                sem_buffer.wait();

                int pos = index % BUFFER_SIZE;

                buffer_mutex.lock();

                auto data = pThis->vec_rtp[pos];

                if (false == data.isValid)
                {
                        LOG_WARN("packet is not valid!!! seq:%d,pos:%d", data.seq, pos);
                        buffer_mutex.unlock();
                        for (;;)
                        {
                                index++;
                                if (pThis->vec_rtp[index % BUFFER_SIZE].isValid)
                                {
                                        break;
                                }
                        }
                        continue;
                }
                pThis->vec_rtp[pos].isValid = false;

                buffer_mutex.unlock();

                char *pdata = data.pdata;
                unsigned short seq = data.seq;

                static ByteArray packet_data;
                bool res = false;
                bool mark = false;
                uint32_t timestamp;
                if (CODEC_TYPE::H264 == pThis->TYPE)
                        res = pThis->parserRtpH264(data.pdata, data.len, packet_data, mark, timestamp);
                else
                        res = pThis->parserRtpH265(data.pdata, data.len, packet_data, mark, timestamp);

                if (res)
                {
                        //送入vcu
                        (pThis->pf_callback)(packet_data.data(), packet_data.size(), mark, timestamp);

                        packet_data.clear();
                }

                delete[] data.pdata;

                index++;
        }

        LOG_INFO("rtp parser thread exit");
}
