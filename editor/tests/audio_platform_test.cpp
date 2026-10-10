#include "evengine/platform.hpp"
#include "evengine/studio.hpp"
#include <SDL3/SDL.h>
#include <iostream>
int main(int argc,char** argv){try{
 evengine::StudioGame g(argv[1]);auto p=evengine::createPlatform(g.atlas,"EveNGINE audio check");if(!p->audioAvailable())throw std::runtime_error("Dummy audio device missing");
 p->setMusic(g.music(),.25f);auto &sound=g.audioAssets.at("amber-chime");for(int i=0;i<24;i++){p->playSound(sound,.3f);p->idle();}for(int i=0;i<350;i++)p->idle();p->setMusic(g.music(),0);p->setMusic(nullptr,0);p->setMusic(g.music(),.5f);for(int i=0;i<100;i++)p->idle();p->setMusic(nullptr,0);
 // Exercise SDL's actual queued PCM conversion without a playback device.
 auto clip=g.music();SDL_AudioSpec source{SDL_AUDIO_S16,clip->channels,clip->rate},dest{SDL_AUDIO_F32,1,clip->rate};auto stream=SDL_CreateAudioStream(&source,&dest);if(!stream)throw std::runtime_error(SDL_GetError());
 if(!SDL_PutAudioStreamData(stream,clip->samples.data(),int(clip->samples.size()*2))||!SDL_FlushAudioStream(stream))throw std::runtime_error(SDL_GetError());std::vector<float> data(clip->samples.size());int bytes=SDL_GetAudioStreamData(stream,data.data(),int(data.size()*4));SDL_DestroyAudioStream(stream);if(bytes!=int(data.size()*4))throw std::runtime_error("PCM conversion lost frames");
 float error=0;for(size_t i=0;i<data.size();i++)error=std::max(error,std::abs(data[i]-clip->samples[i]/32768.f));if(error>1e-6)throw std::runtime_error("PCM conversion differs");
 std::cout<<"Dummy audio: music refill, gain changes, stop/restart, 24 overlapping effects, clean destruction; "<<data.size()<<" converted frames match PCM\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
