/* ---------------------------------------------------------------------- *
 * src/lincity-ng/Sound.cpp
 * This file is part of Lincity-NG.
 *
 * Copyright (C) 2005      Wolfgang Becker <uafr@gmx.de>
 * Copyright (C) 2025      David Bears <dbear4q@gmail.com>
 * Copyright (C) 2026      Marc Young <myoung008@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
** ---------------------------------------------------------------------- */

#include "Sound.hpp"
#include <algorithm>
#include "lincity/groups.hpp"

#include <SDL3/SDL.h>                     // for SDL_GetError, SDL_CreateThread
#include <SDL3_mixer/SDL_mixer.h>         // for Mix_Volume, Mix_FreeMusic
#include <fmt/base.h>                     // for println
#include <fmt/format.h>                   // for format
#include <libxml++/parsers/textreader.h>  // for TextReader
#include <libxml++/ustring.h>             // for ustring
#include <stdio.h>                        // for stderr, size_t, fprintf, NULL
#include <stdlib.h>                       // for rand
#include <cassert>
#include <algorithm>                        // for assert
#include <cmath>                          // for round
#include <iostream>                       // for basic_ostream, operator<<
#include <optional>                       // for optional
#include <stdexcept>                      // for runtime_error
#include <utility>                        // for pair
#include <vector>                         // for vector
#include <fmt/std.h> // IWYU pragma: keep

#include "Config.hpp"                     // for getConfig, Config
#include "lincity/lin-city.hpp"           // for MAX_TECH_LEVEL
#include "lincity/lintypes.hpp"           // for Construction
#include "lincity/resources.hpp"          // for ResourceGroup
#include "lincity/world.hpp"              // for MapTile
#include "util/xmlutil.hpp"               // for xmlParse, unexpectedXmlAttr...

Sound* soundPtr = 0;

Sound *getSound()
{
  return soundPtr;
}

int
Sound::soundThread(void* ptr)
{
    Sound* sound = (Sound*) ptr;
    sound->loadWaves();
    return 0;
}

void
Sound::loadWaves() {
  // TODO: improve the format of sounds.xml

  std::filesystem::path directory = getConfig()->appDataDir.get() / "sounds";
  std::filesystem::path xmlfile = directory / "sounds.xml";
  xmlpp::TextReader reader(xmlfile.string());
  if(!reader.read())
    throw std::runtime_error(fmt::format("file is empty: {}", xmlfile));
  while(reader.get_node_type() != xmlpp::TextReader::NodeType::Element) {
    if(!reader.next())
      throw std::runtime_error(
        fmt::format("file doesn't contain XML data: {}", xmlfile));
  }

  std::filesystem::path filename;
  std::filesystem::path fullname;
  std::vector<ResourceGroup*> resGrpVec;
  resGrpVec.clear();
  int resourceID_level = 0;
  std::filesystem::path key;
  MIX_Audio *chunk;

  if(!reader.is_empty_element())
  while(reader.read()) {
    if(reader.get_node_type() != xmlpp::TextReader::NodeType::Element) {
      continue;
    }
    xmlpp::ustring element = reader.get_name();
    if(element == "resourceID") {
      std::string resName;

      while(reader.move_to_next_attribute()) {
        xmlpp::ustring name = reader.get_name();
        xmlpp::ustring value = reader.get_value();
        if(name == "name")
          resName = xmlParse<std::string>(value);
        else
          unexpectedXmlAttribute(reader);
      }
      reader.move_to_element();

      if(ResourceGroup::resMap.count(resName))
      {
        resGrpVec.push_back(ResourceGroup::resMap[resName]);
        resourceID_level = reader.get_depth();
        if(resGrpVec.back()->sounds_loaded) //could crash if game is already running
          fmt::println(stderr,
            "warning: duplicate resourceID in sounds.xml: {:?}", resName);
      }
      else
        fmt::println(stderr,
          "warning: unknown resourceID in sounds.xml: {:?}", resName);
    }
    if(reader.get_depth() < resourceID_level-1) {
      for(size_t i=0; i< resGrpVec.size(); ++i)
        resGrpVec[i]->sounds_loaded = true;
      resGrpVec.clear();
      resourceID_level = 0;
    }
    if(element == "sound") {
      while(reader.move_to_next_attribute()) {
        xmlpp::ustring name = reader.get_name();
        xmlpp::ustring value = reader.get_value();
        if(name == "file")
          key = xmlParse<std::filesystem::path>(value);
        else
          unexpectedXmlAttribute(reader);
      }
      reader.move_to_element();

      fullname = directory / key;
      chunk = MIX_LoadAudio(mixer, fullname.string().c_str(), true);
      if(!chunk) {
        fmt::println(stderr, "warning: filed to load sound {:?}: {}",
          key, SDL_GetError());
      }
      if (resourceID_level && resGrpVec.size()) {
        for(size_t i=0; i< resGrpVec.size(); ++i)
          resGrpVec[i]->chunks.push_back(chunk);
      }
      else {
        std::string idName = getIdName(key.string());
        waves.insert(std::pair<std::string,MIX_Audio*>(idName, chunk));
      }
      key.clear();
    }
  }

  if(resGrpVec.size()) {
    for(size_t i=0; i< resGrpVec.size(); ++i)
      resGrpVec[i]->sounds_loaded = true;
    resGrpVec.clear();
  }
}

 /*
 * Load music theme from subfolder of 'music/'.
 */
