// wCenterWindow
// globals.h
//
#pragma once
#include "wCenterWindow.h"

#define MAX_LOADSTRING 50
#define TBUFLEN 32
#define DBUFLEN 256

extern WCHAR szTitle[];
extern SYSTEMTIME lt;
extern wchar_t debugBuffer[DBUFLEN];
extern std::wofstream logfile;
extern CRITICAL_SECTION cs;
