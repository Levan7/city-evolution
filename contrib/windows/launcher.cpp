// SPDX-License-Identifier: GPL-2.0-or-later
// Native portable launcher; no command shell or script interpreter.
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <filesystem>
#include <string>
static std::wstring quote(const std::filesystem::path&p){return L"\""+p.wstring()+L"\"";}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){try{
 wchar_t path[32768];DWORD length=GetModuleFileNameW(nullptr,path,32768);if(!length||length>=32768)return 1;
 auto root=std::filesystem::path(path).parent_path(),data=root/L"PortableData",exe=root/L"Game/bin/lincity-ng.exe",config=data/L"config.xml";
 std::filesystem::create_directories(data);
 if(!std::filesystem::exists(config)&&std::filesystem::exists(root/L"Settings/default.xml"))std::filesystem::copy_file(root/L"Settings/default.xml",config);
 SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE log=CreateFileW((data/L"launch.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
 if(log==INVALID_HANDLE_VALUE||input==INVALID_HANDLE_VALUE){MessageBoxW(nullptr,L"Cannot create PortableData/launch.log. Extract the entire game to a writable folder.",L"City Evolution",MB_OK|MB_ICONERROR);return 1;}
 STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdOutput=log;si.hStdError=log;si.hStdInput=input;PROCESS_INFORMATION pi{};
 std::wstring command=quote(exe)+L" --sdl --app-data "+quote(root/L"Game/share/lincity-ng")+L" --user-data "+quote(data)+L" --config "+quote(config);
 BOOL ok=CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,root.c_str(),&si,&pi);DWORD error=GetLastError();CloseHandle(input);CloseHandle(log);
 if(!ok){std::wstring message=L"Could not start City Evolution (Windows error "+std::to_wstring(error)+L"). Extract the entire ZIP, including the Game folder.";MessageBoxW(nullptr,message.c_str(),L"City Evolution",MB_OK|MB_ICONERROR);return 1;}
 CloseHandle(pi.hThread);WaitForSingleObject(pi.hProcess,INFINITE);DWORD result=1;GetExitCodeProcess(pi.hProcess,&result);CloseHandle(pi.hProcess);
 if(result){std::wstring message=L"City Evolution stopped unexpectedly. Startup details are in "+(data/L"launch.log").wstring();MessageBoxW(nullptr,message.c_str(),L"City Evolution",MB_OK|MB_ICONERROR);}return int(result);
}catch(...){MessageBoxW(nullptr,L"Could not prepare the portable game folder. Extract the whole ZIP to a writable folder.",L"City Evolution",MB_OK|MB_ICONERROR);return 1;}}
