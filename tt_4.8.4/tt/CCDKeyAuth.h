#ifndef TT_INCLUDE__CCDKEYAUTH_H
#define TT_INCLUDE__CCDKEYAUTH_H



class StringClass;



class CCDKeyAuth
{

public:

	static UNK GenChallenge(int);
	static UNK auth_callback(int, int, const char*, void*);
	static void DisconnectUser(int clientId);
	static UNK AuthenticateUser(int, uint32, char*, char*);
	static UNK AuthSerial(const char*, StringClass&);
	static void GetSerialHash(StringClass&);

};



#endif