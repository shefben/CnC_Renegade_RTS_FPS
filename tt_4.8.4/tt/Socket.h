#ifndef TT_INCLUDE__SOCKET_H
#define TT_INCLUDE__SOCKET_H



typedef uint32 Ip;
typedef uint16 Port;
typedef sockaddr_in Address;



class Socket
{

private:

	SOCKET handle;
	bool canShutdown; // This is only because Socket(SOCKET _handle) is not supposed to shut down when the destructor runs

public:

	Socket();
	Socket(SOCKET _handle);
	virtual ~Socket();

	bool bind(Ip address, Port port);
	virtual bool sendTo(const byte* data, int dataSize, const Address& address, uint flags = 0);
	virtual int receiveFrom(byte* data, int maxDataSize, Address& address, uint flags = 0);
	void setBufferSizes(uint bufferSize);
	int clearError();

	SOCKET getHandle() { return handle; }

};



#endif