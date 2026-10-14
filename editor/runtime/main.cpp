#include "studio.hpp"
#include "lota/platform.hpp"
#include <iostream>
#include <filesystem>

int main(int argc,char **argv){
 try{
  auto base=std::filesystem::path(lota::defaultProjectPath()).parent_path().parent_path().parent_path();
  std::string path=(base/"project.lota").string(),capture,scene;
  int ticks=0;bool check=false;
  for(int i=1;i<argc;i++){
   std::string argument=argv[i];
   auto value=[&](){if(++i>=argc)throw std::runtime_error("Missing value");return std::string(argv[i]);};
   if(argument=="--project")path=value();
   else if(argument=="--scene")scene=value();
   else if(argument=="--capture")capture=value();
   else if(argument=="--ticks")ticks=std::stoi(value());
   else if(argument=="--validate")check=true;
   else throw std::runtime_error("Unknown option");
  }
  lota::StudioGame game(path);
  if(!scene.empty()){
   if(!game.project.scenes.count(scene))throw std::runtime_error("Unknown playtest map");
   game.scene=scene;game.feet=game.project.scenes.at(scene).spawn;
  }
  if(check||!capture.empty()){
   lota::Input input;input.set(lota::Action::Right,true);
   for(int i=0;i<ticks;i++){game.update(input,lota::Tick);input.consume();}
   if(!capture.empty()){lota::SoftwareRenderer renderer;renderer.render(game.draw(),game.atlas);renderer.writePPM(capture);}
   std::cout<<"Project validated: "<<game.project.scenes.size()<<" maps, "<<game.clips.size()<<" clips\n";
   return 0;
  }
  auto platform=lota::createPlatform(game.atlas,game.project.title);
  if(!game.audioAssets.empty()&&!platform->audioAvailable())std::cerr<<"Audio device unavailable; continuing silently.\n";
  game.beginSession(platform->savePath(game.project.id),!scene.empty());
  platform->configureInput(game.keyBindings,game.buttonBindings);
  platform->confirmWindowClose(true);
  bool fullscreen=false,suppressCapture=false;
  lota::Input input;
  double previous=platform->seconds(),accumulator=0;
  while(!game.quit&&platform->poll(input)){
   if(platform->takeCloseRequest())game.requestWindowClose();
   if(game.quit)break;
   game.controllerMode=platform->usingGamepad();
   auto key=platform->takeKeyPress();
   if(!game.pendingBinding.empty()&&!key.empty()){
    game.captureBinding(key);
    platform->configureInput(game.keyBindings,game.buttonBindings);
    // A captured key must be released before it can confirm another menu action.
    input=lota::Input{};
    for(auto& binding:game.keyBindings)if(binding.second==key)input.held[size_t(binding.first)]=true;
    suppressCapture=true;
   }
   if(suppressCapture){
    input.consume();
    if(std::none_of(input.held.begin(),input.held.end(),[](bool held){return held;}))suppressCapture=false;
   }
   if(game.fullscreen!=fullscreen){
    if(platform->setFullscreen(game.fullscreen))fullscreen=game.fullscreen;
    else{game.fullscreen=fullscreen;game.toast="Fullscreen is unavailable.";game.toastTime=3;}
   }
   double now=platform->seconds();
   game.frameMs=float((now-previous)*1000);
   accumulator+=std::clamp(now-previous,0.,.1);previous=now;
   while(accumulator>=lota::Tick){game.update(input,lota::Tick);input.consume();accumulator-=lota::Tick;}
   platform->setMusic(game.music(),game.musicVolume*game.userMusic);
   for(auto& id:game.soundEvents)platform->playSound(game.audioAssets.at(id),game.sfxVolume*game.userEffects);
   game.soundEvents.clear();platform->present(game.draw());platform->idle();
  }
  return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
