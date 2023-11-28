// wCenterWindow
// wCenterWindow.h
//
#pragma once
#include "resource.h"

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
