import os,pathlib,time,subprocess
r=pathlib.Path('preview14-work').resolve();e=os.environ.copy();e['LD_PRELOAD']=str(r/'xvfb_paths.so');e['CITY_TEST_XKB_BIN']=str(r/'sysroot/usr/bin');e.update(DISPLAY='127.0.0.1:97',SDL_VIDEO_DRIVER='x11',SDL_RENDER_DRIVER='software',SDL_AUDIO_DRIVER='nonexistent');e.pop('WAYLAND_SOCKET',None);e.pop('WAYLAND_DISPLAY',None)
log=open(r/'logs/x11-server.log','w');x=subprocess.Popen([str(r/'sysroot/usr/bin/Xvfb'),':97','-screen','0','3072x1590x24','-nolisten','local','-nolisten','unix','-listen','tcp','-ac','-xkbdir','/usr/share/X11/xkb'],env=e,stdout=log,stderr=log)
try:
 time.sleep(.6);assert x.poll() is None,'X11 server unavailable'
 e.pop('LD_LIBRARY_PATH',None);e['LD_PRELOAD']=str(r/'ui_smoke_hook.so');e['CITY_SMOKE_SCREEN_DIR']=str(r/'screens/x11');(r/'screens/x11').mkdir(exist_ok=True)
 u=r/'x11-user';u.mkdir(exist_ok=True);(u/'config.xml').write_text('<lc-config><audio><musicVolume>35</musicVolume><soundVolume>45</soundVolume></audio></lc-config>')
 log=open(r/'logs/appimage-x11.log','w');g=subprocess.run([str(r/'CityEvolution-Preview-14-Linux-x86_64.AppImage'),'--appimage-extract-and-run','--sdl','--window','-S','3072x1590','--user-data',str(u),'--config',str(u/'config.xml')],env=e,stdout=log,stderr=log,timeout=15)
 assert g.returncode==0,'X11 graphical test failed'
 t=(r/'logs/appimage-x11.log').read_text();assert '[video] driver x11' in t and 'scale 1.75' in t and '[smoke] quit event' in t,t
 c=(u/'config.xml').read_text();assert '<musicVolume>100</musicVolume>' in c and '<soundVolume>100</soundVolume>' in c,'Volume controls missed'
 print('PASS packaged AppImage X11 fallback at 3072x1590: settings, 30 music and30 effects plus clicks, missing-device safe handling and graceful exit')
finally:x.terminate();x.wait(timeout=5)
