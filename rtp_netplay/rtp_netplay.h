#ifndef __RTP_NETPLAY_H__
#define __RTP_NETPLAY_H__

#include "config.h"
#include "param.h"


#ifdef __cplusplus
extern "C" {
#endif

	RTP_NETPLAY_API rphandle rtp_instance();

	RTP_NETPLAY_API void rtp_netplay_init(rphandle _handle,NETParam _param, pf_rgb_callback _pf, void* arg);

	RTP_NETPLAY_API bool rtp_netplay_start(rphandle _handle);

	RTP_NETPLAY_API void rtp_netplay_stop(rphandle _handle);


#ifdef __cplusplus
}
#endif


#endif