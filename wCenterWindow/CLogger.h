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
// CLogger.h

#pragma once
#include <Windows.h>
#include <fstream>

#define MAX_LOGBUFFER_LENGTH 512

class CLogger {
public:
    void Out(const wchar_t*, ...);
    CLogger(const wchar_t*);
    ~CLogger();

private:
    SYSTEMTIME lt;
    CRITICAL_SECTION cs;
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
