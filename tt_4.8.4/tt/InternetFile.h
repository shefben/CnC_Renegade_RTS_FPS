#ifndef TT_INCLUDE__INTERNETFILE_H
#define TT_INCLUDE__INTERNETFILE_H



class Internet;



class InternetFile
{

private:

	HINTERNET handle;

protected:

	friend Internet;

	InternetFile(HINTERNET _handle);
	~InternetFile();

public:

	int read(byte* data, uint maxDataSize);
	int getSize();
	int getHttpStatus();

};



#endif