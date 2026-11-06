@echo off
setlocal EnableExtensions DisableDelayedExpansion
title LinCity-NG - Original Engine Portable
set "CITY_ROOT=%~dp0"
if not exist "%CITY_ROOT%Game\bin\lincity-ng.exe" goto missing
if not exist "%CITY_ROOT%Game\share\lincity-ng\gui\mainmenu.xml" goto missing
if not exist "%CITY_ROOT%PortableData" mkdir "%CITY_ROOT%PortableData"
if not exist "%CITY_ROOT%PortableData" goto readonly
>"%CITY_ROOT%PortableData\.write-test.tmp" echo portable
if errorlevel 1 goto readonly
del /q "%CITY_ROOT%PortableData\.write-test.tmp" >nul 2>&1
if not exist "%CITY_ROOT%PortableData\config.xml" copy /y "%CITY_ROOT%Settings\default.xml" "%CITY_ROOT%PortableData\config.xml" >nul
if not exist "%CITY_ROOT%PortableData\config.xml" goto readonly
pushd "%CITY_ROOT%Game\bin"
if errorlevel 1 goto missing
"%CITY_ROOT%Game\bin\lincity-ng.exe" --config "%CITY_ROOT%PortableData\config.xml" --user-data "%CITY_ROOT%PortableData" --app-data "%CITY_ROOT%Game\share\lincity-ng" %* >"%CITY_ROOT%PortableData\launch.log" 2>&1
set "CITY_EXIT=%ERRORLEVEL%"
popd
if not "%CITY_EXIT%"=="0" goto gamefailed
endlocal & exit /b 0
:missing
echo Game files are missing. Extract the entire ZIP before starting Play.cmd.
pause
endlocal & exit /b 2
:readonly
echo This folder cannot store saves or settings.
echo Extract the game to a writable folder, then start Play.cmd again.
pause
endlocal & exit /b 3
:gamefailed
echo LinCity-NG exited with code %CITY_EXIT%.
echo Details are in PortableData\launch.log beside Play.cmd.
pause
endlocal & exit /b %CITY_EXIT%
