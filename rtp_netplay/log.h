//
//
// Copyright (c) 2018-2020 Snow Lake Inc. All rights reserved.
// email: yucheng@snowlake-tech.com
// Created by yucheng on 5/17/19
//
//

#ifndef _FSERVO_PUBLIC_H_
#define _FSERVO_PUBLIC_H_

#include <assert.h>
#include <stdio.h>
#include <cstdio>
#include <string.h>
#include <errno.h>


enum class LOG_LEVEL{
    TRACE   =0,
    DEBUG   =1,
    INFO    =2,
    WARN    =3,
    ERROR   =4
};

namespace{
    LOG_LEVEL logLevel=LOG_LEVEL::INFO;
}

#define OPT_LOG 2

#if OPT_LOG<2
#define LOG_DEBUG(format,...)                                                                   \
    do {                                                                                        \
        static const char* file_last_level = strrchr(__FILE__,'/');                             \
        static const char* file_name = file_last_level?file_last_level+1:file_last_level;       \
        fprintf(stderr, "[%s:%d][\033[1;36mdebug\033[0m] " format "\n",                        \
                    file_name, __LINE__, ##__VA_ARGS__);                                        \                                    
    } while (0) 
#else
#define LOG_DEBUG(format,...)  
#endif

#if OPT_LOG<3
#define LOG_INFO(format,...)                                                                    \
    do {                                                                                        \
        static const char* file_last_level = strrchr(__FILE__,'/');                             \
        static const char* file_name = file_last_level?file_last_level+1:file_last_level;       \
        fprintf(stderr, "[%s:%d][\033[1;32minfo\033[0m] " format "\n",                         \
                    file_name, __LINE__, ##__VA_ARGS__);                                        \                                    
    } while (0) 
#else
#define LOG_INFO(format,...)  
#endif

#if OPT_LOG<4
#define LOG_WARN(format,...)                                                                    \
    do {                                                                                        \
        static const char* file_last_level = strrchr(__FILE__,'/');                             \
        static const char* file_name = file_last_level?file_last_level+1:file_last_level;       \
        fprintf(stderr, "[%s:%d][\033[1;33mwarn\033[0m] " format "\n",                         \
                    file_name, __LINE__, ##__VA_ARGS__);                                        \                                    
    } while (0) 
#else
#define LOG_WARN(format,...)
#endif

#if OPT_LOG<5
#define LOG_ERR(format,...)                                                                     \
    do {                                                                                        \
        static const char* file_last_level = strrchr(__FILE__,'/');                             \
        static const char* file_name = file_last_level?file_last_level+1:file_last_level;       \
        fprintf(stderr, "[%s:%d][\033[1;31merror\033[0m] " format "\n",                        \
                    file_name, __LINE__, ##__VA_ARGS__);                                        \                                    
    } while (0) 
#else
#define LOG_ERR(format,...)   
#endif









#define RED_COLOR_WRAPPER(arg) "\033[40;31m" arg "\033[0m"
#define GREEN_COLOR_WRAPPER(arg) "\033[40;32m" arg "\033[0m"
#define YELLOW_COLOR_WRAPPER(arg) "\033[40;33m" arg "\033[0m"
namespace snowlake {


/* ----------------------------------------------------------------------
**                          MACROS for debug
** ---------------------------------------------------------------------- */
//Define log level

// log to print err info
#define    LEVEL_ERR                                    0

// log to print both in debug and release
#define    LEVEL_INFO                                   1

// log to print only in release
#define    LEVEL_DEBUG                                  2


#define FSERVO_LEVEL_TO_STR(level) (LEVEL_ERR==(level))?"Error ": ((LEVEL_DEBUG==(level))?"Debug ":"Info ")

#define  OPT_LOG_ENABLE
#define  OPT_DEBUG_ENABLE 0
#define  OPT_LOG_ENABLE

//Definition for log print begin
#if defined(OPT_LOG_ENABLE)
#define FSERVO_LOG(level, format, ...)                                          \
    do {                                                                        \
        static const char* file_last_level = strrchr(__FILE__,'/');             \
        static const char* file_name = file_last_level?file_last_level+1:file_last_level;  \
        if (LEVEL_ERR == level) {                                                 \
            fprintf(stderr, "[%s][%s:%d][errno: %s] " format "\n",              \
                    FSERVO_LEVEL_TO_STR(level),                                 \
                    file_name, __LINE__, strerror(errno), ##__VA_ARGS__);       \
        }else if((LEVEL_INFO == level) || ((LEVEL_DEBUG == level) && (bool)OPT_DEBUG_ENABLE)){ \
            fprintf(stderr, "[%s][%s:%d] " format "\n",                         \
                    FSERVO_LEVEL_TO_STR(level),                                 \
                    file_name, __LINE__, ##__VA_ARGS__);                        \
        }                                                                       \
    } while (0)
//Definition for log print end
#else
#define FSERVO_LOG(level, format, ...)
#endif


//#define    FSERVO_ASSERT(expression)                     assert((expression))
#define    FSERVO_ASSERT(expression) assert((expression))                     

//Definition for exception check begin
#define    FSERVO_CHECK(exp)                                                \
    do {                                                                    \
        if (false == (exp))                                                 \
        {                                                                   \
            LOG_ERR("Check failed, invalid expression:" #exp);  \
            FSERVO_ASSERT(exp);                                             \
        }                                                                   \
    } while (0)

#define    FSERVO_CHECK_BREAK(exp, ret_val)                                 \
        if (false == (exp))                                                 \
        {                                                                   \
            LOG_ERR("Check failed, invalid expression:" #exp);  \
            ret_val = ERR_NORM;                                             \
            break;                                                          \
        }
//Definition for exception check end

} // end of namespace snowlake

#endif //_FSERVO_PUBLIC_H_


