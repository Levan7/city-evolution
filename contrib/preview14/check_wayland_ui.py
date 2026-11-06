import os,subprocess,time,pathlib,socket
r=pathlib.Path('preview14-work').resolve();rt=pathlib.Path('/tmp/city-evolution-wayland14');rt.mkdir(exist_ok=True);rt.chmod(0o700);u=r/'wayland-user';u.mkdir(exist_ok=True)
e=os.environ.copy();e['XDG_RUNTIME_DIR']=str(rt);e.pop('DISPLAY',None);e['WESTON_MODULE_MAP']='headless-backend.so='+str(r/'sysroot/usr/lib/x86_64-linux-gnu/libweston-13/headless-backend.so')+';desktop-shell.so='+str(r/'sysroot/usr/lib/x86_64-linux-gnu/weston/kiosk-shell.so')+';';e['LD_LIBRARY_PATH']=str(r/'sysroot/usr/lib/x86_64-linux-gnu/weston')+':'+e.get('LD_LIBRARY_PATH','')
# This environment forbids named UNIX sockets but permits inherited socketpairs.
# Weston and libwayland explicitly support a preconnected client/server pair.
server,client=socket.socketpair();e['WAYLAND_SERVER_SOCKET']=str(server.fileno())
westonlog=open(r/'logs/wayland-compositor.log','w');w=subprocess.Popen([str(r/'sysroot/usr/bin/weston'),'--backend=headless','--shell='+str(r/'sysroot/usr/lib/x86_64-linux-gnu/weston/kiosk-shell.so'),'--width=3072','--height=1590','--renderer=pixman','--idle-time=0','--no-config'],pass_fds=(server.fileno(),),env=e,stdout=westonlog,stderr=westonlog);server.close()
try:
 time.sleep(.7);assert w.poll() is None,'Wayland compositor failed; see log'
 e.pop('WAYLAND_SERVER_SOCKET',None);e.update(WAYLAND_SOCKET=str(client.fileno()),SDL_VIDEO_DRIVER='wayland',SDL_RENDER_DRIVER='software',SDL_AUDIO_DRIVER='nonexistent');e.pop('LD_LIBRARY_PATH',None)
 (r/'screens').mkdir(exist_ok=True);e['LD_PRELOAD']=str(r/'ui_smoke_hook.so');e['CITY_SMOKE_SCREEN_DIR']=str(r/'screens')
 if os.environ.get('CITY_TEST_AUDIO')=='auto':e.pop('SDL_AUDIO_DRIVER',None)
 log=open(r/'logs/appimage-wayland-ui.log','w')
 g=subprocess.Popen([str(r/'CityEvolution-Preview-14-Linux-x86_64.AppImage'),'--appimage-extract-and-run','--sdl','--window','-S','3072x1590','--user-data',str(u),'--config',str(u/'config.xml')],pass_fds=(client.fileno(),),env=e,stdout=log,stderr=log);client.close()
 try:
  result=g.wait(timeout=15);assert result==0,'Packaged Wayland game failed graceful UI exit: '+str(result)
  log.flush();t=(r/'logs/appimage-wayland-ui.log').read_text();assert '[video] driver wayland' in t,t;assert 'scale 1.75' in t,t;assert '[startup] menus ready' in t,t;assert '[smoke] 30 music and effects plus clicks' in t,t
  print('PASS packaged native Wayland game under Weston headless using inherited Wayland socket, 3072x1590, scale1.75, no DISPLAY or development library paths; graphical volume controls and graceful exit',flush=True)
 finally:
  if g.poll() is None:g.terminate()
  g.wait(timeout=10)
finally:
 if w.poll() is None:w.terminate()
 w.wait(timeout=10)
