@ECHO OFF
CHCP 1251 >nul
SETLOCAL ENABLEDELAYEDEXPANSION

SET CURRENT_TIME=%TIME%
SET CURRENT_DATE=%DATE%

SET BUILDTIME=%CURRENT_TIME:~0,8%
SET BUILDDATE=%CURRENT_DATE%
SET BUILD_DATETIME=Build time: %BUILDDATE% %BUILDTIME%
SET CURRENT_YEAR=%CURRENT_DATE:~6,4%
SET BUILDSECS=0

SET GIT_COUNT=0
SET GIT_TIME=0
SET GIT_DATE=0
SET GIT_DATETIME=0

SET VerMajor=0
SET VerMinor=0
SET VerPatch=0

SET INT_NAME=0
SET PN=0
SET VS=0
SET VSF=0
SET PCF=0
SET PYS=0
SET PA=0

CD /D %~dp0
IF NOT EXIST "Version.h" (
	ECHO Can't find file 'Version.h'
	TIMEOUT /T 3
	EXIT /B 1
)
COPY /Y "Version.h" "VersionInfo.h" >nul

FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_MAJOR" "VersionInfo.h"') DO (SET "VerMajor=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_MINOR" "VersionInfo.h"') DO (SET "VerMinor=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_PATCH" "VersionInfo.h"') DO (SET "VerPatch=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_NAME" "VersionInfo.h"') DO (SET "PN=%%~A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_YEAR_START" "VersionInfo.h"') DO (SET "PYS=%%A")
FOR /F "tokens=2*" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_AUTHORS" "VersionInfo.h"') DO (SET "PA=%%~B")
FOR /F "tokens=1-4 delims=:., " %%A IN ("%BUILDTIME%") DO (SET /A "BUILDSECS=%%A * 3600 + %%B * 60 + %%C")

IF EXIST ".git" (
	FOR /F "delims=" %%A IN ('git rev-list --count HEAD') DO (SET /A GIT_COUNT=%%A)
	FOR /F "tokens=1,2 delims= " %%A IN ('git log -1 --date=format:%%d.%%m.%%Y ^| find /I "Date:"') DO (SET "GIT_DATE=%%B")
	FOR /F "tokens=2-4 delims=, " %%A IN ('git log -1 --date=format:"%%a,%%d-%%h-%%Y,%%T" ^| find /I "Date:"') DO (
		SET "GIT_DATETIME=Git time: %%A, %%B %%C"
		SET "GIT_TIME=%%C"
	)
)

SET VSF=%VerMajor%.%VerMinor%.%VerPatch%.%GIT_COUNT%
SET VS=%VerMajor%.%VerMinor%.%VerPatch%
SET VNF=%VerMajor%,%VerMinor%,%VerPatch%,%GIT_COUNT%
SET VN=%VerMajor%,%VerMinor%,%VerPatch%

SET PNF=%PN% v%VS% (C++)
SET PCF=Copyright (C) %PYS%-%CURRENT_YEAR% by %PA%

SET INT_NAME=%PN%C++
SET ORIG_NAME=%PN%.exe

ECHO #define BUILD_DATE "%BUILDDATE%">> VersionInfo.h
ECHO #define BUILD_TIME "%BUILDTIME%">> VersionInfo.h
ECHO #define BUILD_DATETIME "%BUILD_DATETIME%">> VersionInfo.h

IF EXIST ".git" (
	ECHO #define GIT_DATE "%GIT_DATE%">> VersionInfo.h
	ECHO #define GIT_TIME "%GIT_TIME%">> VersionInfo.h
	ECHO #define GIT_DATETIME "%GIT_DATETIME%">> VersionInfo.h
	ECHO #define GIT_COUNT %GIT_COUNT% >> VersionInfo.h
)

ECHO #define V_SECS %BUILDSECS% >> VersionInfo.h
ECHO #define INTERNAL_NAME "%INT_NAME%">> VersionInfo.h
ECHO #define ORIG_FILE_NAME "%ORIG_NAME%">> VersionInfo.h
ECHO #define PRODUCT_NAME_FULL "%PNF%">> VersionInfo.h
ECHO #define PRODUCT_COPYRIGHT "%PCF%">> VersionInfo.h
ECHO #define VERSION_NUM %VN% >> VersionInfo.h
ECHO #define VERSION_STR "%VS%">> VersionInfo.h
ECHO #define VERSION_NUM_FULL %VNF% >> VersionInfo.h
ECHO #define VERSION_STR_FULL "%VSF%">> VersionInfo.h

ENDLOCAL
TIMEOUT /T 1 >nul
EXIT
