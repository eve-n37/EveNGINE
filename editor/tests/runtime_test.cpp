#include "lota/studio.hpp"
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <fstream>
using namespace lota;
int main(int argc,char **argv){try{if(argc<3)throw std::runtime_error("Need project and output folder");StudioGame g(argv[1]);int checks=0;auto check=[&](bool ok,const char *n){if(!ok)throw std::runtime_error(n);checks++;std::cout<<"OK "<<n<<'\n';};Input in;auto start=g.feet;in.set(Action::Right,true);for(int i=0;i<60;i++){g.update(in,Tick);in.consume();}check(std::abs(g.feet.x-start.x-64)<.01,"fixed-step walking speed");check(g.direction=="east","direction follows movement");float walkClock=g.clock;g.feet=start;g.clock=0;in.set(Action::Run,true);for(int i=0;i<60;i++){g.update(in,Tick);in.consume();}check(std::abs(g.feet.x-start.x-108.8f)<.02,"running speed");check(std::abs(g.clock/walkClock-1.7)<.001,"running animation cadence");
 in={};in.set(Action::Left,true);g.feet={23,168};for(int i=0;i<60;i++){g.update(in,Tick);in.consume();}check(g.feet.x>=21,"wall collision uses shared engine body");
 auto &s=g.project.scenes.at(g.scene);s.cells[10*s.width+20]='#';g.feet={300,168};in={};in.set(Action::Right,true);for(int i=0;i<60;i++){g.update(in,Tick);in.consume();}check(g.feet.x<=315.001f,"painted collision stops player");s.cells[10*s.width+20]='.';
 g.feet={440,168};for(int i=0;i<60;i++){g.update(in,Tick);in.consume();}check(g.scene=="grove","exit transitions to destination");check(g.feet.x>=40&&g.feet.x<100,"exit arrival preserved");
 g.scene="meadow";g.feet={192+8,140+8};in={};g.update(in,Tick);check(g.score==1,"collectible behaviour");g.update(in,Tick);check(g.score==1,"collectible only awarded once");
 std::string error,save=(std::filesystem::path(argv[2])/"studio-save.dat").string();check(g.save(save,error),"atomic progress save");g.feet={100,200};g.score=20;check(g.load(save,error)&&g.score==1&&g.feet.x==200,"save reload restores progress");
 g.feet={370,85};in={};in.set(Action::Right,true);start=g.feet;for(int i=0;i<30;i++){g.update(in,Tick);in.consume();}check(std::abs(g.feet.x-start.x-19.2f)<.02,"painted water slows movement");
 g.scene="grove";g.feet={258,145};in={};in.set(Action::Interact,true);g.update(in,Tick);check(g.message=="A quiet place to start your next game.","sign interaction");
 auto attack=g.clips[2];attack.state="attack";attack.direction="east";attack.frames.resize(2);for(auto &f:attack.frames)f.ms=100;g.clips.push_back(attack);g.direction="east";in={};in.set(Action::Attack,true);g.update(in,Tick);in.consume();check(g.attackClock>=0&&g.pose().x==attack.frames[0].source.x,"authored attack frame plays");in.set(Action::Left,true);g.update(in,Tick);in.consume();check(g.direction=="east","attack direction held until clip completes");for(int i=0;i<13;i++){g.update(in,Tick);in.consume();}check(g.attackClock<0,"attack completes once");
 in={};in.set(Action::Debug,true);g.update(in,Tick);check(g.debug,"debug view toggle");SoftwareRenderer r;r.render(g.draw(),g.atlas);r.writePPM((std::filesystem::path(argv[2])/"studio-runtime.ppm").string());check(r.pixels.size()==Width*Height,"shared software renderer produces frame");
 StudioGame invalid(argv[1]);invalid.project.scenes.at(invalid.scene).cells[9*32+12]='#';auto previous=invalid.feet;check(!invalid.load(save,error)&&invalid.feet.x==previous.x,"invalid saved position rejected without mutation");
 std::ifstream f(argv[1]);std::string text((std::istreambuf_iterator<char>(f)),{});auto at=text.find("\"format\": \"lota-studio\"");check(at!=std::string::npos,"test fixture format");text.replace(at,23,"\"format\": \"other-data\"");auto bad=(std::filesystem::path(argv[2])/"bad.lota").string();std::ofstream(bad)<<text;bool rejected=false;try{StudioGame badGame(bad);}catch(...){rejected=true;}check(rejected,"unsupported project rejected");
 std::cout<<checks<<" runtime checks passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
