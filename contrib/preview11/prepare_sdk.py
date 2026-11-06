from pathlib import Path
import urllib.request,re,json,concurrent.futures as F,tarfile,io,hashlib
R=Path(__file__).resolve().parent;C=R/'cache';C.mkdir(exist_ok=True);S=R/'sdk';S.mkdir(exist_ok=True)
def get(url,p):
 if p.exists():return p.read_bytes()
 for i in range(3):
  try:
   b=urllib.request.urlopen(url,timeout=60).read();p.write_bytes(b);return b
  except Exception:
   if i==2:raise
tcname='llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64'
def tc():
 p=C/(tcname+'.tar.xz');get('https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/'+p.name,p)
 if not (R/tcname).exists():
  with tarfile.open(p) as t:t.extractall(R,filter='data')
 print('Compiler ready',flush=True)
index=get('https://repo.msys2.org/mingw/ucrt64/',R/'index.html').decode()
names=re.findall(r'href="([^"/]+\.pkg\.tar\.zst)"',index)
def ver(n):return tuple(int(x) if x.isdigit() else x for x in re.split(r'(\d+)',n))
def pkg(name):
 prefix='mingw-w64-ucrt-x86_64-'+name+'-';choices=[n for n in names if re.match(re.escape(prefix)+r'\d',n)];assert choices,name
 fn=max(choices,key=ver);raw=get('https://repo.msys2.org/mingw/ucrt64/'+fn,C/fn)
 import zstandard
 with zstandard.ZstdDecompressor().stream_reader(io.BytesIO(raw)) as reader:
  with tarfile.open(fileobj=reader,mode='r|') as t:
   for m in t:
    if m.name.startswith('ucrt64/') and not m.islnk():t.extract(m,S,filter='data')
 print(fn,flush=True);return fn
packages=['sdl3','sdl3-image','sdl3-mixer','sdl3-ttf','zlib','libxml2','freetype','harfbuzz','graphite2','glib2','libiconv','pcre2','libpng','bzip2','brotli','libvorbis','libogg','libjpeg-turbo']
with F.ThreadPoolExecutor(max_workers=3) as ex:
 future=ex.submit(tc);got=list(ex.map(pkg,packages));future.result()
(R/'packages.json').write_text(json.dumps(got,indent=2))
print('All build dependencies ready',flush=True)
