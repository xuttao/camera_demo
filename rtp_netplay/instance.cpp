#include "instance.h"
#include "netcontrol.h"
#include <map>
#include <memory>
int Instance::netInstance()
{
	static int ins = 0;
	return ++ins;
}

NetControl* Instance::getNetInstance(int ins)
{
	static std::map<int, std::shared_ptr<NetControl> > mp;
	if (mp.find(ins) == mp.end())
	{
		//std::unique_ptr<NetControl> pNet = std::make_unique<NetControl>(new NetControl);
		mp[ins] = std::shared_ptr<NetControl>(new NetControl);
	}
	return mp[ins].get();
}