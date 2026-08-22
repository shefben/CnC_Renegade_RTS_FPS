#ifndef TT_INCLUDE__CREGISTRYINT_H
#define TT_INCLUDE__CREGISTRYINT_H
class cRegistryInt
{
public:
	int Data;
	char Key[400];
	char Value[100];
	cRegistryInt(const char *key, const char *value,int data);
	void Set(int data);
	inline operator int()
	{
		return Data;
	}
};
#endif
