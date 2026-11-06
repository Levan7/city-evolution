import pathlib,os,subprocess,time,runpy,array,re
r=pathlib.Path('preview14-work').resolve();env=os.environ.copy();env['LD_LIBRARY_PATH']=str(r/'sysroot/usr/lib/x86_64-linux-gnu/pulseaudio')+':'+env.get('LD_LIBRARY_PATH','');env['PULSE_RUNTIME_PATH']=str(r/'pulse-run');pathlib.Path(env['PULSE_RUNTIME_PATH']).mkdir(exist_ok=True)
modules=next((r/'sysroot/usr/lib').glob('pulse-*/modules'));env['LD_LIBRARY_PATH']=str(modules)+':'+env['LD_LIBRARY_PATH'];cfg=r/'pulse.pa';cfg.write_text('load-module module-native-protocol-tcp listen=127.0.0.1 port=4713 auth-anonymous=1\nload-module module-null-sink sink_name=city_test channels=2 rate=48000\nset-default-sink city_test\n')
log=open(r/'logs/packaged-pulse-server.log','w');p=subprocess.Popen([str(r/'sysroot/usr/bin/pulseaudio'),'-n','-F',str(cfg),'-p',str(modules),'--daemonize=no','--use-pid-file=no','--exit-idle-time=-1','--disable-shm=yes','--log-target=stderr'],env=env,stdout=log,stderr=log)
rec=None
try:
 time.sleep(.6);assert p.poll() is None,'Pulse server did not start'
 os.environ.update(PULSE_SERVER='tcp:127.0.0.1:4713',CITY_TEST_AUDIO='auto')
 c=r/'wayland-user/config.xml';c.write_text('<lc-config><audio><musicVolume>35</musicVolume><soundVolume>45</soundVolume></audio></lc-config>')
 pcm=open(r/'game-audio.raw','wb');rec=subprocess.Popen([str(r/'sysroot/usr/bin/parec'),'--server=tcp:127.0.0.1:4713','--device=city_test.monitor','--format=s16le','--rate=48000','--channels=2'],env=env,stdout=pcm,stderr=subprocess.DEVNULL)
 runpy.run_path(str(r/'check_wayland_ui.py'),run_name='__main__')
 rec.terminate();rec.wait(timeout=5);pcm.close();a=array.array('h');a.frombytes((r/'game-audio.raw').read_bytes());peak=max(map(abs,a),default=0);assert peak>0,'Game music produced no PCM'
 t=(r/'logs/appimage-wayland-ui.log').read_text();assert '[audio] driver pulseaudio' in t,'Default audio did not select PulseAudio'
 config=c.read_text();assert '<musicVolume>100</musicVolume>' in config,'Music plus clicks missed';assert '<soundVolume>100</soundVolume>' in config,'Effects plus clicks missed'
 print('PASS packaged AppImage with native Wayland: automatic PulseAudio, non-zero PCM peak='+str(peak)+', 30 music+30 effects graphical plus clicks saved volumes100, graceful exit',flush=True)
finally:
 if rec and rec.poll() is None:rec.terminate();rec.wait(timeout=5)
 p.terminate();p.wait(timeout=5)
