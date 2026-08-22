#ifndef TT_INCLUDE__CREGISTRYBOOL_H
#define TT_INCLUDE__CREGISTRYBOOL_H
class cRegistryBool
{
public:
	bool Data;
	char Key[400];
	char Value[100];
	cRegistryBool(const char *key, const char *value,bool data);
	void Toggle();
	void Set(bool data);
	inline operator bool()
	{
		return Data;
	}
};
#endif
