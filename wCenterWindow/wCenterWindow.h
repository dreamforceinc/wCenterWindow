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
// wCenterWindow.h

#pragma once
#include "resource.h"

#define T0 10
#define T1 60
#define T2 86400

#ifdef _WIN64
#define ARCH 64
#else
#define ARCH 86
#endif

#define MAX_LOADSTRING 50

// Windows Header Files
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <strsafe.h>
#include <windows.h>
#include <wininet.h>
#include <shellapi.h>
#include <CommCtrl.h>
#include <process.h>

// Logger header file
#include "CLogger.h"

// VerionInfo header file
#include "VersionInfo.h"

// wCenterWindow's title
extern WCHAR szTitle[MAX_LOADSTRING];

// An instance of the "CLogger" class
extern CLogger logger;
