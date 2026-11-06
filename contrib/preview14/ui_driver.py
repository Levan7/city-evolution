import os,subprocess,time,json,pathlib
from PIL import ImageGrab
r=pathlib.Path('preview14-work').resolve();(r/'screens').mkdir(exist_ok=True);u=r/'ui-user';u.mkdir(exist_ok=True)
width=int(os.environ.get('CITY_WIDTH','1440'));height=int(os.environ.get('CITY_HEIGHT','900'))
config=pathlib.Path('portable-recovery/CityPreview/Settings/default.xml').read_text().replace('<x>1280</x><y>800</y>',f'<x>{width}</x><y>{height}</y>');(u/'config.xml').write_text(config)
env=os.environ.copy();env.update(DISPLAY='127.0.0.1:97',SDL_AUDIO_DRIVER=os.environ.get('CITY_AUDIO','dummy'));env.pop('SDL_AUDIODRIVER',None)
xlog=open(r/'logs/xvfb.log','w');gameLog=open(r/'logs/ui.log','w')
x=subprocess.Popen([str(r/'sysroot/usr/bin/Xvfb'),':97','-screen','0',f'{width}x{height}x24','-nolisten','local','-nolisten','unix','-listen','tcp','-ac','-xkbdir','/usr/share/X11/xkb'],env=env,stdout=xlog,stderr=xlog);time.sleep(1)
controlEnv=env.copy()
app=os.environ.get('CITY_TEST_APP');cmd=[app,'--appimage-extract-and-run'] if app else [str(r/'build/bin/lincity-ng'),'--app-data',str(r/'build/share/lincity-ng')]
if app:env.pop('LD_LIBRARY_PATH',None)
cmd+=['--sdl','--window','-S',f'{width}x{height}','--user-data',str(u),'--config',str(u/'config.xml')]
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
     elif item[0]=='ui_move':
      import math,re
      found=subprocess.check_output([str(r/'sysroot/usr/bin/xdotool'),'search','--name','^City Evolution'],env=controlEnv,text=True).splitlines()[-1]
      geo=subprocess.check_output([str(r/'sysroot/usr/bin/xdotool'),'getwindowgeometry','--shell',found],env=controlEnv,text=True)
      w=int(re.search(r'WIDTH=(\d+)',geo)[1]);h=int(re.search(r'HEIGHT=(\d+)',geo)[1]);scale=max(1,min(3,math.floor(min(w/1440,h/900)*4)/4))
      subprocess.run([str(r/'sysroot/usr/bin/xdotool'),'mousemove',str(round(float(item[1])*scale)),str(round(float(item[2])*scale))],env=controlEnv,check=True)
     elif item[0]=='stop':raise KeyboardInterrupt
     else:subprocess.run([str(r/'sysroot/usr/bin/xdotool')]+item,env=controlEnv,check=True)
    print('DONE',seq,flush=True)
  time.sleep(.15)
except KeyboardInterrupt:pass
finally:
 if g.poll() is None:g.terminate()
 x.terminate();print('UI_EXIT',g.poll(),flush=True)
