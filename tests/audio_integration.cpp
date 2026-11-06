// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity-ng/Config.hpp"
#include "lincity-ng/Sound.hpp"
#include "lincity/modules/all_modules.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
extern Config* configPtr;
int main(int argc,char**argv){try{if(argc<3)return 2;SDL_Init(0);initializeModules();Config config;configPtr=&config;config.appDataDir.session=std::filesystem::path(argv[1]);config.musicTheme.session="test";bool expect=std::string(argv[2])=="working";
 {Sound sound;if(sound.isAudioOpen()!=expect)throw std::runtime_error("audio availability did not match test setup");
  for(int i=0;i<1000;++i){sound.setMusicVolume(i%101);sound.setSoundVolume((100-i%101));}
  sound.setMusicVolume(150);sound.setSoundVolume(-1);if(config.musicVolume.get()!=100||config.soundVolume.get()!=0)throw std::runtime_error("volume clamping");
  if(expect){for(int wait=0;wait<160&&sound.currentTrack.trackNumber==0;++wait){SDL_Delay(50);sound.update();}if(sound.currentTrack.trackNumber!=1)throw std::runtime_error("finished music did not advance on main thread");for(int i=0;i<20;++i){sound.changeTrack(NEXT_OR_FIRST_TRACK);sound.enableMusic(false);sound.update();sound.enableMusic(true);}sound.setMusicVolume(45);sound.setSoundVolume(65);sound.playSound("Click");}
  else {sound.enableMusic(false);sound.enableMusic(true);sound.changeTrack(NEXT_TRACK);sound.playSound("Click");sound.playASound(nullptr);sound.update();}
 }
 configPtr=nullptr;SDL_Quit();std::cout<<"PASS "<<(expect?"working-device playback, completion queue and track changes":"missing-device controls and shutdown")<<"; 1,000 volume changes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
