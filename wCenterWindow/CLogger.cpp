// MIT License
// 
// Copyright (c) 2023 W0LF aka 'dreamforce'
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// wCenterWindow
// CLogger.cpp

#include "CLogger.h"
#include <filesystem>
#include <strsafe.h>

inline wchar_t* CLogger::GetTimeStamp() {
	GetLocalTime(&lt);
	StringCchPrintfW(logTimeBuffer, _countof(logTimeBuffer), L"%d-%02d-%02d %02d:%02d:%02d.%03d | ", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
	return logTimeBuffer;
}

void CLogger::Out(const wchar_t* fmt, ...) {
	if (fsLogFile.is_open()) {
		va_list args;
		va_start(args, fmt);
		EnterCriticalSection(&cs);
		StringCchVPrintfW(logBuffer, _countof(logBuffer), fmt, args);
		va_end(args);
		fsLogFile << GetTimeStamp() << logBuffer << std::endl;
		LeaveCriticalSection(&cs);
	}
}

void CLogger::Init() {
	InitializeCriticalSection(&cs);
	wchar_t szPath[MAX_PATH] = { 0 };
	DWORD dwPathLength = GetModuleFileNameW(NULL, szPath, MAX_PATH);
	DWORD dwError = GetLastError();
	if (ERROR_INSUFFICIENT_BUFFER == dwError) {
		MessageBoxW(NULL, L"Warning!\nPath to log file is too long! Working without logging.", szAppTitle.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}
	if (NULL == dwPathLength) {
		MessageBoxW(NULL, L"Warning!\nCan't get application's filename! Working without logging.", szAppTitle.c_str(), MB_OK | MB_ICONWARNING);
		return;
	}

	std::filesystem::path log_path = szPath;
	log_path.replace_extension(L".log");
	std::filesystem::path bak_path = log_path;
	bak_path.replace_extension(L".bak");

	if (std::filesystem::exists(log_path)) std::filesystem::rename(log_path, bak_path);
#ifdef _DEBUG
	log_path = L"D:\\test.log";
#endif
	fsLogFile.open(log_path, std::ios::trunc);
	if (fsLogFile.is_open()) {
		fsLogFile << "\xEF\xBB\xBF";																// (0xEF, 0xBB, 0xBF) - UTF-8 BOM
		fsLogFile.imbue(std::locale("en-US.utf8"));
		fsLogFile << GetTimeStamp() << "[ " << szAppTitleVer.c_str() << " ] Start log." << std::endl;
		fsLogFile << GetTimeStamp() << "Logfile: \"" << log_path.native() << "\"" << std::endl;
	}
	else {
		MessageBoxW(NULL, L"Warning!\nCan't create log file! Working without logging.", szAppTitle.c_str(), MB_OK | MB_ICONWARNING);
	}
}

CLogger::CLogger(const wchar_t* _appTitle) {
	szAppTitle = _appTitle;
	szAppTitleVer = _appTitle;
	Init();
}

CLogger::CLogger(const wchar_t* _appTitle, const wchar_t* _appVersion) {
	szAppTitle = _appTitle; szAppVersion = _appVersion;
	szAppTitleVer = _appTitle;
	szAppTitleVer.append(L", v").append(_appVersion);
	Init();
}

CLogger::~CLogger() {
	if (fsLogFile) {
		fsLogFile << GetTimeStamp() << "Stop log." << std::endl;
		fsLogFile.close();
		DeleteCriticalSection(&cs);
	}
}
