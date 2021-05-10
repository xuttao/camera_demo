/*
 * @Author: xtt
 * @Date: 2020-12-25 13:48:44
 * @Description: ...
 * @LastEditTime: 2021-05-05 10:21:03
 */

#pragma once

#include <cstring>
#include <netinet/in.h>

#define MAX_RTP_PACKET_LEN 1450
#define FU_A 28
#define FU_B 29

#pragma pack(push, 1)
struct NALU_HEADER
{
        unsigned char F : 1;
        unsigned char NRI : 2;
        unsigned char Type : 5;
};

struct FU_INDICATOR
{
        unsigned char F : 1;
        unsigned char NRI : 2;
        unsigned char Type : 5;
};

struct FU_HEADER
{
        unsigned char S : 1;
        unsigned char E : 1;
        unsigned char R : 1;
        unsigned char Type : 5;
};

#pragma pack(pop)

struct RTP_HEADER
{
        unsigned char version;
        unsigned char mark;
        unsigned char load_type;
        unsigned short seq;
        unsigned int timestamp;
        unsigned int ssrc;

        RTP_HEADER()
        {
                memset(this, 0, sizeof(RTP_HEADER));
        }
};

inline int findStartCode(const char *_p)
{
        if (_p[0] == 0x00 && _p[1] == 0x00 && _p[2] == 0x00 && _p[3] == 0x01)
        {
                return 4;
        }
        else if (_p[0] == 0x00 && _p[1] == 0x00 && _p[2] == 0x01)
                return 3;
        else
                return -1;
}

inline bool findStartCode2(const char *_p, int &move_pos, int &start_code_len)
{
        bool res = false;
        if (_p[0] == 0x00 && _p[1] == 0x00 && _p[2] == 0x00 && _p[3] == 0x01)
        {
                move_pos = 4;
                start_code_len = 4;
                res = true;
        }
        else if (_p[0] == 0x00 && _p[1] == 0x00 && _p[2] == 0x01)
        {
                move_pos = 3;
                start_code_len = 3;
                res = true;
        }
        else
        {
                if (0x00 == _p[1])
                        move_pos = 1;
                else if (0x00 == _p[2])
                        move_pos = 2;
                else if (0x00 == _p[3])
                        move_pos = 3;
                else
                        move_pos = 4;
                start_code_len = -1;
        }
        return res;
}

inline char *getRtpHeader(unsigned short seq, int mark, uint32_t timestamp)
{
        static char rtp_head_buffer[12] = {0};
        memset(rtp_head_buffer, 0, 12);

        rtp_head_buffer[0] = 0x80;

        rtp_head_buffer[1] = 0x60;
        if (mark == 1)
        {
                rtp_head_buffer[1] = 0xe0;
        }

        *(unsigned short *)(&rtp_head_buffer[2]) = htons(seq);

        *(unsigned int *)(&rtp_head_buffer[4]) = htonl(timestamp);

        *(unsigned int *)(&rtp_head_buffer[8]) = htonl(0x12345678);

        return rtp_head_buffer;
}

inline RTP_HEADER getRtpHeader(const char *pdata)
{
        RTP_HEADER header;
        header.version = ((pdata[0] & 0xc0) >> 6);
        header.mark = (pdata[1] & 0x80) >> 7;
        header.load_type = pdata[1] & 0x7F;
        header.seq = ntohs(*(unsigned short *)(&pdata[2]));
        header.timestamp = ntohl(*(unsigned int *)(&pdata[4]));
        header.ssrc = ntohl(*(unsigned int *)(&pdata[8]));

        return header;
}