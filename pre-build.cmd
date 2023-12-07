@ECHO OFF

REM In VisualStudio add to Pre-Build events:
REM "$(SolutionDir)pre-build.cmd" "$(SolutionDir)" "$(ProjectDir)" "$(PlatformTarget)"
REM Then in "Resource Includes..." add '#include "VersionInfo.rc"' into 'Compile-time Directives'

IF "%~1" == "" GOTO :no_args
IF "%~2" == "" GOTO :no_args
IF "%~3" == "" GOTO :no_args
SET solutionDir=%1
SET projectDir=%2
SET platformArch=%3

CD /D %solutionDir%
powershell -ExecutionPolicy RemoteSigned -File Update_Version.ps1 %platformArch%
MOVE /Y %solutionDir%VersionInfo.h %projectDir%VersionInfo.h
EXIT

:no_args
ECHO Not enough arguments!
EXIT 1