void Sound::loadMusicTheme() {
  std::filesystem::path musicDir = getConfig()->appDataDir.get() / "music";
  //Reset track counter:
  totalTracks=0;
  playlist.clear();
  //Get the current music theme
  std::string theme = getConfig()->musicTheme.get();
  std::filesystem::path themeDir = musicDir / theme;
  std::filesystem::path xml_name = themeDir / (theme + ".xml");

  if(!std::filesystem::is_regular_file(xml_name))
    throw std::runtime_error(
      std::string("could not load music theme: " + theme));

  //Get the number of songs
  xmlpp::TextReader reader(xml_name.string());
  if(!reader.read())
    throw std::runtime_error(fmt::format("file is empty: {}", xml_name));
  while(reader.get_node_type() != xmlpp::TextReader::NodeType::Element) {
    if(!reader.next())
      throw std::runtime_error(
        fmt::format("file doesn't contain XML data: {}", xml_name));
  }

  if(!reader.is_empty_element() && reader.read())
  while(reader.get_node_type() != xmlpp::TextReader::NodeType::EndElement) {
    if(reader.get_node_type() != xmlpp::TextReader::NodeType::Element) {
      reader.next();
      continue;
    }
    xmlpp::ustring element = reader.get_name();

    if(element == "song") {
      std::string title;
      std::filesystem::path filename;
      float lowest_tech_level = 0.0;
      float highest_tech_level = 10000.0;

      while(reader.move_to_next_attribute()) {
        xmlpp::ustring name = reader.get_name();
        xmlpp::ustring value = reader.get_value();
        if(name == "title")
          title = xmlParse<std::string>(value);
        else if(name == "filename")
          filename = themeDir / xmlParse<std::filesystem::path>(value);
        else if(name == "highest-tech-level")
          highest_tech_level = xmlParse<float>(value);
        else if(name == "lowest-tech-level")
          lowest_tech_level = xmlParse<float>(value);
        else
          unexpectedXmlAttribute(reader);
      }
      reader.move_to_element();

      song tempSong;
      tempSong.title = title;
      tempSong.filename = filename;
      tempSong.trackNumber = totalTracks;
      tempSong.lowestTechLevel = lowest_tech_level;
      if(highest_tech_level == 0)
        highest_tech_level = 10 * MAX_TECH_LEVEL;
      tempSong.highestTechLevel = highest_tech_level;

      playlist.push_back(tempSong);
      totalTracks++;
    }
    else {
      unexpectedXmlElement(reader);
    }
    reader.next();
  }

  while(reader.next()) {
    if(reader.get_node_type() != xmlpp::TextReader::NodeType::Element)
      continue;
    unexpectedXmlElement(reader);
  }
}

