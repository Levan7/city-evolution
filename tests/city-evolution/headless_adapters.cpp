// SPDX-License-Identifier: GPL-2.0-or-later
// Headless test adapters: only display/audio/config are replaced, simulation is unchanged.
#include "lincity-ng/Config.hpp"
#include "lincity-ng/Mps.hpp"
#include "lincity-ng/Sound.hpp"
template<typename T> Config::Option<T>::Option() = default;
template<typename T> Config::Option<T>::Option(const T& v) : default_(v) { }
template<typename T> const T& Config::Option<T>::get() const { if(session)return *session;if(config)return *config;return *default_; }
template class Config::Option<bool>;
template class Config::Option<int>;
template class Config::Option<std::string>;
template class Config::Option<std::filesystem::path>;
int simDelay=100;
Config::Config() { carsEnabled.default_=false; }
Config::~Config()=default;
Config* getConfig() { static Config config;return &config; }
Sound* getSound() { return nullptr; }
void Sound::playASound(MIX_Audio*) { }
void Mps::addBlank() { }
void Mps::add_s(const std::string&) { }
void Mps::add_fp(int,double) { }
void Mps::add_f(double) { }
void Mps::add_sf(const std::string&,double) { }
void Mps::add_d(int) { }
void Mps::add_ss(const std::string&,const std::string&) { }
void Mps::add_sd(const std::string&,int) { }
void Mps::add_ssd(const std::string&,const std::string&,int) { }
void Mps::add_sfp(const std::string&,double) { }
void Mps::add_sdd(const std::string&,int,int) { }
void Mps::add_sddp(const std::string&,int,int) { }
void Mps::add_tsddp(const std::string&,const std::string&,int,int) { }
void Mps::add_ttt(const std::string&,const std::string&,const std::string&) { }
