#include "rtp_netplay.h"
#include "netcontrol.h"
#include "instance.h"

rphandle rtp_instance()
{
	return Instance::netInstance();
}

void rtp_netplay_init(rphandle _handle,NETParam _param, pf_rgb_callback _pf, void* arg)
{
	Instance::getNetInstance(_handle)->init(_param, _pf, arg);
}

bool rtp_netplay_start(rphandle _handle)
{
	return Instance::getNetInstance(_handle)->start();
}

void rtp_netplay_stop(rphandle _handle)
{
	Instance::getNetInstance(_handle)->stop();
}