Sound::Sound() {
  assert(soundPtr==nullptr);soundPtr=this;
  try { loadMusicTheme();totalTracks=int(playlist.size())-1;if(!playlist.empty())currentTrack=playlist.front(); }
  catch(const std::exception& ex){std::cerr<<"Music theme unavailable: "<<ex.what()<<'\n';}
  if(!SDL_InitSubSystem(SDL_INIT_AUDIO)){std::cerr<<"Audio unavailable: "<<SDL_GetError()<<'\n';return;}
  audioInitialized=true;
  if(!MIX_Init()){std::cerr<<"Mixer unavailable: "<<SDL_GetError()<<'\n';return;}
  mixerInitialized=true;mixer=MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,nullptr);
  if(!mixer){std::cerr<<"Couldn't open audio: "<<SDL_GetError()<<'\n';return;}
  musicTrack=MIX_CreateTrack(mixer);soundTrack=MIX_CreateTrack(mixer);
  if(!musicTrack||!soundTrack){std::cerr<<"Couldn't create audio tracks: "<<SDL_GetError()<<'\n';return;}
  audioOpen=true;
  setMusicVolume(getConfig()->musicVolume.get());setSoundVolume(getConfig()->soundVolume.get());
  // Load on the owning thread: previously the loader mutated waves while playback read it.
  try { loadWaves(); }catch(const std::exception& ex){std::cerr<<"Some effects unavailable: "<<ex.what()<<'\n';}
  std::cerr<<"[audio] driver "<<SDL_GetCurrentAudioDriver()<<"; music/effect tracks ready\n";
  playMusic();
}

Sound::~Sound() {
  audioOpen=false;
  if(loaderThread)SDL_WaitThread(loaderThread,nullptr);
  if(musicTrack){MIX_SetTrackStoppedCallback(musicTrack,nullptr,nullptr);MIX_StopTrack(musicTrack,0);MIX_SetTrackAudio(musicTrack,nullptr);}
  if(soundTrack){MIX_StopTrack(soundTrack,0);MIX_SetTrackAudio(soundTrack,nullptr);}
  if(mixer)MIX_DestroyMixer(mixer);
  mixer=nullptr;musicTrack=nullptr;soundTrack=nullptr;rainTrack=nullptr;
  if(currentMusic)MIX_DestroyAudio(currentMusic);
  for(auto& item:waves)MIX_DestroyAudio(item.second);
  if(mixerInitialized)MIX_Quit();
  if(audioInitialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);
  if(soundPtr==this)soundPtr=nullptr;
}

/*
 *  Playback an Audio-Effect.
 *  Name is the Name of an Audiofile from sounds/ minus .wav
 *  and without trailing Numbers. If there are eg.
 *  beep1.wav, beep2.wav, beep3.wav
 *  playSound( "beep" ) would pick one of the three Files randomly
 */
void
Sound::playSound(const std::string& name) {
  if(!getConfig()->soundEnabled.get()) {
    return;
  }
  if(!audioOpen) {
    return;
  }

  chunks_t::size_type count = waves.count( name );
  if(count == 0) {
    std::cout << "Couldn't find audio file '" << name << "'" << std::endl;
    return;
  }

  chunks_t::iterator it = waves.find(name);
  for(int i = rand() % count; i > 0; i--) {
    it++;
  }

  MIX_SetTrackGain(soundTrack, .01f * getConfig()->soundVolume.get());
  MIX_SetTrackAudio(soundTrack, it->second);
  MIX_PlayTrack(soundTrack, 0);
}

void
Sound::playSound(const MapTile& tile) {
  if(tile.reportingConstruction &&
     tile.reportingConstruction->constructionGroup->group == GROUP_CHURCH) {
    const auto tick = SDL_GetTicks();
    if(lastBell == 0 || tick - lastBell >= 1800) {
      playSound("ChurchBell");
      lastBell = tick;
    }
    return;
  }
  ResourceGroup* resourceGroup;
  if(tile.reportingConstruction)
    resourceGroup = tile.reportingConstruction->soundGroup;
  else
    resourceGroup = tile.getTileResourceGroup();
  int count = resourceGroup->chunks.size();
  if(count)
    playASound(resourceGroup->chunks[rand() % count]);
}

void Sound::setRainAmbience(float intensity) {
  if(!audioOpen) return;
  const auto now = SDL_GetTicks();
  const float step = lastRainUpdate ? std::min<Uint64>(now - lastRainUpdate, 100) / 600.f : 0.f;
  lastRainUpdate = now;
  const float target = getConfig()->soundEnabled.get()
    ? intensity * getConfig()->soundVolume.get() * .01f : 0.f;
  if(target <= 0 && rainGain <= 0) return;
  if(!rainTrack) {
    const auto sample = waves.find("RainAmbience");
    if(sample == waves.end() || !sample->second) return;
    rainTrack = MIX_CreateTrack(mixer);
    if(!rainTrack) return;
    MIX_SetTrackAudio(rainTrack, sample->second);
  }
  rainGain += std::clamp(target - rainGain, -step, step);
  MIX_SetTrackGain(rainTrack, rainGain);
  if(rainGain <= 0) {
    if(MIX_TrackPlaying(rainTrack)) MIX_StopTrack(rainTrack, 0);
  } else if(!MIX_TrackPlaying(rainTrack)) {
    const auto props = SDL_CreateProperties();
    if(!props) return;
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_PlayTrack(rainTrack, props);
    SDL_DestroyProperties(props);
  }
}

