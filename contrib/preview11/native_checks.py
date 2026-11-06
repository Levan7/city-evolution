from pathlib import Path
import urllib.request,concurrent.futures as F,io,tarfile,shutil,os,subprocess
W=Path(__file__).resolve().parent;N=W/'native-dev';N.mkdir(exist_ok=True)
def headers(pair):
 url,name=pair;p=W/'cache'/name
 if not p.exists():p.write_bytes(urllib.request.urlopen(url+name,timeout=45).read())
 raw=p.read_bytes();i=8
 while i<len(raw):
  h=raw[i:i+60];size=int(h[48:58]);name=h[:16].decode().strip().rstrip('/');blob=raw[i+60:i+60+size];i+=60+size+size%2
  if name.startswith('data.tar'):
   if name.endswith('zst'):
    import zstandard;blob=zstandard.ZstdDecompressor().stream_reader(io.BytesIO(blob)).read()
   with tarfile.open(fileobj=io.BytesIO(blob)) as t:t.extractall(N,members=[m for m in t.getmembers() if '/include/' in m.name],filter='data')
   break
with F.ThreadPoolExecutor(max_workers=2) as e:list(e.map(headers,[('https://archive.ubuntu.com/ubuntu/pool/main/libx/libxml2/','libxml2-dev_2.9.14+dfsg-1.3ubuntu3_amd64.deb'),('https://archive.ubuntu.com/ubuntu/pool/main/i/icu/','libicu-dev_74.2-1ubuntu3_amd64.deb')]))
shutil.copy2('/usr/include/iconv.h',N/'usr/include/iconv.h')
fake=W/'native-interfaces';fake.mkdir(exist_ok=True)
for name in ['SDL3','SDL3_image','SDL3_ttf','SDL3_mixer']:
 p=fake/name;p.mkdir(exist_ok=True);(p/(name+'Config.cmake')).write_text('add_library('+name+'::'+name+' INTERFACE IMPORTED)\nset_target_properties('+name+'::'+name+' PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "'+str(W/'sdk/ucrt64/include')+'")\n')
 (p/(name+'ConfigVersion.cmake')).write_text('set(PACKAGE_VERSION "3.4.18")\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n')
env=os.environ.copy();env['PYTHONPATH']=str(W/'host-python');env['PATH']=str(W/'host-bin')+':'+env['PATH'];cm=W/'host-python/cmake/data/bin/cmake';S=W/'CityEvolution-Preview-11-Source'
args=[str(cm),'-S',str(S),'-B',str(W/'build-native'),'-G','Ninja','-DCMAKE_MAKE_PROGRAM='+str(W/'host-python/bin/ninja'),'-DCMAKE_PREFIX_PATH='+str(fake),'-DLIBXML2_INCLUDE_DIR='+str(N/'usr/include/libxml2'),'-DLIBXML2_LIBRARY=/usr/lib/x86_64-linux-gnu/libxml2.so.2','-DZLIB_INCLUDE_DIR='+str(W/'sdk/ucrt64/include'),'-DZLIB_LIBRARY=/usr/lib/x86_64-linux-gnu/libz.so.1','-DCMAKE_CXX_FLAGS=-I'+str(N/'usr/include'),'-DENABLE_NLS=OFF','-DLINCITYNG_EVOLUTION_CHECKS=ON','-DCMAKE_BUILD_TYPE=Debug','-DCMAKE_POLICY_VERSION_MINIMUM=3.5']
with (W/'native-configure.log').open('w') as log:r=subprocess.run(args,env=env,stdout=log,stderr=subprocess.STDOUT)
if r.returncode:print((W/'native-configure.log').read_text()[-3000:]);raise SystemExit(r.returncode)
with (W/'native-build.log').open('w') as log:r=subprocess.run([str(cm),'--build',str(W/'build-native'),'--target','community-checks','transit-checks','--parallel','3'],env=env,stdout=log,stderr=subprocess.STDOUT)
if r.returncode:print((W/'native-build.log').read_text()[-3000:]);raise SystemExit(r.returncode)
for name in ['community','transit']:
 with (S/'validation'/f'preview11-{name}.log').open('w') as log:r=subprocess.run([str(W/'build-native/tests/city-evolution'/f'{name}-checks')],stdout=log,stderr=subprocess.STDOUT)
 print(name,r.returncode,(S/'validation'/f'preview11-{name}.log').read_text(),flush=True)
 if r.returncode:raise SystemExit(r.returncode)
