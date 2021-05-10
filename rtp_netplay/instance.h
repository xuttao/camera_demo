#pragma once

class NetControl;

class Instance
{
public:
	Instance() = delete;
	~Instance() = delete;
public:
	static int netInstance();
	static NetControl* getNetInstance(int);
};