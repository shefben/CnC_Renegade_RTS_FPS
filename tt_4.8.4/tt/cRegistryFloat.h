#ifndef TT_INCLUDE__CREGISTRYFLOAT_H
#define TT_INCLUDE__CREGISTRYFLOAT_H
class cRegistryFloat
{
public:
	float Data;
	char Key[400];
	char Value[100];
	cRegistryFloat(const char *key, const char *value,float data);
	void Set(float data);
	inline operator float()
	{
		return Data;
	}
};
#endif
