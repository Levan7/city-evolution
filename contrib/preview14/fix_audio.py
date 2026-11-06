from pathlib import Path
s=Path('preview14-work/CityEvolution-Preview-14-Source')
p=s/'src/lincity-ng/Sound.hpp';t=p.read_text().replace('#include <filesystem>','#include <filesystem>\n#include <atomic>');t=t.replace('    void playMusic();','    void playMusic();\n    void update();\n    bool isAudioOpen() const { return audioOpen; }');t=t.replace('    bool audioOpen;','    bool audioOpen=false;\n    bool mixerInitialized=false, audioInitialized=false;\n    std::atomic<bool> musicAdvanceRequested{false};');t=t.replace('SDL_Thread* loaderThread;','SDL_Thread* loaderThread=nullptr;').replace('MIX_Audio* currentMusic;','MIX_Audio* currentMusic=nullptr;').replace('    int totalTracks;','    int totalTracks=-1;').replace('MIX_Track *musicTrack;','MIX_Track *musicTrack=nullptr;').replace('MIX_Track *soundTrack;','MIX_Track *soundTrack=nullptr;').replace('MIX_Mixer *mixer;','MIX_Mixer *mixer=nullptr;');p.write_text(t)
p=s/'src/lincity-ng/Sound.cpp';t=p.read_text().replace('#include <cassert>','#include <cassert>\n#include <algorithm>')
a=t.index('Sound::Sound()');b=t.index('/*\n *  Playback an Audio-Effect.',a)
t=t[:a]+'''Sound::Sound() {
  assert(soundPtr==nullptr);soundPtr=this;
  try { loadMusicTheme();totalTracks=int(playlist.size())-1;if(!playlist.empty())currentTrack=playlist.front(); }
  catch(const std::exception& ex){std::cerr<<"Music theme unavailable: "<<ex.what()<<'\\n';}
  if(!SDL_InitSubSystem(SDL_INIT_AUDIO)){std::cerr<<"Audio unavailable: "<<SDL_GetError()<<'\\n';return;}
  audioInitialized=true;
  if(!MIX_Init()){std::cerr<<"Mixer unavailable: "<<SDL_GetError()<<'\\n';return;}
  mixerInitialized=true;mixer=MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,nullptr);
  if(!mixer){std::cerr<<"Couldn't open audio: "<<SDL_GetError()<<'\\n';return;}
  musicTrack=MIX_CreateTrack(mixer);soundTrack=MIX_CreateTrack(mixer);
  if(!musicTrack||!soundTrack){std::cerr<<"Couldn't create audio tracks: "<<SDL_GetError()<<'\\n';return;}
  audioOpen=true;
  setMusicVolume(getConfig()->musicVolume.get());setSoundVolume(getConfig()->soundVolume.get());
  // Load on the owning thread: previously the loader mutated waves while playback read it.
  try { loadWaves(); }catch(const std::exception& ex){std::cerr<<"Some effects unavailable: "<<ex.what()<<'\\n';}
  std::cerr<<"[audio] driver "<<SDL_GetCurrentAudioDriver()<<"; music/effect tracks ready\\n";
  playMusic();
}

Sound::~Sound() {
  audioOpen=false;
  if(loaderThread)SDL_WaitThread(loaderThread,nullptr);
  if(musicTrack){MIX_SetTrackStoppedCallback(musicTrack,nullptr,nullptr);MIX_StopTrack(musicTrack,0);MIX_SetTrackAudio(musicTrack,nullptr);}
  if(soundTrack){MIX_StopTrack(soundTrack,0);MIX_SetTrackAudio(soundTrack,nullptr);}
  if(mixer)MIX_DestroyMixer(mixer);
  mixer=nullptr;musicTrack=nullptr;soundTrack=nullptr;
  if(currentMusic)MIX_DestroyAudio(currentMusic);
  for(auto& item:waves)MIX_DestroyAudio(item.second);
  if(mixerInitialized)MIX_Quit();
  if(audioInitialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);
  if(soundPtr==this)soundPtr=nullptr;
}

'''+t[b:]
a=t.index('Sound::musicHalted(');b=t.index('\n\n/*',a)
t=t[:a]+'''Sound::musicHalted(void *userdata, MIX_Track *) {
  // Audio callback never loads files or changes tracks. The main thread owns playback.
  if(auto* self=static_cast<Sound*>(userdata))self->musicAdvanceRequested.store(true);
}

void Sound::update() {
  if(musicAdvanceRequested.exchange(false)&&audioOpen&&getConfig()->musicEnabled.get())changeTrack(NEXT_OR_FIRST_TRACK);
}
'''+t[b:]
a=t.index('void\nSound::playMusic()');b=t.index('void\nSound::enableMusic',a)
t=t[:a]+'''void
Sound::playMusic() {
  if(!audioOpen||!musicTrack||!getConfig()->musicEnabled.get()||playlist.empty())return;
  MIX_SetTrackStoppedCallback(musicTrack,nullptr,nullptr);
  MIX_StopTrack(musicTrack,0);MIX_SetTrackAudio(musicTrack,nullptr);musicAdvanceRequested=false;
  if(currentMusic){MIX_DestroyAudio(currentMusic);currentMusic=nullptr;}
  const float tech=tech_level*100.f/MAX_TECH_LEVEL;
  bool eligible=false;
  for(size_t attempts=0;attempts<playlist.size();++attempts){
    if(tech>=currentTrack.lowestTechLevel&&tech<=currentTrack.highestTechLevel){eligible=true;break;}
    currentTrack=playlist[(currentTrack.trackNumber+1)%playlist.size()];
  }
  if(!eligible)return;
  currentMusic=MIX_LoadAudio(mixer,currentTrack.filename.string().c_str(),false);
  if(!currentMusic){std::cerr<<"Couldn't load music: "<<SDL_GetError()<<'\\n';return;}
  MIX_SetTrackAudio(musicTrack,currentMusic);MIX_SetTrackGain(musicTrack,getConfig()->musicVolume.get()*.01f);
  MIX_SetTrackStoppedCallback(musicTrack,musicHalted,this);MIX_PlayTrack(musicTrack,0);
}

'''+t[b:]
t=t.replace('    if(playlist.empty())','    if(!audioOpen||playlist.empty())')
t=t.replace('  if(!audioOpen)\n    return;\n\n  if(enabled)', '  if(!audioOpen||!musicTrack)\n    return;\n\n  if(enabled)')
t=t.replace('      MIX_StopTrack(musicTrack, MIX_TrackMSToFrames(musicTrack, 1000));','      MIX_SetTrackStoppedCallback(musicTrack,nullptr,nullptr);\n      MIX_StopTrack(musicTrack,0);musicAdvanceRequested=false;')
t=t.replace('    assert(vol >= 0 && vol <= 100);','    vol=std::clamp(vol,0,100);')
t=t.replace('    MIX_SetTrackGain(musicTrack, volvalue);','    if(audioOpen&&musicTrack)MIX_SetTrackGain(musicTrack, volvalue);').replace('    MIX_SetTrackGain(soundTrack, volvalue);','    if(audioOpen&&soundTrack)MIX_SetTrackGain(soundTrack, volvalue);')
p.write_text(t)
p=s/'src/lincity-ng/main.cpp';t=p.read_text().replace('    SDL_INIT_AUDIO |\n','');p.write_text(t)
for name in ['Game.cpp','MainMenu.cpp']:
 p=s/'src/lincity-ng'/name;t=p.read_text();old='        next_task = std::min(';assert old in t;t=t.replace(old,'        getSound()->update();\n'+old);p.write_text(t)
