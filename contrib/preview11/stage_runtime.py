from pathlib import Path
import sys,json,re,shutil,hashlib
W=Path(__file__).resolve().parent;G=W/'CityEvolution-Preview-11';BIN=G/'Game/bin';S=W/'CityEvolution-Preview-11-Source'
sys.path.insert(0,str(W/'host-python'));import pefile
for name,source in [('lincity-ng.exe',W/'build-win/bin/lincity-ng.exe')]+[(n+'-checks.exe',W/'build-win/tests/city-evolution'/(n+'-checks.exe')) for n in ['community','transit','mayor','city-evolution']]:
 assert source.exists(),source;shutil.copy2(source,BIN/name)
old={p.name.lower():p for p in BIN.glob('*.dll')};available={p.name.lower():p for p in (W/'sdk/ucrt64/bin').glob('*.dll')}
tc=W/'llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64'
for p in (tc/'x86_64-w64-mingw32/bin').glob('*.dll'):available[p.name.lower()]=p
systems=set('kernel32.dll kernelbase.dll advapi32.dll bcrypt.dll cfgmgr32.dll comctl32.dll comdlg32.dll crypt32.dll d3d11.dll d3d12.dll d3d9.dll d3dcompiler_47.dll dbghelp.dll dinput8.dll dsound.dll dwmapi.dll dwrite.dll dxgi.dll gdi32.dll hid.dll imm32.dll iphlpapi.dll mf.dll mfplat.dll mfreadwrite.dll mfuuid.dll msacm32.dll msvcrt.dll ncrypt.dll normaliz.dll ntdll.dll ole32.dll oleaut32.dll opengl32.dll powrprof.dll propsys.dll psapi.dll rpcrt4.dll secur32.dll setupapi.dll shell32.dll shlwapi.dll strmiids.dll ucrtbase.dll user32.dll userenv.dll usp10.dll uxtheme.dll version.dll winhttp.dll wininet.dll winmm.dll winspool.drv wintrust.dll ws2_32.dll wtsapi32.dll avrt.dll winmmbase.dll dxva2.dll msimg32.dll dnsapi.dll ksuser.dll'.split())
def system(n):return n in systems or n.startswith(('api-ms-win-','ext-ms-win-'))
def pe(p):
 obj=pefile.PE(str(p),fast_load=True);obj.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT'],pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']]);return obj
q=list(BIN.glob('*.exe'));required={};seen={};missing=[];fallback=set()
# The shipped media uses PNG, WAV, Ogg Vorbis and TrueType. Include their dynamic plugins.
for n in ['libpng16-16.dll','libjpeg-8.dll','libvorbisfile-3.dll','libfreetype-6.dll','libharfbuzz-0.dll']:
 assert n in available,n;required[n]=available[n];q.append(available[n])
while q:
 p=q.pop();key=p.name.lower()
 if key in seen:continue
 obj=pe(p);assert obj.FILE_HEADER.Machine==0x8664;seen[key]=p
 for d in getattr(obj,'DIRECTORY_ENTRY_IMPORT',[]):
  n=d.dll.decode().lower()
  if system(n):continue
  dep=available.get(n) or old.get(n)
  if not dep:missing.append([key,n]);continue
  if n not in available:fallback.add(n)
  required[n]=dep;q.append(dep)
symbols=[]
for key,p in seen.items():
 for d in getattr(pe(p),'DIRECTORY_ENTRY_IMPORT',[]):
  n=d.dll.decode().lower()
  if system(n) or n not in seen:continue
  exports=getattr(pe(seen[n]),'DIRECTORY_ENTRY_EXPORT',None);names={s.name for s in exports.symbols if s.name} if exports else set();ords={s.ordinal for s in exports.symbols} if exports else set()
  for i in d.imports:
   if i.name and i.name not in names:symbols.append([key,n,i.name.decode()])
   elif not i.name and i.ordinal not in ords:symbols.append([key,n,'ordinal '+str(i.ordinal)])
report={'missing_dlls':missing,'missing_symbols':symbols,'resolved_pe_images':len(seen),'old_runtime_fallbacks':sorted(fallback),'dlls':sorted(required)}
(S/'validation/preview11-runtime-audit.json').write_text(json.dumps(report,indent=2))
if missing or symbols:print(json.dumps(report,indent=2));raise SystemExit(2)
for n,p in required.items():
 dst=BIN/p.name
 if p.resolve()!=dst.resolve():shutil.copy2(p,dst)
for p in BIN.glob('*.dll'):
 if p.name.lower() not in required:p.unlink()
licenses=G/'Licenses/Preview-11-Runtime';licenses.mkdir(parents=True,exist_ok=True);shutil.copytree(W/'sdk/ucrt64/share/licenses',licenses/'MSYS2',dirs_exist_ok=True)
for p in tc.glob('LICENSE*'):
 if p.is_file():shutil.copy2(p,licenses/('LLVM-MinGW-'+p.name))
report['binaries']={p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in BIN.iterdir() if p.suffix.lower() in ['.dll','.exe']}
(S/'validation/preview11-runtime-audit.json').write_text(json.dumps(report,indent=2))
print('PASS Windows DLL imports/exports',len(seen),'images',flush=True)
