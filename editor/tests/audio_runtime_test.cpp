#include "evengine/studio.hpp"
#include <fstream>
#include <iostream>
using namespace evengine;
int main(int argc,char** argv){try{
 StudioGame g(argv[1]);int count=0;auto check=[&](bool ok,const char* name){if(!ok)throw std::runtime_error(name);std::cout<<"OK "<<name<<'\n';count++;};
 check(g.audioAssets.size()==2,"embedded WAV assets load");auto music=g.music();check(music&&music->rate==16000&&music->channels==1&&music->samples.size()==64000,"music PCM rate and frame count");
 g.feet={200,148};Input in;g.update(in,Tick);check(g.soundEvents.size()==1&&g.soundEvents[0]=="amber-chime","collect sound emitted once");g.update(in,Tick);check(g.soundEvents.size()==1,"collected item cannot repeat sound");g.soundEvents.clear();
 g.audioCues["attack"]="amber-chime";in.set(Action::Attack,true);g.update(in,Tick);in.consume();g.update(in,Tick);check(g.soundEvents.size()==1,"attack sound is edge triggered");
 g.soundEvents.clear();g.audioCues["interact"]="amber-chime";g.scene="grove";g.feet={258,145};in={};in.set(Action::Interact,true);g.update(in,Tick);in.consume();g.update(in,Tick);check(g.soundEvents.size()==1,"sign sound is edge triggered");
 g.soundEvents.clear();g.scene="meadow";g.feet={470,168};in={};g.portalCooldown=0;g.update(in,Tick);check(g.scene=="grove"&&g.soundEvents.size()==1,"exit emits sound after transition");check(g.music()==music,"maps sharing music preserve clip identity");g.musicByScene["grove"]="";check(g.music()==nullptr,"map without music stops track");
 for(int i=0;i<100;i++)g.cue("collect");check(g.soundEvents.size()==32,"pending audio events are bounded");
 auto j=Json::load(argv[1]);auto bytes=decodeBase64(j["audio"]["assets"][0]["data"].text());auto reject=[&](const std::vector<uint8_t>& b){try{decodeWavBytes(b);return false;}catch(...){return true;}};
 auto b=bytes;b[4]=0;check(reject(b),"bad RIFF extent rejected");b=bytes;b[20]=3;check(reject(b),"float WAV rejected");b=bytes;b[32]=1;check(reject(b),"bad sample alignment rejected");b=bytes;b.resize(b.size()-2);check(reject(b),"truncated samples rejected");b=bytes;b[22]=2;b[28]=0;b[29]=250;b[30]=0;b[31]=0;b[32]=4;check(decodeWavBytes(b).channels==2,"stereo WAV supported");
 bool bad64=false;try{decodeBase64("AAAA=bad");}catch(...){bad64=true;}check(bad64,"malformed base64 rejected");
 std::ifstream fixture(argv[1]);auto text=std::string(std::istreambuf_iterator<char>(fixture),{}); // fixture edits use exact replacement below
 auto badProject=[&](const std::string& from,const std::string& to){auto source=text;auto at=source.find(from);if(at==std::string::npos)throw std::runtime_error("Fixture replacement absent");source.replace(at,from.size(),to);std::ofstream out(argv[2]);out<<source;out.close();try{StudioGame invalid(argv[2]);return false;}catch(...){return true;}};
 check(badProject("\"collect\": \"amber-chime\"","\"collect\": \"missing\""),"unknown audio cue reference rejected");
 check(badProject("\"music\": \"mosslight-music\"","\"music\": \"missing\""),"unknown map music rejected");
 check(badProject("\"musicVolume\": 0.5","\"musicVolume\": 2"),"out-of-range gain rejected");
 check(badProject("\"pebble-path\"","\"invalid stamp id\""),"bad stamp metadata rejected");
 std::cout<<count<<" audio runtime checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
