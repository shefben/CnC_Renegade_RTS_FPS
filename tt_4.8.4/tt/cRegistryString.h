#ifndef TT_INCLUDE__CREGISTRYSTRING_H
#define TT_INCLUDE__CREGISTRYSTRING_H
class cRegistryString
{
public:
	char Data[200];
	char Key[400];
	char Value[100];
	cRegistryString(const char *key, const char *value,const char *data);
	void Set(const char *data);
	inline operator const char *()
	{
		return Data;
	}
};
#endif
