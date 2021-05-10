#pragma once
#include <cstring>
#include <memory>
#include <stdio.h>
struct RGBModel
{
        //const char* rgbData;
        std::shared_ptr<char> rgbData;
        int h, w;
        unsigned int seq;

        RGBModel() = default;
        RGBModel(std::shared_ptr<char> _data, int _h, int _w, unsigned int _seq) : h(_h), w(_w), seq(_seq)
        {
                rgbData = _data;
        }
};

typedef struct BoxInfo
{
        int left;
        int top;
        int right;
        int bottom;
        int classNo;
        float score;
} BoxInfo;

struct GPSData
{
        uint32_t time_boot_ms;
        int32_t latitude;       //维度
        int32_t longitude;      //经度
        int32_t altitude;       //wgs高程
        int32_t relative_alt;   //相对高程
        int16_t ground_x_speed; //
        int16_t ground_y_speed;
        int16_t ground_z_speed;
        uint16_t hdg;
};

struct AttitudeData
{
        uint32_t time_boot_ms;
        float roll;  //翻滚角
        float pitch; //俯仰角
        float yaw;   //航向角
        float roll_speed;
        float pitch_speed;
        float yaw_speed;
};

struct BOXModel
{
        std::shared_ptr<BoxInfo> boxData;
        int num;
        unsigned int seq;
        // bool

        BOXModel() = default;
        BOXModel(BoxInfo *_data, int _num, unsigned int _seq) : num(_num), seq(_seq)
        {
                boxData = std::shared_ptr<BoxInfo>(new BoxInfo[_num], [](BoxInfo *p) { delete[] p; });
                memcpy((char *)boxData.get(), (char *)_data, sizeof(BoxInfo) * _num);
        }
};

typedef struct NETParam
{
        char dstIp[16];
        unsigned short dstPort;
        char localIp[16];
        unsigned char storageFrame;
} NETParam;

typedef int rphandle;

typedef void (*pf_rgb_callback)(RGBModel rgb_data, BOXModel box_data, void *);