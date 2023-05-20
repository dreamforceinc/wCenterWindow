// wCenterWindow
// logger.cpp
//
#include "wCenterWindow.h"
#include "globals.h"
#include "logger.h"

SYSTEMTIME lt;
wchar_t debugTimeBuffer[TBUFLEN];
wchar_t debugBuffer[DBUFLEN];
std::wofstream logfile;

wchar_t* GetTimeStamp()
{
	GetLocalTime(&lt);
	StringCchPrintfW(debugTimeBuffer, TBUFLEN, L"%d-%02d-%02d %02d:%02d:%02d.%03d | ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
	return debugTimeBuffer;
}

void OpenLogFile(const wchar_t appTitle[], const wchar_t appVersion[])
{
	WCHAR appTitleVer[MAX_LOADSTRING]{ 0 };
	StringCchPrintfW(appTitleVer, MAX_LOADSTRING, L"%s v%s", appTitle, appVersion);
	WCHAR lpszPath[MAX_PATH + 1] = { 0 };
	DWORD dwPathLength = GetModuleFileNameW(NULL, lpszPath, MAX_PATH);
	DWORD dwError = GetLastError();
	if (ERROR_INSUFFICIENT_BUFFER == dwError)
	{
		MessageBoxW(NULL, L"Path to logfile is too long! Working without logging", appTitle, MB_OK | MB_ICONWARNING);
		return;
	}
	if (NULL == dwPathLength)
	{
		MessageBoxW(NULL, L"Can't get module filename! Working without logging", appTitle, MB_OK | MB_ICONWARNING);
		return;
	}

	std::filesystem::path log_path = lpszPath;
	log_path.replace_extension(L".log");
	std::filesystem::path bak_path = log_path;
	bak_path.replace_extension(L".bak");

	if (std::filesystem::exists(log_path)) std::filesystem::rename(log_path, bak_path);
#ifdef _DEBUG
	log_path = L"D:\\test.log";
#endif
	logfile.open(log_path, std::ios::trunc);
	if (logfile.is_open())
	{
		logfile << "\xEF\xBB\xBF";			// (0xEF, 0xBB, 0xBF) - UTF-8 BOM
		logfile.imbue(std::locale("en-US.utf8"));
		logfile << GetTimeStamp() << "[ " << appTitleVer << " ] Start log." << std::endl;
		logfile << GetTimeStamp() << "Logfile: \"" << log_path.native() << "\"" << std::endl;
	}
	else
	{
		MessageBoxW(NULL, L"Can't open logfile! Working without logging", appTitle, MB_OK | MB_ICONWARNING);
	}
	return;
}

void CloseLogFile()
{
	if (logfile)
	{
		logfile << GetTimeStamp() << "Stop log." << std::endl;
		logfile.close();
	}
}
