import pathlib,subprocess,shutil,re,os,json
r=pathlib.Path('preview15-work').resolve();s=r/'CityEvolution-Preview-15-Source';app=r/'CityEvolution.AppDir'
subprocess.run(['cmake','--install',str(r/'build'),'--prefix',str(app/'usr')],check=True,stdout=open(r/'logs/linux-install.log','w'))
lib=app/'usr/lib';lib.mkdir(exist_ok=True);queue=[app/'usr/bin/lincity-ng'];seen=set();origins={}
exclude=re.compile(r'^(libasound\.so|libpulse|libwayland|libxkbcommon|libdecor|libEGL|libGL\.so|libGLX|libGLdispatch|libgbm|libdrm|libc\.so|libm\.so|libdl\.so|libpthread\.so|librt\.so|ld-linux|libresolv\.so)')
def copy(src,name=None):
 src=pathlib.Path(src);name=name or src.name
 if name in seen or exclude.match(name):return
 seen.add(name);shutil.copy2(src.resolve(),lib/name);origins[name]=str(src.resolve());queue.append(lib/name)
for pat in ['libX11.so.6','libXext.so.6','libXcursor.so.1','libXfixes.so.3','libXi.so.6','libXrandr.so.2','libXrender.so.1','libXss.so.1','libXtst.so.6','libasound.so.2']:
 for d in [r/'sysroot/usr/lib/x86_64-linux-gnu',pathlib.Path('/usr/lib/x86_64-linux-gnu')]:
  if (d/pat).exists():copy(d/pat);break
while queue:
 f=queue.pop(0);out=subprocess.check_output(['ldd',str(f)],text=True)
 for line in out.splitlines():
  m=re.search(r'^\s*(\S+) => (/\S+)',line)
  if m:copy(m.group(2),m.group(1))
  elif 'not found' in line:raise RuntimeError(line)
# SDL dlopen uses resolved filenames on this platform, alongside ordinary SONAMEs.
for name,origin in origins.items():
 alias=lib/pathlib.Path(origin).name
 if not alias.exists() and alias.name!=name:alias.symlink_to(name)
(app/'AppRun').write_text('''#!/bin/sh
set -eu
APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
CITY_USER_STORE="${XDG_DATA_HOME:-$HOME/.local/share}/city-evolution"
CITY_CONFIG_STORE="${XDG_CONFIG_HOME:-$HOME/.config}/city-evolution"
mkdir -p "$CITY_USER_STORE" "$CITY_CONFIG_STORE"
HAS_CONFIG=0
for ARG in "$@"; do case "$ARG" in -c|--config|--config=*) HAS_CONFIG=1;; esac; done
if [ "$HAS_CONFIG" = 0 ]; then set -- --config "$CITY_CONFIG_STORE/config.xml" "$@"; fi
exec "$APPDIR/usr/bin/lincity-ng" --app-data "$APPDIR/usr/share/lincity-ng" --user-data "$CITY_USER_STORE" "$@"
''');(app/'AppRun').chmod(0o755)
(app/'city-evolution.desktop').write_text('[Desktop Entry]\nType=Application\nName=City Evolution\nExec=lincity-ng\nIcon=city-evolution\nCategories=Game;Simulation;\nTerminal=false\n')
shutil.copy2(s/'data/io.github.lincity_ng.lincity-ng.png',app/'city-evolution.png')
if not (app/'.DirIcon').exists():(app/'.DirIcon').symlink_to('city-evolution.png')
doc=app/'usr/share/doc/city-evolution/dependency-licenses';doc.mkdir(parents=True,exist_ok=True)
for d in (r/'deps').iterdir():
 if not d.is_dir():continue
 for name in ['LICENSE.txt','LICENSE','COPYING','docs/LICENSE.TXT','docs/FTL.TXT','docs/GPLv2.TXT']:
  p=d/name
  if p.is_file():shutil.copy2(p,doc/(d.name+'-'+p.name))
for name in ['libstdc++6','libgcc-s1','libicu74','libfreetype6','libx11-6','libasound2t64','libpng16-16t64','libbrotli1','libxcb1','libxau6','libxdmcp6','libbsd0','libmd0','libxml2','zlib1g','libbz2-1.0']:
 p=pathlib.Path('/usr/share/doc')/name/'copyright'
 if p.exists():shutil.copy2(p,doc/(name+'-copyright'))
(s/'doc/audit/Linux-bundled-libraries.json').write_text(json.dumps(origins,indent=2))
print('BUNDLED',len(seen),'libraries')
subprocess.run([str(r/'squashfs-root/AppRun'),'--no-appstream','--runtime-file',str(r/'appimage-runtime-x86_64'),str(app),str(r/'CityEvolution-Preview-15-Linux-x86_64.AppImage')],env={**os.environ,'ARCH':'x86_64'},check=True)

(r/'CityEvolution-Preview-15-Linux-x86_64.AppImage').chmod(0o755)
