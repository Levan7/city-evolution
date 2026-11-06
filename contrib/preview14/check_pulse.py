import pathlib,os,subprocess,time
r=pathlib.Path('preview14-work').resolve();env=os.environ.copy();env['LD_LIBRARY_PATH']=str(r/'sysroot/usr/lib/x86_64-linux-gnu/pulseaudio')+':'+env.get('LD_LIBRARY_PATH','');env['PULSE_RUNTIME_PATH']=str(r/'pulse-run');pathlib.Path(env['PULSE_RUNTIME_PATH']).mkdir(exist_ok=True)
modules=next((r/'sysroot/usr/lib').glob('pulse-*/modules'));env['LD_LIBRARY_PATH']=str(modules)+':'+env['LD_LIBRARY_PATH'];cfg=r/'pulse.pa';cfg.write_text('load-module module-native-protocol-tcp listen=127.0.0.1 port=4713 auth-anonymous=1\nload-module module-null-sink sink_name=city_test channels=2 rate=48000\nset-default-sink city_test\n')
log=open(r/'logs/pulse-server.log','w');p=subprocess.Popen([str(r/'sysroot/usr/bin/pulseaudio'),'-n','-F',str(cfg),'-p',str(modules),'--daemonize=no','--use-pid-file=no','--exit-idle-time=-1','--disable-shm=yes','--log-target=stderr'],env=env,stdout=log,stderr=log)
try:
 time.sleep(1);assert p.poll() is None,'Pulse server did not start'
 testenv=os.environ.copy();testenv['PULSE_SERVER']='tcp:127.0.0.1:4713';testenv['SDL_AUDIO_DRIVER']='pulseaudio';testenv.pop('SDL_AUDIODRIVER',None)
 result=subprocess.run([str(r/'build/tests/city-evolution/audio-checks'),str(r/'audio-fixture'),'working'],env=testenv,capture_output=True,text=True,timeout=20);print(result.stdout,result.stderr);assert result.returncode==0
 print('PASS real PulseAudio protocol playback and volume changes with null output sink')
finally:p.terminate();p.wait(timeout=5)
