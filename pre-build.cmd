@ECHO OFF

:: MIT License
:: 
:: Copyright (c) 2023 W0LF aka 'dreamforce'
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to deal
:: in the Software without restriction, including without limitation the rights
:: to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
:: copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in all
:: copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
:: OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
:: SOFTWARE.

:: In VisualStudio add to Pre-Build events:
:: "$(SolutionDir)pre-build.cmd" "$(SolutionDir)" "$(ProjectDir)"
:: Then in "Resource Includes..." add '#include "VersionInfo.rc"' into 'Compile-time Directives'

IF "%~1" == "" GOTO :no_args
IF "%~2" == "" GOTO :no_args
SET solutionDir=%1
SET projectDir=%2

CD /D %solutionDir%
powershell -ExecutionPolicy RemoteSigned -File Update_Version.ps1
MOVE /Y %solutionDir%VersionInfo.h %projectDir%VersionInfo.h
EXIT

:no_args
ECHO Not enough arguments!
EXIT 1
