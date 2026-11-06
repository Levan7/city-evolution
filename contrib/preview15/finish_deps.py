from pathlib import Path
import re,subprocess,concurrent.futures,hashlib,urllib.parse
r=Path('preview15-work');items=re.findall(r"'([^']+)'\s+(\S+)\s+(\d+)\s+MD5Sum:(\w+)",(r/'native-uris.txt').read_text())
def get(item):
 url,name,size,md5=item;name=urllib.parse.unquote(name);out=r/'debs'/name
 if out.exists() and out.stat().st_size==int(size) and hashlib.md5(out.read_bytes()).hexdigest()==md5:return
 mirror='https://archive.ubuntu.com/ubuntu/pool/'+url.split('/pool/',1)[1];temp=out.with_suffix('.download')
 for source in [mirror,url]:
  z=subprocess.run(['curl','-fLsS','--connect-timeout','10','--max-time','50',source,'-o',str(temp)])
  if z.returncode==0 and temp.stat().st_size==int(size) and hashlib.md5(temp.read_bytes()).hexdigest()==md5:temp.replace(out);print(name,flush=True);return
 raise RuntimeError(name)
with concurrent.futures.ThreadPoolExecutor(6) as e:list(e.map(get,items))
