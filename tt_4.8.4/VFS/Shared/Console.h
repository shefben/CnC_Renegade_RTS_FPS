#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#include <windows.h>


#define CON_RED					(FOREGROUND_RED		| FOREGROUND_INTENSITY)
#define CON_GREEN				(FOREGROUND_GREEN	| FOREGROUND_INTENSITY)
#define CON_BLUE				(FOREGROUND_BLUE	| FOREGROUND_INTENSITY)
#define CON_DARK_RED			(FOREGROUND_RED		)
#define CON_DARK_GREEN			(FOREGROUND_GREEN	)
#define CON_DARK_BLUE			(FOREGROUND_BLUE	)

#define CON_WHITE				(CON_RED	| CON_GREEN	| CON_BLUE)
#define CON_PINK				(CON_RED	| CON_BLUE	)
#define CON_YELLOW				(CON_RED	| CON_GREEN	)
#define CON_CYAN				(CON_GREEN	| CON_BLUE	)

#define CON_DARK_MAGENTA		(CON_DARK_RED	| CON_DARK_BLUE		)
#define CON_DARK_YELLOW			(CON_DARK_RED	| CON_DARK_GREEN	)
#define CON_DARK_CYAN			(CON_DARK_GREEN	| CON_DARK_BLUE		)

#define CON_DARK_GREY			(FOREGROUND_INTENSITY)
#define CON_LIGHT_GREY			(CON_DARK_RED | CON_DARK_GREEN | CON_DARK_BLUE)


class CConsole {
private:
	HANDLE	m_hConsoleOut;
	HANDLE	m_hConsoleIn;
	bool	m_Inited;
public:
	CConsole(bool init);
	~CConsole();
	void Init();
	void SetTitle(const wchar_t* pTitle);
	void SetInfo(int w, int h, int bh);
	void FillAttributes(WORD attributes);
	void Write(const wchar_t* pText, WORD Attributes = 0xFFFF);
	void WriteFmt(const wchar_t* pText, WORD Attributes, ...);
	//Attributes, fmt, arg count, args, attributes, fmt, ...
	void WriteMultiFmt(const wchar_t* pText, WORD Attributes, int count, ...);
	wchar_t ReadChar();
	void GetHandles(HANDLE *phConsoleIn, HANDLE *phConsoleOut);
};


class CRemoteConsole {
private:
	HANDLE	m_hConsoleOut;
	HANDLE	m_hConsoleIn;
	HANDLE	m_hProcess;
public:
	CRemoteConsole();
	~CRemoteConsole();
	bool GetHandles(const wchar_t* pClient);
	void Shutdown();
	void SetTitle(const wchar_t* pTitle);
	void SetInfo(int w, int h, int bh);
	void FillAttributes(WORD attributes);
	void Write(const wchar_t* pText, WORD Attributes = 0xFFFF);
	void WriteFmt(const wchar_t* pText, WORD Attributes, ...);
	//Attributes, fmt, arg count, args, attributes, fmt, ...
	void WriteMultiFmt(const wchar_t* pText, WORD Attributes, int count, ...);
	wchar_t ReadChar();
};

#endif //_CONSOLE_H_
