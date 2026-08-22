#include "General.h"
#include "Config.h"



// TODO: Read configuration from file instead of using compiletime values.



bool Config::operator()(BOOL_CONFIG_ENTRY configEntry) const
{
	switch (configEntry)
	{

	case CONFIG_NET_DIAGNOSTICS: return false;
	case CONFIG_NET_SIMULATION: return false;
	case CONFIG_USE_TT_SPAWNERS: return true;
	default: TT_UNREACHABLE;

	}
}



int Config::operator()(INT_CONFIG_ENTRY configEntry) const
{
	switch (configEntry)
	{

	case CONFIG_NET_PACKET_LATENCY: return 1000;
	case CONFIG_NET_MAX_BANDWIDTH: return 56000;
	default: TT_UNREACHABLE;

	}
}



float Config::operator()(FLOAT_CONFIG_ENTRY configEntry) const
{
	switch (configEntry)
	{

	case CONFIG_NET_PACKET_LOSS: return 0.f;
	case CONFIG_NET_PACKET_DUPLICATION: return 0.f;
	default: TT_UNREACHABLE;

	}
}
