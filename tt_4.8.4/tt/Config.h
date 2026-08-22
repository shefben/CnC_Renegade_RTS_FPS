#ifndef TT_INCLUDE__CONFIG_H
#define TT_INCLUDE__CONFIG_H



#include "Singleton.h"



#define config (Config::getInstance())



enum BOOL_CONFIG_ENTRY
{
	CONFIG_NET_DIAGNOSTICS,
	CONFIG_NET_SIMULATION,
	CONFIG_USE_TT_SPAWNERS,
};



enum INT_CONFIG_ENTRY
{
	CONFIG_NET_PACKET_LATENCY,
	CONFIG_NET_MAX_BANDWIDTH,
};



enum FLOAT_CONFIG_ENTRY
{
	CONFIG_NET_PACKET_LOSS,
	CONFIG_NET_PACKET_DUPLICATION,
};



class Config :
	public Singleton<Config>
{

protected:

	friend Singleton<Config>;
	Config() {}

public:

	bool operator()(BOOL_CONFIG_ENTRY configEntry) const;
	int operator()(INT_CONFIG_ENTRY configEntry) const;
	float operator()(FLOAT_CONFIG_ENTRY configEntry) const;

};



#endif