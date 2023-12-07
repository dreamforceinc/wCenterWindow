// wCenterWindow
// CLogger.h
#pragma once
#include <Windows.h>
#include <fstream>

#define MAX_LOGBUFFER_LENGTH 512

class CLogger
{
public:
	void Out(const wchar_t*, ...);
	CLogger(const wchar_t*);
	~CLogger();

private:
	SYSTEMTIME lt;
	CRITICAL_SECTION cs;
	HANDLE hLoggerThread = NULL;
	HANDLE hLoggerEvent = NULL;
	wchar_t logTimeBuffer[28]{ 0 };
	wchar_t logBuffer[MAX_LOGBUFFER_LENGTH]{ 0 };
	std::wofstream fsLogFile;
	std::wstring szAppTitle{ 0 };
	std::wstring szAppVersion{ 0 };
	std::wstring szAppPlatform{ 0 };
	std::wstring szAppTitleVer{ 0 };

	inline wchar_t* GetTimeStamp();
	void Init();
};
