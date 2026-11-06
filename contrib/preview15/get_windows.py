import pathlib,re,subprocess,concurrent.futures
r=pathlib.Path('preview15-work').resolve();out=r/'win-debs';out.mkdir(exist_ok=True);dest=r/'win-tools';dest.mkdir(exist_ok=True)
items=re.findall(r"'([^']+)'\s+(\S+)\s+(\d+)",(r/'win-uris.txt').read_text())
def get(item):
 url,name,size=item;url='https://archive.ubuntu.com/ubuntu/pool/'+url.split('/pool/',1)[1];url=url.replace('%2b','+');f=out/name
 subprocess.run(['curl','-fLsS','--retry','2','--max-time','180',url,'-o',str(f)],check=True);assert f.stat().st_size==int(size);subprocess.run(['dpkg-deb','-x',str(f),str(dest)],check=True);print(name,flush=True)
with concurrent.futures.ThreadPoolExecutor(3) as e:list(e.map(get,items))
for c in ['gcc','g++']:
 p=dest/'usr/bin'/('x86_64-w64-mingw32-'+c)
 if not p.exists():p.symlink_to('x86_64-w64-mingw32-'+c+'-posix')
