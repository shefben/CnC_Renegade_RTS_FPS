#ifndef TT_INCLUDE__THREAD_H
#define TT_INCLUDE__THREAD_H

class Thread
{

private:

	HANDLE handle;
	int id;

protected:
	
	virtual void execute() = 0;


public:

	Thread();
	~Thread();
	static void __cdecl ThreadMain(LPVOID parameter);
	virtual void start();
	virtual void reset();
	virtual void wait() const;
	virtual bool isRunning() const;

};
#endif
