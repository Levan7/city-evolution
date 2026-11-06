import os,pathlib,time,subprocess
r=pathlib.Path('preview15-work').resolve();e=os.environ.copy();e['LD_PRELOAD']=str(r/'xvfb_paths.so');e['CITY_TEST_XKB_BIN']=str(r/'sysroot/usr/bin');e.update(DISPLAY='127.0.0.1:97',SDL_VIDEO_DRIVER='x11',SDL_RENDER_DRIVER='software',SDL_AUDIO_DRIVER='nonexistent');e.pop('WAYLAND_SOCKET',None);e.pop('WAYLAND_DISPLAY',None)
log=open(r/'logs/x11-server.log','w');x=subprocess.Popen([str(r/'sysroot/usr/bin/Xvfb'),':97','-screen','0','1920x1080x24','-nolisten','local','-nolisten','unix','-listen','tcp','-ac','-xkbdir','/usr/share/X11/xkb'],env=e,stdout=log,stderr=log)
try:
 time.sleep(.6);assert x.poll() is None,'X11 server unavailable'
 e['LD_PRELOAD']=str(r/'ui_game_hook.so');e['CITY_SMOKE_SCREEN_DIR']=str(r/'screens');(r/'screens').mkdir(exist_ok=True)
 u=r/'ui-user';u.mkdir(exist_ok=True);(u/'config.xml').write_text('<lc-config><video><useOpenGL>no</useOpenGL></video><audio><musicEnabled>no</musicEnabled><soundEnabled>no</soundEnabled></audio></lc-config>')
 log=open(r/'logs/game-ui.log','w');g=subprocess.run([str(r/'build/bin/lincity-ng'),'--app-data',str(r/'build/share/lincity-ng'),'--sdl','--window','-S','1920x1080','--user-data',str(u),'--config',str(u/'config.xml')],env=e,stdout=log,stderr=log,timeout=22)
 assert g.returncode==0,'Graphical game test failed: '+str(g.returncode)
 print('PASS native graphical UI: loaded autonomous city, visited budget/Homes/Services/Industry/Civic/Help/Stats and exited cleanly')
finally:x.terminate();x.wait(timeout=5)
