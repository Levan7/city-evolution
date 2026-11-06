from pathlib import Path
import json,shutil,subprocess,re,zipfile
r=Path('preview15-work').resolve();s=r/'CityEvolution-Preview-15-Source';out=r/'CityEvolution-Preview-15-Windows-x64';game=out/'Game';game.mkdir(parents=True,exist_ok=True)
subprocess.run(['cmake','--install',str(r/'win-build'),'--prefix',str(game)],check=True,stdout=open(r/'logs/windows-install.log','w'))
for d in [r/'win-prefix/bin',r/'win-tools/usr/lib/gcc/x86_64-w64-mingw32/13-posix',r/'win-tools/usr/x86_64-w64-mingw32/lib']:
 for p in d.glob('*.dll'):shutil.copy2(p,game/'bin'/p.name)
(out/'Settings').mkdir(exist_ok=True);(out/'PortableData').mkdir(exist_ok=True);shutil.copy2('portable-recovery/CityPreview/Settings/default.xml',out/'Settings/default.xml');shutil.copy2(r/'City Evolution.exe',out/'City Evolution.exe')
(out/'Play.cmd').unlink(missing_ok=True)
(game/'diagnostics').mkdir(exist_ok=True)
for p in (r/'win-build/tests/city-evolution').glob('*-checks.exe'):shutil.copy2(p,game/'diagnostics'/p.name)
shutil.copytree(s/'tests/audio-fixture',game/'diagnostics/audio-fixture',dirs_exist_ok=True)
(out/'Verify-Simulation.cmd').write_text('@echo off\nsetlocal\nset "PATH=%~dp0Game\\bin;%PATH%"\ncd /d "%~dp0PortableData"\nfor %%T in (flu services community mayor transit city-evolution autonomy street-routes) do (\n "%~dp0Game\\diagnostics\\%%T-checks.exe" > "%%T-checks.log" 2>&1\n if errorlevel 1 (echo FAIL %%T. See PortableData\\%%T-checks.log & pause & exit /b 1)\n)\necho All simulation checks passed.\npause\n',newline='\r\n')
(out/'Verify-Audio.cmd').write_text('@echo off\nsetlocal\nset "PATH=%~dp0Game\\bin;%PATH%"\nset "SDL_AUDIO_DRIVER=dummy"\n"%~dp0Game\\diagnostics\\audio-checks.exe" "%~dp0Game\\diagnostics\\audio-fixture" working\nif errorlevel 1 (pause & exit /b 1)\nset "SDL_AUDIO_DRIVER=nonexistent"\n"%~dp0Game\\diagnostics\\audio-checks.exe" "%~dp0Game\\diagnostics\\audio-fixture" missing\npause\n',newline='\r\n')
(out/'README.txt').write_text('''City Evolution - Preview 15 (64-bit Windows)
By Levan Chelidze | studio76@mail.com
A fork of LinCity-NG. Original contributors and licenses are retained.

Extract the ENTIRE ZIP to a writable folder. Double-click City Evolution.exe.
Keep its Game, Settings and PortableData folders beside it.
The native launcher has the game icon and writes PortableData/launch.log.
Saves and configuration remain in PortableData. Back up old saves first.

Full autonomy: open Mayor or City Hall > Budget / public services (F7).
Select the whole city, a district or a drawn zone, then enable Full autonomy.
It builds and connects affordable projects with actual cash and technology.
It does not grant free money or residents. OFF stops building.
An empty city needs startup funds; tiny or inaccessible zones can prevent growth.

Police are under Services. Residential towers are under Homes; offices under
Industry. Locked technology stays grey. Building visibility remains in the
lower map controls; the duplicate top-right button has been removed.

Pedestrians and horse/car traffic now follow destination routes and leave
when their trip finishes. Damaged streets trigger replanning. These are visual
street actors; they are not a full individual commuter or cargo simulation.

Flu, women pedestrians, inventory props, activity frames, night lighting,
service dispatch and the previous audio/scaling repairs remain included.

Native Linux regression and graphical checks passed. Windows was cross-built
and every DLL import checked; this release was not run on physical Windows.
Run Verify-Simulation.cmd or Verify-Audio.cmd for local diagnostics.
See the source, changelog and audit bundle for details and limits.
''')
files=list((game/'bin').glob('*.dll'))+list((game/'bin').glob('*.exe'))+list((game/'diagnostics').glob('*.exe'))+[out/'City Evolution.exe'];names={p.name.lower() for p in files};system={'kernel32.dll','user32.dll','gdi32.dll','advapi32.dll','shell32.dll','ole32.dll','oleaut32.dll','uuid.dll','comdlg32.dll','comctl32.dll','winmm.dll','version.dll','msvcrt.dll','ws2_32.dll','imm32.dll','setupapi.dll','cfgmgr32.dll','hid.dll','powrprof.dll','dwmapi.dll','uxtheme.dll','shlwapi.dll','secur32.dll','bcrypt.dll','ntdll.dll','rpcrt4.dll','dinput8.dll','dxgi.dll','d3d11.dll','d3d12.dll','dxguid.dll','avrt.dll','winspool.drv','msimg32.dll'}
report={};missing=[];objdump=r/'win-tools/usr/bin/x86_64-w64-mingw32-objdump'
for p in files:
 t=subprocess.check_output([str(objdump),'-p',str(p)],text=True);imports=re.findall(r'DLL Name:\s*(\S+)',t);report[str(p.relative_to(out))]=imports
 for name in imports:
  if name.lower() not in names|system and not name.lower().startswith(('api-ms-','ext-ms-')):missing.append((p.name,name))
 if p.name=='City Evolution.exe':
  assert 'Windows GUI' in t,'launcher is not a GUI application'
  assert 'RT_GROUP_ICON' in t or '.rsrc' in t,'launcher resource missing'
  assert not any('libstdc++' in x or 'libgcc' in x for x in imports),'launcher requires external C++ runtime'
(s/'doc/audit/Windows-imports-Preview15.json').write_text(json.dumps({'imports':report,'unresolved':missing,'runtime_tested':False},indent=2));assert not missing,missing
licenses=game/'share/licenses/dependencies';licenses.mkdir(parents=True,exist_ok=True)
for d in (r/'deps').iterdir():
 if not d.is_dir():continue
 for n in ['LICENSE.txt','LICENSE','COPYING','docs/LICENSE.TXT','docs/FTL.TXT','docs/GPLv2.TXT']:
  if (d/n).is_file():shutil.copy2(d/n,licenses/(d.name+'-'+Path(n).name))
for n in ['gcc-mingw-w64-base','mingw-w64-common','gcc-mingw-w64-x86-64-posix-runtime']:
 p=r/'win-tools/usr/share/doc'/n/'copyright'
 if p.exists():shutil.copy2(p,licenses/(n+'-copyright'))
with zipfile.ZipFile(r/(out.name+'.zip'),'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in out.rglob('*'):
  if p.is_file():z.write(p,str(p.relative_to(out.parent)))
print('PASS Windows executable, native GUI launcher resources/static runtime, DLL closure;',len(files),'binaries')