void Sound::stopRainAmbience() {
  if(rainTrack) MIX_StopTrack(rainTrack, MIX_TrackMSToFrames(rainTrack, 250));
  rainGain = 0.f;
  lastRainUpdate = 0;
}

void Sound::playASound(MIX_Audio *chunk) {
  if(!getConfig()->soundEnabled.get())
    return;
  if(!audioOpen)
    return;
  MIX_SetTrackGain(soundTrack, .01f * getConfig()->soundVolume.get());
  MIX_SetTrackAudio(soundTrack, chunk);
  MIX_PlayTrack(soundTrack, 0);
}


/*
 * Get ID-String for a given Filename.
 */
std::string
Sound::getIdName(const std::string& filename)
{
    std::string::size_type pos = filename.find_first_of(".0123456789");

    return filename.substr(0, pos);
}

void
Sound::musicHalted(void *userdata, MIX_Track *) {
  // Audio callback never loads files or changes tracks. The main thread owns playback.
  if(auto* self=static_cast<Sound*>(userdata))self->musicAdvanceRequested.store(true);
}

void Sound::update() {
  if(musicAdvanceRequested.exchange(false)&&audioOpen&&getConfig()->musicEnabled.get())changeTrack(NEXT_OR_FIRST_TRACK);
}


/*
 * Change backround music.
 * Possible variables can be found from Sound.hpp: enum musicTransport
 * Currently there are three of them: NEXT_TRACK, NEXT_OR_FIRST_TRACK, PREV_TRACK
 * Sould be self-explanatory.
 */
void
Sound::changeTrack(MusicTransport command)
{

    //Something may gone wrong in the initialization:
    if(!audioOpen||playlist.empty())
        return;

    switch(command) {
        case NEXT_TRACK:
            if (currentTrack.trackNumber+1 <= totalTracks) {
                currentTrack=playlist[currentTrack.trackNumber+1];
                playMusic();
            }
            break;

        case NEXT_OR_FIRST_TRACK:
            if (currentTrack.trackNumber+1 <= totalTracks) {
                currentTrack=playlist[currentTrack.trackNumber+1];
                playMusic();
            } else {
                //Jump to the beginning
                currentTrack=playlist[0];
                playMusic();
            }
            break;

        case PREV_TRACK:
            if (currentTrack.trackNumber > 0) {
                currentTrack=playlist[currentTrack.trackNumber-1];
                playMusic();
            } else {
                //Jump to the beginning
                currentTrack=playlist[0];
                playMusic();
            }
            break;
    }

}


void
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
  if(!currentMusic){std::cerr<<"Couldn't load music: "<<SDL_GetError()<<'\n';return;}
  MIX_SetTrackAudio(musicTrack,currentMusic);MIX_SetTrackGain(musicTrack,getConfig()->musicVolume.get()*.01f);
  MIX_SetTrackStoppedCallback(musicTrack,musicHalted,this);MIX_PlayTrack(musicTrack,0);
}

void
Sound::enableMusic(bool enabled) {
  bool old = getConfig()->musicEnabled.get();
  getConfig()->musicEnabled.session = enabled;

  if(old == enabled)
    return;

  if(!audioOpen||!musicTrack)
    return;

  if(enabled) {
    playMusic();
  } else {
    if(MIX_TrackPlaying(musicTrack)) {
      MIX_SetTrackStoppedCallback(musicTrack,nullptr,nullptr);
      MIX_StopTrack(musicTrack,0);musicAdvanceRequested=false;
    }
  }
}

void
Sound::setMusicVolume(int vol)
{
    vol=std::clamp(vol,0,100);
    getConfig()->musicVolume.session = vol;
    float volvalue = .01f * vol;
    if(audioOpen&&musicTrack)MIX_SetTrackGain(musicTrack, volvalue);
}

void
Sound::setSoundVolume(int vol)
{
    vol=std::clamp(vol,0,100);
    getConfig()->soundVolume.session = vol;
    float volvalue = .01f * vol;
    if(audioOpen&&soundTrack)MIX_SetTrackGain(soundTrack, volvalue);
}


/** @file lincity-ng/Sound.cpp */
