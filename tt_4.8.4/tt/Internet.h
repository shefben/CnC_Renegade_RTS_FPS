#ifndef TT_INCLUDE__INTERNET_H
#define TT_INCLUDE__INTERNET_H



#include "Singleton.h"



#define internet (Internet::getInstance())



class InternetFile;



class Internet :
	public Singleton<Internet>
{

private:

	HINTERNET handle;

protected:

	friend Singleton<Internet>;

	Internet();
	~Internet();

public:
	
	InternetFile* openFile(const char* url);
	void closeFile(InternetFile* file);

};



#endif