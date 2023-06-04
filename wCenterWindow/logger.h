// wCenterWindow
// logger.h
// wLogger v3.3 (Edited version from RBTray project [https://github.com/benbuck/rbtray])
// 
// Usage: LOG_TO_FILE(L"%s(%d): Log message", TEXT(__FUNCTION__), __LINE__);
//
#pragma once
#define LOG_TO_FILE(fmt, ...) do {\
	EnterCriticalSection(&cs); \
	StringCchPrintfW(debugBuffer, DBUFLEN, fmt, ##__VA_ARGS__); \
	logfile << GetTimeStamp() << debugBuffer << std::endl; \
	LeaveCriticalSection(&cs); \
} while (0);

wchar_t* GetTimeStamp();
void OpenLogFile(const wchar_t[], const wchar_t[]);
void CloseLogFile();
