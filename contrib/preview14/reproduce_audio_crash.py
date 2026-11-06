import pathlib,subprocess,shlex,os,resource,json
r=pathlib.Path('preview14-work').resolve();b=r/'build';base=r/'base/CityEvolution-Preview-13-Source';current=r/'CityEvolution-Preview-14-Source'
commands=subprocess.check_output(['ninja','-C',str(b),'-t','commands','audio-checks'],text=True).splitlines();compilecmd=next(shlex.split(c) for c in commands if 'Sound.cpp.o -c ' in c);link=next(c for c in reversed(commands) if ' -o tests/city-evolution/audio-checks ' in c);link=shlex.split(link.removeprefix(': && ').removesuffix(' && :'))
for version,src in [('before',base),('after',current)]:
 main=r/f'audio-{version}.cpp';main.write_text('#include "'+str(src/'src/lincity-ng/Sound.hpp')+'"\n#include "lincity-ng/Config.hpp"\n#include "lincity/modules/all_modules.hpp"\n#include <new>\n#include <cstring>\n#include <iostream>\nextern Config* configPtr;\nint main(int argc,char**argv){SDL_Init(0);SDL_InitSubSystem(SDL_INIT_AUDIO);initializeModules();Config cfg;configPtr=&cfg;cfg.appDataDir.session=std::filesystem::path(argv[1]);cfg.musicTheme.session="test";alignas(Sound) unsigned char storage[sizeof(Sound)];std::memset(storage,0xa5,sizeof(storage));Sound* sound=new(storage)Sound();sound->setMusicVolume(90);sound->setSoundVolume(90);sound->~Sound();std::cout<<"Survived volume controls and shutdown\\n";SDL_Quit();}\n')
 objects={}
 for role,file in [('main',main),('sound',src/'src/lincity-ng/Sound.cpp')]:
  cmd=compilecmd.copy();output=r/f'audio-{version}-{role}.o';cmd[cmd.index('-o')+1]=str(output);cmd[cmd.index('-c')+1]=str(file);# Remove dependency output paths to avoid touching the normal target.
  for flag in ['-MD','-MT','-MF']:
   if flag in cmd:
    i=cmd.index(flag);del cmd[i:i+(1 if flag=='-MD' else 2)]
  subprocess.run(cmd,cwd=b,check=True,stdout=subprocess.DEVNULL);objects[role]=str(output)
 cmd=link.copy();cmd[cmd.index('-o')+1]=str(r/f'audio-{version}')
 for i,arg in enumerate(cmd):
  if arg.endswith('/__/audio_integration.cpp.o'):cmd[i]=objects['main']
  if arg.endswith('/lincity-ng/Sound.cpp.o'):cmd[i]=objects['sound']
 subprocess.run(cmd,cwd=b,check=True,stdout=subprocess.DEVNULL)
 env=os.environ.copy();env['SDL_AUDIO_DRIVER']='alsa';env['ALSA_CONFIG_PATH']='/nonexistent'
 def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 result=subprocess.run([str(r/f'audio-{version}'),str(r/'audio-fixture')],env=env,capture_output=True,text=True,preexec_fn=no_core,timeout=10);(r/'logs'/f'audio-regression-{version}.log').write_text('Exit status '+str(result.returncode)+'\n'+result.stdout+result.stderr)
 print(version,result.returncode,flush=True)
 if version=='before':assert result.returncode!=0,'Baseline unexpectedly did not reproduce'
 else:assert result.returncode==0,'Fixed version failed'
print('PASS deterministic Preview13 crash reproduction and Preview14 survival after failed ALSA initialization')
