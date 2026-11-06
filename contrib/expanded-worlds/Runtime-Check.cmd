@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "CITY_ROOT=%~dp0"
if not exist "%CITY_ROOT%PortableData" mkdir "%CITY_ROOT%PortableData"
if not exist "%CITY_ROOT%PortableData\config.xml" copy /y "%CITY_ROOT%Settings\default.xml" "%CITY_ROOT%PortableData\config.xml" >nul
"%CITY_ROOT%Game\bin\lincity-ng.exe" --version --config "%CITY_ROOT%PortableData\config.xml" --user-data "%CITY_ROOT%PortableData" --app-data "%CITY_ROOT%Game\share\lincity-ng" >"%CITY_ROOT%PortableData\runtime-check.log" 2>&1
set "CITY_EXIT=%ERRORLEVEL%"
echo Runtime/configuration check exit code: %CITY_EXIT%
echo Log: PortableData\runtime-check.log
pause
endlocal & exit /b %CITY_EXIT%
