#pragma once

#ifndef _UNIX

#ifdef RTP_NETPLAY_EXPORTS
#define RTP_NETPLAY_API __declspec(dllexport)
#else
#define RTP_NETPLAY_API __declspec(dllimport)
#endif

#else
#define RTP_NETPLAY_API
#endif