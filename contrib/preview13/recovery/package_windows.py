import pathlib,subprocess,shutil,re,zipfile,json,os
r=pathlib.Path('preview13-work').resolve();s=r/'CityEvolution-Preview-13-Source';out=r/'CityEvolution-Preview-13-Windows-x64';game=out/'Game';game.mkdir(parents=True,exist_ok=True)
subprocess.run(['cmake','--install',str(r/'win-build'),'--prefix',str(game)],check=True,stdout=open(r/'logs/windows-install.log','w'))
for directory in [r/'win-prefix/bin',r/'win-tools/usr/lib/gcc/x86_64-w64-mingw32/13-posix',r/'win-tools/usr/x86_64-w64-mingw32/lib']:
 for p in directory.glob('*.dll'):shutil.copy2(p,game/'bin'/p.name)
(out/'Settings').mkdir(exist_ok=True);(out/'PortableData').mkdir(exist_ok=True);shutil.copy2('portable-recovery/CityPreview/Settings/default.xml',out/'Settings/default.xml')
t=(s/'contrib/expanded-worlds/Play.cmd').read_text().replace('title LinCity-NG - Original Engine Portable','title City Evolution - Preview 13').replace('echo LinCity-NG exited','echo City Evolution exited').replace('" --config','" --sdl --config');(out/'Play.cmd').write_text(t,newline='\r\n')
(game/'diagnostics').mkdir(exist_ok=True)
for p in (r/'win-build/tests/city-evolution').glob('*-checks.exe'):shutil.copy2(p,game/'diagnostics'/p.name)
(out/'Verify-Simulation.cmd').write_text('@echo off\nsetlocal\nset "PATH=%~dp0Game\\bin;%PATH%"\ncd /d "%~dp0PortableData"\nfor %%T in (services community mayor transit city-evolution) do (\n "%~dp0Game\\diagnostics\\%%T-checks.exe" > "%%T-checks.log" 2>&1\n if errorlevel 1 (echo FAIL %%T. See PortableData\\%%T-checks.log & pause & exit /b 1)\n)\necho All simulation checks passed.\npause\n',newline='\r\n')
(out/'README.txt').write_text('City Evolution - Preview 13 (x64 Windows)\n\nExtract the whole ZIP to a writable folder. Run Play.cmd. Saves and settings remain in PortableData. Startup details: PortableData/launch.log.\n\nCity Hall has Budget / public services; F7 also opens it. Police and six later-game towers are in its second tab. The existing left build catalogue and audited menus remain intact.\n\nPollution illness, service funding, education, police and emergency dispatch are implemented. Zombie Easter egg defaults ON and has an OFF switch. Full contagious flu/COVID simulation is not included.\n\nThis is an experimental integration, not a fully balanced release. Cross-compiled and DLL imports checked; no real Windows runtime test was available. Run Verify-Simulation.cmd for local simulation checks.\n\nSee accompanying source, changelog and audit bundle. City Evolution is a fork of LinCity-NG. Licenses and original credits retained.\n')
# DLL closure audit. Windows system components are provided by Windows itself.
files=list((game/'bin').glob('*.dll'))+list((game/'bin').glob('*.exe'))+list((game/'diagnostics').glob('*.exe'));names={p.name.lower() for p in files};system={'kernel32.dll','user32.dll','gdi32.dll','advapi32.dll','shell32.dll','ole32.dll','oleaut32.dll','uuid.dll','comdlg32.dll','comctl32.dll','winmm.dll','version.dll','msvcrt.dll','ws2_32.dll','imm32.dll','setupapi.dll','cfgmgr32.dll','hid.dll','powrprof.dll','dwmapi.dll','uxtheme.dll','shlwapi.dll','secur32.dll','bcrypt.dll','ntdll.dll','rpcrt4.dll','dinput8.dll','dxgi.dll','d3d11.dll','d3d12.dll','dxguid.dll','avrt.dll','winspool.drv','msimg32.dll'}
objdump=r/'win-tools/usr/bin/x86_64-w64-mingw32-objdump';report={};missing=[]
for p in files:
 output=subprocess.check_output([str(objdump),'-p',str(p)],text=True);imports=re.findall(r'DLL Name:\s*(\S+)',output);report[str(p.relative_to(out))]=imports
 for name in imports:
  n=name.lower()
  if n not in names and n not in system and not n.startswith(('api-ms-','ext-ms-')):missing.append((p.name,name))
(s/'doc/audit/Windows-imports.json').write_text(json.dumps({'imports':report,'unresolved':missing,'runtime_tested':False},indent=2));assert not missing,missing
# License files accompany binaries. DLLs are built from the included dependency sources.
licenses=game/'share/licenses/dependencies';licenses.mkdir(parents=True,exist_ok=True)
for d in (r/'deps').iterdir():
 if not d.is_dir():continue
 for name in ['LICENSE.txt','LICENSE','COPYING','docs/LICENSE.TXT','docs/FTL.TXT','docs/GPLv2.TXT']:
  p=d/name
  if p.exists():shutil.copy2(p,licenses/(d.name+'-'+p.name))
for name in ['gcc-mingw-w64-base','mingw-w64-common','gcc-mingw-w64-x86-64-posix-runtime']:
 p=r/'win-tools/usr/share/doc'/name/'copyright'
 if p.exists():shutil.copy2(p,licenses/(name+'-copyright'))
with zipfile.ZipFile(r/(out.name+'.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in out.rglob('*'):
  if p.is_file():z.write(p,str(p.relative_to(out.parent)))
print('PASS Windows DLL closure;',len(files),'binaries; ZIP', (r/(out.name+'.zip')).stat().st_size)
