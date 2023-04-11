// wCenterWindow
// framework.h
//
#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <fstream>
#include <filesystem>
#include <string>
#include <strsafe.h>
#include <windows.h>
#include <shellapi.h>
#include <CommCtrl.h>

// Project Specific Header Files
#include "logger.h"
#include "VersionInfo.h"

// Extern variables
#define MAX_LOADSTRING 50
WCHAR szTitle[MAX_LOADSTRING];          // wCenterWindow's title
