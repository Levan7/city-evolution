@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "CITY_ROOT=%~dp0"
if not exist "%CITY_ROOT%PortableData" mkdir "%CITY_ROOT%PortableData"
echo Checking generated worlds, flat terrain, save/load and simulation...
echo Results will be written to PortableData\world-verification.log.
"%CITY_ROOT%Game\bin\lincity-ng.exe" --verify-worlds "%CITY_ROOT%PortableData\WorldVerification" >"%CITY_ROOT%PortableData\world-verification.log" 2>&1
set "CITY_EXIT=%ERRORLEVEL%"
if "%CITY_EXIT%"=="0" (echo All world verification checks passed.) else (echo Verification failed. See PortableData\world-verification.log.)
pause
endlocal & exit /b %CITY_EXIT%
