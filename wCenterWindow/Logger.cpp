// Logger.cpp
//
#include <fstream>
#include <filesystem>
#include <string>
#include <strsafe.h>
#include <Windows.h>
#include "Logger.h"

#define TS_LEN 30
#define PATH_LEN 1024

std::wofstream logfile;
extern LPVOID szBuffer;
namespace fs = std::filesystem;

std::wstring PrintTitle() {
	wchar_t szWinTitle[2048];
	StringCchPrintf(szWinTitle, 2048, L"%s", szBuffer);
	return szWinTitle;
}

std::wstring GetTimeStamp() {
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	wchar_t ts[TS_LEN];
	StringCchPrintf(ts, TS_LEN, L"%d-%02d-%02d %02d:%02d:%02d.%03d - ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
	return ts;
}

void OpenLogFile() {
	wchar_t lpszPath[PATH_LEN]{};
	DWORD dwPathLength = GetModuleFileName(NULL, lpszPath, PATH_LEN);
	DWORD dwError = GetLastError();
	if (ERROR_INSUFFICIENT_BUFFER == dwError) {
		MessageBoxW(NULL, L"Path to logfile is too long! Working without logging.", L"WARNING", MB_OK | MB_ICONWARNING);
		return;
	}
	if (NULL == dwPathLength) {
		MessageBoxW(NULL, L"Can't get module filename! Working without logging.", L"WARNING", MB_OK | MB_ICONWARNING);
		return;
	}

	fs::path log_path = lpszPath;
	log_path.replace_extension(L".log");
	fs::path bak_path = log_path;
	bak_path.replace_extension(L".bak");

	if (fs::exists(log_path)) fs::rename(log_path, bak_path);
#ifdef _DEBUG
	log_path = L"d:\\test.log";
#endif
	logfile.open(log_path);
	if (logfile.is_open()) {
		diag_log(L"Starting logging.");
		diag_log(L"logfile:", log_path);
		diag_log(log_path, L"successfully opened.");
	} else {
		MessageBoxW(NULL, L"Can't open logfile! Working without logging.", L"WARNING", MB_OK | MB_ICONWARNING);
	}
	return;
}

void CloseLogFile() {
	if (logfile) {
		diag_log(L"Ending logging.");
		logfile.close();
	}
}
