// wCenterWindow
// Logger.cpp
//
#include "framework.h"

#define TS_LEN 30

std::ofstream logfile;
extern WCHAR szTitle[];
extern LPVOID szBuffer;

std::string GetTimeStamp()
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	CHAR ts[TS_LEN];
	StringCchPrintfA(ts, TS_LEN, "%d-%02d-%02d %02d:%02d:%02d.%03d - ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
	return ts;
}

void OpenLogFile()
{
	WCHAR lpszPath[MAX_PATH + 1] = { 0 };
	DWORD dwPathLength = GetModuleFileNameW(NULL, lpszPath, MAX_PATH);
	DWORD dwError = GetLastError();
	if (ERROR_INSUFFICIENT_BUFFER == dwError)
	{
		MessageBoxW(NULL, L"Path to logfile is too long! Working without logging", (LPCWSTR)szTitle, MB_OK | MB_ICONWARNING);
		return;
	}
	if (NULL == dwPathLength)
	{
		MessageBoxW(NULL, L"Can't get module filename! Working without logging", (LPCWSTR)szTitle, MB_OK | MB_ICONWARNING);
		return;
	}

	std::filesystem::path log_path = lpszPath;
	log_path.replace_extension(L".log");
	std::filesystem::path bak_path = log_path;
	bak_path.replace_extension(L".bak");

	if (std::filesystem::exists(log_path)) std::filesystem::rename(log_path, bak_path);

#ifdef _DEBUG
	log_path = L"d:\\test.log";
#endif

	logfile.open(log_path);
	if (logfile.is_open())
	{
		logfile << std::boolalpha;
		diag_log("Start logging");
		diag_log("Logfile: ", log_path);
		diag_log("Logfile was successfully opened");
	}
	else
	{
		MessageBoxW(NULL, L"Can't open logfile! Working without logging", (LPCWSTR)szTitle, MB_OK | MB_ICONWARNING);
	}
	return;
}

void CloseLogFile()
{
	if (logfile)
	{
		diag_log("End logging");
		logfile.close();
	}
}
