@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
SET CURRENT_PATH=%~dp0

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
SET GIT_DATE_NUM=0
SET GIT_DATETIME=0

SET VerMajor=0
SET VerMinor=0
SET VerBuild=0

SET INT_NAME=0

SET PN=0
SET VS=0
SET VSF=0

SET PCF=0
SET PYS=0
SET PA=0

CD /D "%CURRENT_PATH%"
IF NOT EXIST "VersionInfo.h" (
	ECHO Can't find file 'VersionInfo.h'
	TIMEOUT /T 1
	EXIT /B 1
)
COPY /Y "VersionInfo.h" "version.h" >nul

FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_MAJOR" "version.h"') DO (SET "VerMajor=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_MINOR" "version.h"') DO (SET "VerMinor=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_BUILD" "version.h"') DO (SET "VerBuild=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_NAME" "version.h"') DO (SET "PN=%%~A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_YEAR_START" "version.h"') DO (SET "PYS=%%A")
FOR /F "tokens=2*" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_AUTHORS" "version.h"') DO (SET "PA=%%~B")
FOR /F "tokens=1-4 delims=:., " %%A IN ("%BUILDTIME%") DO (SET /A "BUILDSECS=%%A * 3600 + %%B * 60 + %%C")
FOR /F "delims=" %%A IN ('git rev-list --count HEAD') DO (SET /A GIT_COUNT=%%A)
::FOR /F "tokens=1,2 delims= " %%A IN ('git log -1 --date=format:%%y%%m%%d ^| find /I "Date:"') DO (SET GIT_DATE_NUM=%%B)
FOR /F "tokens=1,2 delims= " %%A IN ('git log -1 --date=format:%%d.%%m.%%Y ^| find /I "Date:"') DO (SET "GIT_DATE=%%B")
FOR /F "tokens=2-4 delims=, " %%A IN ('git log -1 --date=format:"%%a,%%d-%%h-%%Y,%%T" ^| find /I "Date:"') DO (
  SET "GIT_DATETIME=Git time: %%A, %%B %%C"
  SET "GIT_TIME=%%C"
)

SET VSF=%VerMajor%.%VerMinor%.%VerBuild%.%GIT_COUNT%
SET VS=%VerMajor%.%VerMinor%.%VerBuild%
SET VNF=%VerMajor%,%VerMinor%,%VerBuild%,%GIT_COUNT%
SET VN=%VerMajor%,%VerMinor%,%VerBuild%

SET PNF=%PN% v%VS% (C++)
SET PCF=Copyright (C) %PYS%-%CURRENT_YEAR% by %PA%

SET INT_NAME=%PN%C++
SET ORIG_NAME=%PN%.exe

ECHO #define BUILD_DATE "%BUILDDATE%">> version.h
ECHO #define BUILD_TIME "%BUILDTIME%">> version.h
ECHO #define BUILD_DATETIME "%BUILD_DATETIME%">> version.h
ECHO #define GIT_DATE "%GIT_DATE%">> version.h
ECHO #define GIT_TIME "%GIT_TIME%">> version.h
ECHO #define GIT_DATETIME "%GIT_DATETIME%">> version.h
ECHO #define GIT_COUNT %GIT_COUNT% >> version.h
ECHO #define V_SECS %BUILDSECS% >> version.h
ECHO #define INTERNAL_NAME "%INT_NAME%">> version.h
ECHO #define ORIG_FILE_NAME "%ORIG_NAME%">> version.h
ECHO #define PRODUCT_NAME_FULL "%PNF%">> version.h
ECHO #define PRODUCT_COPYRIGHT "%PCF%">> version.h
ECHO #define VERSION_NUM %VN% >> version.h
ECHO #define VERSION_STR "%VS%">> version.h
ECHO #define VERSION_NUM_FULL %VNF% >> version.h
ECHO #define VERSION_STR_FULL "%VSF%">> version.h

ENDLOCAL
EXIT
