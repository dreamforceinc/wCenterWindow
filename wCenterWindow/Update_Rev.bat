@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET CURRENT_TIME=%TIME%
SET CURRENT_DATE=%DATE%
SET CURRENT_YEAR=%CURRENT_DATE:~10,4%

SET BUILDTIME=0
SET GIT_BRANCH=LOCAL
SET GIT_REV_COUNT=0
SET GIT_DATE=0
SET GIT_DATE_HUMAN=0

SET VerMajor=0
SET VerMinor=0
SET VerBuild=0

SET PN=0
SET VS=0
SET VSF=0

SET PCF=0
SET PYS=0
SET PA=0

SET INT_NAME=0

CD /d %~dp0

FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_MAJOR" "Version.h"') DO (SET "VerMajor=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_MINOR" "Version.h"') DO (SET "VerMinor=%%A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define V_BUILD" "Version.h"') DO (SET "VerBuild=%%A")

FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_NAME" "Version.h"') DO (SET "PN=%%~A")
FOR /F "tokens=3" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_YEAR_START" "Version.h"') DO (SET "PYS=%%A")
FOR /F "tokens=2*" %%A IN ('FINDSTR /I /L /C:"define PRODUCT_AUTHORS" "Version.h"') DO (SET "PA=%%~B")

FOR /F "tokens=1-4 delims=:.," %%A IN ("%CURRENT_TIME%") DO (
  SET /A "BUILDTIME=(100%%A %% 100) * 3600 + (100%%B %% 100) * 60 + (100%%C %% 100)"
)

FOR /F "delims=" %%A IN ('git symbolic-ref --short HEAD') DO (SET GIT_BRANCH=%%A)

FOR /F "delims=" %%A IN ('git rev-list --count HEAD') DO (
  SET /A GIT_REV_COUNT=%%A - 57
)

FOR /F "tokens=1,2 delims= " %%A IN ('git log -1 --date=format:%%y%%m%%d ^| find /I "Date:"') DO (
  SET GIT_DATE=%%B
)

FOR /F "tokens=1,2 delims= " %%A IN ('git log -1 --date=format:%%d.%%m.%%Y ^| find /I "Date:"') DO (
  SET GIT_DATE_HUMAN=%%B
)

SET VSF=%VerMajor%.%VerMinor%.%GIT_DATE%.%BUILDTIME%
SET VS=%VerMajor%.%VerMinor%.%VerBuild%
SET VNF=%VerMajor%,%VerMinor%,%GIT_DATE%,%BUILDTIME%
SET VN=%VerMajor%,%VerMinor%,%VerBuild%

SET PNF="%PN% v%VS% (C++)"
SET PCF="Copyright (C) %PYS%-%CURRENT_YEAR% by %PA%"

SET INT_NAME="%PN%C++"
SET ORIG_NAME="%PN%.exe"

ECHO #pragma once > revision.h
ECHO #define GIT_DATE_HUMAN "%GIT_DATE_HUMAN%" >> revision.h
ECHO #define GIT_DATE "%GIT_DATE%" >> revision.h
ECHO #define GIT_BRANCH %GIT_BRANCH% >> revision.h
ECHO #define GIT_COUNT %GIT_REV_COUNT% >> revision.h
ECHO #define V_SECS %BUILDTIME% >> revision.h
ECHO #define INTERNAL_NAME %INT_NAME% >> revision.h
ECHO #define ORIG_FILE_NAME %ORIG_NAME% >> revision.h
ECHO #define PRODUCT_NAME_FULL %PNF% >> revision.h
ECHO #define PRODUCT_COPYRIGHT %PCF% >> revision.h
ECHO #define VERSION_NUM %VN% >> revision.h
ECHO #define VERSION_STR "%VS%" >> revision.h
ECHO #define VERSION_NUM_FULL %VNF% >> revision.h
ECHO #define VERSION_STR_FULL "%VSF%" >> revision.h

ENDLOCAL
EXIT
