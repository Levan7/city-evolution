import os,subprocess,time,json,pathlib
from PIL import ImageGrab
r=pathlib.Path('preview13-work').resolve();(r/'screens').mkdir(exist_ok=True);u=r/'ui-user';u.mkdir(exist_ok=True)
config=pathlib.Path('portable-recovery/CityPreview/Settings/default.xml').read_text().replace('<x>1280</x><y>800</y>','<x>1440</x><y>900</y>');(u/'config.xml').write_text(config)
env=os.environ.copy();env.update(DISPLAY='127.0.0.1:97',SDL_AUDIODRIVER='dummy')
xlog=open(r/'logs/xvfb.log','w');gameLog=open(r/'logs/ui.log','w')
x=subprocess.Popen([str(r/'sysroot/usr/bin/Xvfb'),':97','-screen','0','1440x900x24','-nolisten','local','-nolisten','unix','-listen','tcp','-ac','-xkbdir','/usr/share/X11/xkb'],env=env,stdout=xlog,stderr=xlog);time.sleep(1)
controlEnv=env.copy()
app=os.environ.get('CITY_TEST_APP');cmd=[app,'--appimage-extract-and-run'] if app else [str(r/'build/bin/lincity-ng'),'--app-data',str(r/'build/share/lincity-ng')]
if app:env.pop('LD_LIBRARY_PATH',None)
cmd+=['--sdl','--window','-S','1440x900','--user-data',str(u),'--config',str(u/'config.xml')]
g=subprocess.Popen(cmd,env=env,stdout=gameLog,stderr=gameLog);time.sleep(5)
seen=-1
print('UI_READY',g.poll(),flush=True)
try:
 start=time.time()
 while time.time()-start<1500 and g.poll() is None:
  p=r/'ui-command.json'
  if p.exists():
   d=json.loads(p.read_text());seq=d['seq']
   if seq!=seen:
    seen=seq
    for item in d['commands']:
     if item[0]=='sleep':time.sleep(float(item[1]))
     elif item[0]=='snapshot':ImageGrab.grab(xdisplay=env['DISPLAY']).save(r/'screens'/item[1])
     elif item[0]=='stop':raise KeyboardInterrupt
     else:subprocess.run([str(r/'sysroot/usr/bin/xdotool')]+item,env=controlEnv,check=True)
    print('DONE',seq,flush=True)
  time.sleep(.15)
except KeyboardInterrupt:pass
finally:
 if g.poll() is None:g.terminate()
 x.terminate();print('UI_EXIT',g.poll(),flush=True)
