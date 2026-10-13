#include "lota/studio.hpp"
#include <stdexcept>
#include <set>
#include <fstream>
#include <iterator>
#include <iomanip>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace lota {
namespace {
void require(bool ok,const std::string &s){if(!ok)throw std::runtime_error(s);}
int integer(const Json &v,int low,int high,const std::string &name){require(v.type==Json::Number&&std::floor(v.number)==v.number&&v.number>=low&&v.number<=high,"Invalid "+name);return int(v.number);}
float real(const Json &v,float lo,float hi,const std::string &name){require(v.type==Json::Number&&std::isfinite(v.number)&&v.number>=lo&&v.number<=hi,"Invalid "+name);return float(v.number);}
std::string id(const Json &v){auto s=v.text();require(!s.empty()&&s.size()<=64&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos,"Invalid identifier");return s;}
std::string text(const Json &v,size_t max){require(v.type==Json::String&&v.string.size()<=max,"Invalid text");return v.string;}
int hex(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("Invalid hex pixel");}
void array(const Json &v,size_t lo,size_t hi,const char *name){require(v.type==Json::Array&&v.array.size()>=lo&&v.array.size()<=hi,std::string("Invalid ")+name);}
std::vector<int> grid(const Json &v,int count,int lo,int hi){array(v,count,count,"map layer");std::vector<int> a;for(auto &n:v.array)a.push_back(integer(n,lo,hi,"layer cell"));return a;}
}
StudioGame::StudioGame(const std::string &path){
 auto j=Json::load(path);require(j["format"].text()=="lota-studio"&&j["version"].integer()==1,"Unsupported studio project");project.id=id(j["id"]);project.title=text(j["title"],80);require(!project.title.empty(),"Missing title");project.mode="studio";project.start=id(j["start"]);
 array(j["palette"],2,256,"palette");std::vector<Color> palette;for(auto &v:j["palette"].array){auto s=v.text();require(s.size()==7||s.size()==9,"Invalid colour");require(s[0]=='#',"Invalid colour");Color c{uint8_t(hex(s[1])*16+hex(s[2])),uint8_t(hex(s[3])*16+hex(s[4])),uint8_t(hex(s[5])*16+hex(s[6])),255};if(s.size()==9)c.a=uint8_t(hex(s[7])*16+hex(s[8]));palette.push_back(c);}
 array(j["tiles"],1,64,"tiles");array(j["sprites"],1,256,"sprites");atlas.width=1024;atlas.height=256;while(atlas.height<192+int((j["tiles"].array.size()+j["sprites"].array.size()+15)/16)*64)atlas.height*=2;atlas.pixels.resize(size_t(atlas.width)*atlas.height,{0,0,0,0});std::map<std::string,Rect> sources;int slot=0;
 auto pack=[&](const Json &a,bool tile){auto name=id(a["id"]);require(!sources.count(name),"Duplicate asset");int w=integer(a["width"],1,64,"asset width"),h=integer(a["height"],1,64,"asset height");if(tile)require(w==16&&h==16,"Tile must be 16 pixels");std::string pixels=text(a["pixels"],8192);require(int(pixels.size())==w*h*2,"Pixel count mismatch");int ax=(slot%16)*64,ay=192+(slot/16)*64;slot++;require(ay+h<=atlas.height,"Atlas full");Rect source{float(ax),float(ay),float(w),float(h)};sources[name]=source;for(int y=0;y<h;y++)for(int x=0;x<w;x++){int i=(y*w+x)*2,n=hex(pixels[i])*16+hex(pixels[i+1]);require(n<int(palette.size()),"Pixel palette index");atlas.pixels[size_t(ay+y)*atlas.width+ax+x]=palette[n];}
 auto &c=a["collider"];Rect box{real(c["x"],0,w,"collider x"),real(c["y"],0,h,"collider y"),real(c["w"],0,w,"collider width"),real(c["h"],0,h,"collider height")};require(box.x+box.w<=w&&box.y+box.h<=h,"Collider exceeds sprite");if(!tile)project.sprites[name]={source,box,{}};return source;};
 for(auto &t:j["tiles"].array){auto r=pack(t,true);tileSources.push_back(r);}for(auto &s:j["sprites"].array)pack(s,false);
 // Font is a fixed 192 x 24 RGBA strip bundled with the executable's resources.
 // The caller loads it below through a sibling PAM; absent font leaves HUD blank.
 auto pos=path.find_last_of("/\\");auto fontPath=(pos==std::string::npos?"":path.substr(0,pos+1))+"font.pam";std::ifstream ff(fontPath);if(ff){auto font=Atlas::load(fontPath);require(font.width==192&&font.height==24,"Invalid font strip");for(int y=0;y<24;y++)for(int x=0;x<192;x++)atlas.pixels[size_t(128+y)*atlas.width+x]=font.pixels[size_t(y)*192+x];}
 if(j.has("items")){array(j["items"],0,64,"item definitions");for(auto &v:j["items"].array){auto name=id(v["id"]);StudioItem item{text(v["name"],160),id(v["icon"])};require(!item.name.empty()&&project.sprites.count(item.icon)&&items.emplace(name,item).second,"Invalid or duplicate item definition");}}
 project.tiles['.']={tileSources[0],false};project.tiles['#']={tileSources[0],true};
 array(j["scenes"],1,64,"scenes");for(auto &m:j["scenes"].array){Scene s;s.id=id(m["id"]);s.title=text(m["title"],80);s.width=integer(m["width"],1,256,"map width");s.height=integer(m["height"],1,256,"map height");int n=s.width*s.height;StudioLayer l;l.base=grid(m["base"],n,0,int(tileSources.size())-1);l.detail=grid(m["detail"],n,-1,int(tileSources.size())-1);l.water=grid(m["water"],n,0,1);auto collision=grid(m["collision"],n,0,1);for(int v:collision)s.cells.push_back(v?'#':'.');auto point=[&](const Json &p){return Vec2{real(p["x"],0,s.width*16-.001f,"point x"),real(p["y"],0,s.height*16-.001f,"point y")};};s.spawn=point(m["spawn"]);
 array(m["objects"],0,4096,"objects");std::set<std::string> ids;for(auto &a:m["objects"].array){Object o;o.id=id(a["id"]);require(ids.insert(o.id).second,"Duplicate object");o.sprite=id(a["sprite"]);require(project.sprites.count(o.sprite),"Unknown sprite");o.kind=a["kind"].text();require(o.kind=="decoration"||o.kind=="collectible"||o.kind=="sign"||o.kind=="npc"||o.kind=="chest"||o.kind=="door"||o.kind=="switch","Unknown object kind");o.text=text(a["text"],720);o.position=point(a);StudioInteraction data;data.kind=o.kind;
 auto ref=[&](const char* key){if(!a.has(key))return std::string();auto name=text(a[key],64);if(!name.empty())id(a[key]);return name;};
 data.openSprite=ref("openSprite");require(data.openSprite.empty()||project.sprites.count(data.openSprite),"Missing alternate sprite");data.item=ref("item");data.requiredItem=ref("requiredItem");require((data.item.empty()||items.count(data.item))&&(data.requiredItem.empty()||items.count(data.requiredItem)),"Unknown inventory item");
 if(a.has("quantity"))data.quantity=integer(a["quantity"],1,99,"item quantity");for(auto key:{"consumeKey","switchOnly"})if(a.has(key))require(a[key].type==Json::Bool,"Invalid interaction toggle");data.consumeKey=a["consumeKey"].boolean();data.switchOnly=a["switchOnly"].boolean();
 if(o.kind=="chest")require(!data.item.empty(),"Chest needs a reward item");
 if(o.kind=="door")require(!(data.switchOnly&&!data.requiredItem.empty()),"Switch-only door cannot require a key");
 if(o.kind=="npc"){data.speaker=text(a["speaker"],160);if(!a.has("dialogue")){array(a["pages"],1,16,"dialogue pages");for(auto &page:a["pages"].array){auto value=text(page,720);require(!value.empty(),"Empty dialogue page");data.pages.push_back(value);}}}
 if(o.kind=="switch"){array(a["targets"],1,32,"switch targets");std::set<std::string> seen;for(auto &v:a["targets"].array){auto name=id(v);require(seen.insert(name).second,"Duplicate switch target");data.targets.push_back(name);}}
 interactions[s.id+":"+o.id]=std::move(data);s.objects.push_back(o);}
 array(m["portals"],0,512,"exits");for(auto &a:m["portals"].array){Portal p;p.bounds={real(a["x"],0,s.width*16,"exit x"),real(a["y"],0,s.height*16,"exit y"),real(a["w"],1,s.width*16,"exit width"),real(a["h"],1,s.height*16,"exit height")};require(p.bounds.x+p.bounds.w<=s.width*16&&p.bounds.y+p.bounds.h<=s.height*16,"Exit outside map");p.target=id(a["target"]);p.arrival={real(a["arrival"]["x"],0,4096,"arrival x"),real(a["arrival"]["y"],0,4096,"arrival y")};s.portals.push_back(p);}require(project.scenes.emplace(s.id,s).second,"Duplicate map");layers[s.id]=std::move(l);}
 require(project.scenes.count(project.start),"Missing starting map");for(auto &entry:project.scenes)for(auto &o:entry.second.objects)if(o.kind=="switch")for(auto &target:interactions.at(entry.first+":"+o.id).targets){auto i=interactions.find(entry.first+":"+target);require(i!=interactions.end()&&i->second.kind=="door","Switch target must be a door on the same map");}for(auto &entry:project.scenes){auto &s=entry.second;require(!blocked(project,s,body(s.spawn)),"Blocked spawn in "+s.title);for(auto &p:s.portals){require(project.scenes.count(p.target),"Missing destination");require(!blocked(project,project.scenes.at(p.target),body(p.arrival)),"Blocked exit arrival");}}
 array(j["animations"],1,64,"animations");std::set<std::string> keys;for(auto &a:j["animations"].array){StudioClip clip;clip.state=a["state"].text();clip.direction=a["direction"].text();require(clip.state=="idle"||clip.state=="walk"||clip.state=="attack","Unknown animation state");require(clip.direction=="south"||clip.direction=="north"||clip.direction=="east"||clip.direction=="west","Unknown direction");require(keys.insert(clip.state+clip.direction).second,"Duplicate animation");array(a["frames"],1,64,"frames");for(auto &f:a["frames"].array){auto name=id(f["sprite"]);require(project.sprites.count(name),"Unknown animation sprite");clip.frames.push_back({sources.at(name),integer(f["ms"],16,5000,"frame duration")});}clips.push_back(clip);}for(auto d:{"south","north","east","west"})require(keys.count(std::string("walk")+d),"Missing walk animation");speed=real(j["player"]["speed"],1,512,"walk speed");runMultiplier=real(j["player"]["runMultiplier"],1,4,"run multiplier");
 if(j.has("audio")) {auto &a=j["audio"];array(a["assets"],0,32,"audio assets");musicVolume=real(a["musicVolume"],0,1,"music volume");sfxVolume=real(a["sfxVolume"],0,1,"effects volume");require(a["cues"].type==Json::Object,"Invalid audio cues");size_t total=0;
  for(auto &v:a["assets"].array) {auto name=id(v["id"]);require(!audioAssets.count(name),"Duplicate audio asset");auto label=text(v["name"],320);require(!label.empty(),"Missing sound name");auto bytes=decodeBase64(text(v["data"],5592408));total+=bytes.size();require(total<=12*1024*1024,"Audio exceeds 12 MiB");audioAssets.emplace(name,decodeWavBytes(bytes));}
  for(auto &entry:a["cues"].object) {auto key=entry.first;require(key=="collect"||key=="attack"||key=="exit"||key=="interact"||key=="chest"||key=="door"||key=="switch","Unknown sound cue");auto name=text(entry.second,64);require(name.empty()||audioAssets.count(name),"Unknown cue clip");audioCues[key]=name;}
 }
 for(auto &m:j["scenes"].array)if(m.has("music")){auto name=text(m["music"],64);require(name.empty()||audioAssets.count(name),"Unknown map music");musicByScene[id(m["id"])]=name;}
 if(j.has("stamps")){array(j["stamps"],0,64,"stamps");std::set<std::string> stampIds;for(auto &st:j["stamps"].array){require(stampIds.insert(id(st["id"])).second,"Duplicate stamp");require(!text(st["name"],320).empty(),"Missing stamp name");int w=integer(st["width"],1,64,"stamp width"),h=integer(st["height"],1,64,"stamp height");auto &ls=st["layers"];require(ls.type==Json::Object&&!ls.object.empty(),"Invalid stamp layers");for(auto &entry:ls.object){auto k=entry.first;require(k=="base"||k=="detail"||k=="collision"||k=="water","Unknown stamp layer");array(entry.second,w*h,w*h,"stamp grid");for(auto &v:entry.second.array){if(k=="base"||k=="detail"){if(k=="detail"&&v.type==Json::Null)continue;auto n=id(v);bool tile=false;for(auto &t:j["tiles"].array)if(t["id"].text()==n)tile=true;require(tile,"Missing stamp tile");}else integer(v,0,1,"stamp flag");}}}}
 loadNarrative(j);
 scene=project.start;feet=project.scenes.at(scene).spawn;
}
const AudioClip* StudioGame::music() const {auto m=musicByScene.find(scene);if(m==musicByScene.end()||m->second.empty())return nullptr;auto a=audioAssets.find(m->second);return a==audioAssets.end()?nullptr:&a->second;}
void StudioGame::cue(const std::string& name){auto c=audioCues.find(name);if(c!=audioCues.end()&&!c->second.empty()&&soundEvents.size()<32)soundEvents.push_back(c->second);}
Rect StudioGame::pose() const {std::string state=attackClock>=0?"attack":clock>0?"walk":"idle";auto find=[&](std::string s)->const StudioClip*{for(auto &c:clips)if(c.state==s&&c.direction==direction)return &c;return nullptr;};auto c=find(state);if(!c)c=find("walk");require(c,"Missing pose");float t=attackClock>=0?attackClock:clock;int total=0;for(auto &f:c->frames)total+=f.ms;int ms=int(t*1000)%total;if(state=="idle")ms=0;for(auto &f:c->frames){if(ms<f.ms)return f.source;ms-=f.ms;}return c->frames[0].source;}

Scene StudioGame::collisionScene(const std::string& name,const std::vector<std::string>& gone,const std::set<std::string>& active) const {
 Scene s=project.scenes.at(name);s.objects.erase(std::remove_if(s.objects.begin(),s.objects.end(),[&](const Object& o){auto key=name+":"+o.id;return std::find(gone.begin(),gone.end(),key)!=gone.end()||(o.kind=="door"&&active.count(key));}),s.objects.end());return s;
}
std::string StudioGame::visibleSprite(const Object& o) const {auto key=scene+":"+o.id;auto i=interactions.find(key);return activated.count(key)&&i!=interactions.end()&&!i->second.openSprite.empty()?i->second.openSprite:o.sprite;}
bool StudioGame::giveItem(const std::string& name,int quantity){if(name.empty())return true;auto current=inventory.find(name);int count=current==inventory.end()?0:current->second;if(count+quantity>999){toast="Your bag is full for this item.";toastTime=3;return false;}inventory[name]=count+quantity;return true;}
void StudioGame::interact(const Object& o){auto key=scene+":"+o.id;auto &data=interactions.at(key);auto say=[&](const std::string& s){toast=s;toastTime=3;};
 if(o.kind=="npc"){toastTime=0;questEvent("talk",key);journalOpen=false;inventoryOpen=false;dialogueObject=key;dialogueSpeaker=data.speaker;dialoguePage=0;dialogueNode=data.dialogueStart;dialogueChoice=0;cue("interact");return;}
 if(o.kind=="sign"){say(o.text);cue("interact");return;}
 if(o.kind=="chest"){if(activated.count(key)){say("The chest is empty.");return;}if(!giveItem(data.item,data.quantity))return;activated.insert(key);say("Found "+items.at(data.item).name+" x"+std::to_string(data.quantity));cue("chest");return;}
 if(o.kind=="door"){if(activated.count(key))return;if(data.switchOnly){say("A nearby switch opens this gate.");return;}
  if(!data.requiredItem.empty()){auto i=inventory.find(data.requiredItem);if(i==inventory.end()||i->second<1){say("Need "+items.at(data.requiredItem).name);return;}if(data.consumeKey){if(--i->second==0)inventory.erase(i);}}
  activated.insert(key);say("Door opened.");cue("door");return;}
 if(o.kind=="switch"){if(activated.count(key)){say("The switch is already latched.");return;}activated.insert(key);for(auto &id:data.targets)activated.insert(scene+":"+id);say("The linked gate is open.");cue("switch");}
}
void StudioGame::update(const Input &in,float dt){
 if(in.hit(Action::Debug))debug=!debug;
 if(in.hit(Action::Quit)){if(!dialogueObject.empty())dialogueObject.clear();else if(inventoryOpen)inventoryOpen=false;else if(journalOpen)journalOpen=false;else quit=true;return;}
 toastTime=std::max(0.f,toastTime-dt);
 if(!dialogueObject.empty()){
  if(in.hit(Action::Attack))dialogueObject.clear();
  else if(!dialogueNode.empty()){auto choices=availableChoices();if(!choices.empty()){dialogueChoice=std::clamp(dialogueChoice,0,int(choices.size())-1);if(in.hit(Action::Down))dialogueChoice=(dialogueChoice+1)%int(choices.size());if(in.hit(Action::Up))dialogueChoice=(dialogueChoice+int(choices.size())-1)%int(choices.size());}if(in.hit(Action::Interact)){if(choices.empty())dialogueObject.clear();else chooseDialogue(size_t(dialogueChoice));}}
  else if(in.hit(Action::Interact)){auto &pages=interactions.at(dialogueObject).pages;if(++dialoguePage>=int(pages.size()))dialogueObject.clear();}clock=0;return;
 }
 if(in.hit(Action::Journal)){journalOpen=!journalOpen;inventoryOpen=false;journalPage=0;}
 if(journalOpen){int pages=std::max(1,int(questProgress.size()));if(in.hit(Action::Right))journalPage=(journalPage+1)%pages;if(in.hit(Action::Left))journalPage=(journalPage+pages-1)%pages;clock=0;return;}
 if(in.hit(Action::Menu)){inventoryOpen=!inventoryOpen;inventoryPage=0;}
 if(inventoryOpen){int pages=std::max(1,int((inventory.size()+7)/8));if(in.hit(Action::Right))inventoryPage=(inventoryPage+1)%pages;if(in.hit(Action::Left))inventoryPage=(inventoryPage+pages-1)%pages;clock=0;return;}
 questEvent("visit",scene);for(auto& key:activated)questEvent("activate",key);
 auto &s=project.scenes.at(scene);auto d=normalized({float(in.down(Action::Right))-float(in.down(Action::Left)),float(in.down(Action::Down))-float(in.down(Action::Up))});bool moving=d.x!=0||d.y!=0;
 if(moving){if(attackClock<0)direction=std::abs(d.x)>std::abs(d.y)?(d.x>0?"east":"west"):(d.y>0?"south":"north");clock+=dt*(in.down(Action::Run)?runMultiplier:1);}else clock=0;
 if(in.hit(Action::Attack))cue("attack");if(in.hit(Action::Attack)&&attackClock<0)for(auto &c:clips)if(c.state=="attack"&&c.direction==direction)attackClock=0;
 if(attackClock>=0){attackClock+=dt;for(auto &c:clips)if(c.state=="attack"&&c.direction==direction){float total=0;for(auto &f:c.frames)total+=f.ms/1000.f;if(attackClock>=total)attackClock=-1;}}
 auto factor=in.down(Action::Run)?runMultiplier:1;int tx=std::clamp(int(feet.x/16),0,s.width-1),ty=std::clamp(int(feet.y/16),0,s.height-1);if(layers.at(scene).water[ty*s.width+tx])factor*=.6f;
 auto collision=collisionScene(scene,removed,activated);feet=moveBody(project,collision,feet,{d.x*speed*factor*dt,d.y*speed*factor*dt});message.clear();const Object* nearest=nullptr;float nearestDistance=33;
 for(auto &o:s.objects){auto key=scene+":"+o.id;if(std::find(removed.begin(),removed.end(),key)!=removed.end())continue;auto r=project.sprites.at(o.sprite).source;float dist=std::hypot(feet.x-(o.position.x+r.w/2),feet.y-(o.position.y+r.h/2));
  auto data=interactions.find(key);if(o.kind=="collectible"&&dist<16){if(data!=interactions.end()&&!giveItem(data->second.item,data->second.quantity))continue;removed.push_back(key);score++;cue("collect");if(data!=interactions.end()&&!data->second.item.empty()){toast="Picked up "+items.at(data->second.item).name;toastTime=3;}continue;}
  if(o.kind!="decoration"&&o.kind!="collectible"&&dist<nearestDistance&&!(o.kind=="door"&&activated.count(key))){nearest=&o;nearestDistance=dist;}
 }
 if(nearest){message=nearest->kind=="npc"?"E TALK":nearest->kind=="chest"?"E OPEN CHEST":nearest->kind=="door"?"E OPEN DOOR":nearest->kind=="switch"?"E USE SWITCH":"E TO READ";
  if(nearest->kind=="sign"&&in.down(Action::Interact))message=nearest->text;
  if(in.hit(Action::Interact)){interact(*nearest);for(auto& key:activated)questEvent("activate",key);}
 }
 if(!dialogueObject.empty())return;
 portalCooldown=std::max(0.f,portalCooldown-dt);if(portalCooldown==0)for(auto &p:s.portals)if(overlaps(body(feet),p.bounds)){scene=p.target;feet=p.arrival;cue("exit");portalCooldown=.6f;clock=0;toastTime=0;questEvent("visit",scene);break;}
}
Canvas StudioGame::draw() const {Canvas c;c.clear({31,47,42});auto &s=project.scenes.at(scene);auto &l=layers.at(scene);auto camera=cameraFor(s,feet);int left=int(camera.x)/16,top=int(camera.y)/16;for(int y=top;y<std::min(s.height,top+17);y++)for(int x=left;x<std::min(s.width,left+27);x++){int i=y*s.width+x;Vec2 pos{float(x*16)-camera.x,float(y*16)-camera.y};c.sprite(tileSources[l.base[i]],pos);if(l.detail[i]>=0)c.sprite(tileSources[l.detail[i]],pos);if(debug&&s.at(x,y)=='#')c.rect({pos.x,pos.y,16,16},{255,100,100,100});if(debug&&l.water[i])c.rect({pos.x,pos.y,16,16},{80,190,255,70});}
 std::vector<const Object*> objs;for(auto &o:s.objects)if(std::find(removed.begin(),removed.end(),scene+":"+o.id)==removed.end())objs.push_back(&o);std::stable_sort(objs.begin(),objs.end(),[&](auto a,auto b){return a->position.y+project.sprites.at(a->sprite).source.h<b->position.y+project.sprites.at(b->sprite).source.h;});bool drawn=false;auto player=[&](){auto r=pose();c.sprite(r,{feet.x-camera.x-r.w/2,feet.y-camera.y-r.h});};for(auto o:objs){auto sp=project.sprites.at(visibleSprite(*o));if(!drawn&&o->position.y+sp.source.h>feet.y){player();drawn=true;}c.sprite(sp.source,{o->position.x-camera.x,o->position.y-camera.y});auto collisionSprite=project.sprites.at(o->sprite);if(debug&&collisionSprite.collider.w>0&&!(o->kind=="door"&&activated.count(scene+":"+o->id)))c.rect({o->position.x+collisionSprite.collider.x-camera.x,o->position.y+collisionSprite.collider.y-camera.y,collisionSprite.collider.w,collisionSprite.collider.h},{255,100,100,100});}if(!drawn)player();if(debug){auto r=body(feet);c.rect({r.x-camera.x,r.y-camera.y,r.w,r.h},{170,255,150,130});for(auto &p:s.portals)c.rect({p.bounds.x-camera.x,p.bounds.y-camera.y,p.bounds.w,p.bounds.h},{170,255,150,60});}
 if(debug){c.rect({6,31,388,17},{18,28,34,230});c.text("X "+std::to_string(int(feet.x))+"  Y "+std::to_string(int(feet.y))+"  FRAME "+std::to_string(int(frameMs))+" MS",12,36,{185,213,175});}
 c.rect({6,6,280,21},{18,28,34,230});c.text(s.title,12,12,{219,226,193});c.rect({330,6,64,21},{18,28,34,230});c.text("ITEMS "+std::to_string(score),334,12,{220,205,154});auto display=toastTime>0?toast:message;if(!display.empty()&&dialogueObject.empty()&&!inventoryOpen&&!journalOpen){c.rect({6,194,388,18},{18,28,34,230});c.text(display.substr(0,63),12,200,{220,225,200});}c.rect({6,217,388,17},{18,28,34,230});c.text("WASD MOVE  SHIFT RUN  E USE  Q/I BAG  J QUESTS",10,222,{153,181,171});if(!dialogueObject.empty()&&dialogueNode.empty()){
 c.rect({6,145,388,69},{18,28,34,245});c.text(dialogueSpeaker.substr(0,50),12,151,{233,200,138});auto &pages=interactions.at(dialogueObject).pages;auto value=pages.at(dialoguePage);std::vector<std::string> lines;std::string line;std::istringstream words(value);std::string word;
 while(words>>word){while(word.size()>60){if(!line.empty()){lines.push_back(line);line.clear();}lines.push_back(word.substr(0,60));word.erase(0,60);}if(!line.empty()&&line.size()+word.size()+1>60){lines.push_back(line);line.clear();}if(!line.empty())line+=' ';line+=word;}if(!line.empty())lines.push_back(line);for(size_t i=0;i<std::min(size_t(4),lines.size());i++)c.text(lines[i],12,164+int(i)*9,{219,226,211});c.text("E NEXT  SPACE CLOSE  "+std::to_string(dialoguePage+1)+"/"+std::to_string(pages.size()),12,204,{153,181,171});
 }
 if(inventoryOpen){c.rect({42,40,316,174},{18,28,34,245});c.text("BAG",52,48,{233,200,138});int row=0,index=0;for(auto &entry:inventory){if(index++<inventoryPage*8)continue;if(row>=8)break;auto &item=items.at(entry.first);auto source=project.sprites.at(item.icon).source;float scale=std::min(12.f/source.w,12.f/source.h);c.image(source,{52,float(67+row*16),source.w*scale,source.h*scale});c.text(item.name.substr(0,36),70,69+row*16,{219,226,211});c.text(std::to_string(entry.second),322,69+row*16,{233,200,138});row++;}if(inventory.empty())c.text("YOUR BAG IS EMPTY",52,78,{153,181,171});c.text("A/D PAGE  Q/I CLOSE  "+std::to_string(inventoryPage+1)+"/"+std::to_string(std::max(1,int((inventory.size()+7)/8))),52,202,{153,181,171});}
 drawNarrative(c);
 return c;}

bool StudioGame::save(const std::string& path,std::string& error) const {
 std::string temp=path+".tmp";std::ofstream out(temp,std::ios::binary);if(!out){error="Cannot create save file";return false;}
 out<<"EVE_STUDIO_SAVE 3\n"<<std::quoted(project.id)<<" "<<std::quoted(scene)<<"\n"<<std::setprecision(9)<<feet.x<<" "<<feet.y<<" "<<score<<"\n"<<removed.size()<<"\n";for(auto &key:removed)out<<std::quoted(key)<<"\n";out<<activated.size()<<"\n";for(auto &key:activated)out<<std::quoted(key)<<"\n";out<<inventory.size()<<"\n";for(auto &entry:inventory)out<<std::quoted(entry.first)<<" "<<entry.second<<"\n";out<<questProgress.size()<<"\n";for(auto& p:questProgress){out<<std::quoted(p.first)<<" "<<int(p.second.completed)<<" "<<p.second.events.size();for(size_t i=0;i<p.second.events.size();i++){auto& objective=quests.at(p.first).objectives.at(i);out<<" "<<std::quoted(objective.type+":"+objective.target+":"+std::to_string(objective.quantity))<<" "<<int(p.second.events[i]);}out<<"\n";}out.flush();bool ok=bool(out);out.close();
 if(!ok||out.fail()){std::remove(temp.c_str());error="Save write failed";return false;}
#ifdef _WIN32
 ok=MoveFileExA(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 ok=std::rename(temp.c_str(),path.c_str())==0;
#endif
 if(!ok){std::remove(temp.c_str());error="Cannot commit save";return false;}error.clear();return true;
}
bool StudioGame::load(const std::string& path,std::string& error){try{
 std::ifstream input(path,std::ios::binary);input.seekg(0,std::ios::end);auto size=input.tellg();require(input&&size>=0&&size<=8*1024*1024,"Save is missing or too large");input.seekg(0);std::string magic;int version=0;input>>magic>>version;
 std::string nextScene;Vec2 nextFeet;int nextScore=0;std::vector<std::string> nextRemoved;std::set<std::string> nextActivated;std::map<std::string,int> nextInventory;std::map<std::string,StudioQuestProgress> nextQuests;
 if(magic=="LOTA_SAVE"&&version==1){SavedState old;auto oldProject=project;if(!loadState(path,oldProject,old,error))return false;nextScene=old.scene;nextFeet=old.player;nextScore=old.score;nextRemoved=old.removed;}
 else{require(magic=="EVE_STUDIO_SAVE"&&(version==2||version==3),"Incompatible progress save");std::string projectId;input>>std::quoted(projectId)>>std::quoted(nextScene)>>nextFeet.x>>nextFeet.y>>nextScore;require(input&&projectId==project.id&&project.scenes.count(nextScene)&&std::isfinite(nextFeet.x)&&std::isfinite(nextFeet.y)&&nextScore>=0,"Corrupt or incompatible progress save");
  auto count=[&](int max){int n=-1;input>>n;require(input&&n>=0&&n<=max,"Invalid save count");return n;};std::set<std::string> seen;
  for(int i=0,n=count(65536);i<n;i++){std::string key;input>>std::quoted(key);auto o=interactions.find(key);require(input&&key.size()<=129&&seen.insert(key).second&&o!=interactions.end()&&o->second.kind=="collectible","Invalid saved pickup");nextRemoved.push_back(key);}
  for(int i=0,n=count(65536);i<n;i++){std::string key;input>>std::quoted(key);auto o=interactions.find(key);require(input&&key.size()<=129&&nextActivated.insert(key).second&&o!=interactions.end()&&(o->second.kind=="door"||o->second.kind=="chest"||o->second.kind=="switch"),"Invalid saved interaction");}
  for(int i=0,n=count(64);i<n;i++){std::string key;int quantity=0;input>>std::quoted(key)>>quantity;require(input&&items.count(key)&&quantity>=1&&quantity<=999&&nextInventory.emplace(key,quantity).second,"Invalid saved inventory");}
  if(version==3)for(int i=0,n=count(64);i<n;i++){std::string name;int completed=-1;input>>std::quoted(name)>>completed;require(input&&quests.count(name)&&(completed==0||completed==1)&&!nextQuests.count(name),"Invalid saved quest");auto& q=quests.at(name);int steps=count(8);require(steps==int(q.objectives.size()),"Quest objectives changed");StudioQuestProgress progress;progress.completed=completed;for(int k=0;k<steps;k++){std::string signature;int event=-1;input>>std::quoted(signature)>>event;auto& objective=q.objectives[k];require(signature==objective.type+":"+objective.target+":"+std::to_string(objective.quantity),"Quest objective changed");require(input&&(event==0||event==1)&&(q.objectives[k].type!="item"||event==0)&&(!completed||q.objectives[k].type=="item"||event==1),"Invalid saved quest objective");progress.events.push_back(event==1);}nextQuests.emplace(name,std::move(progress));}
  input>>std::ws;require(input.eof(),"Trailing save data");
 }
 auto collision=collisionScene(nextScene,nextRemoved,nextActivated);require(!blocked(project,collision,body(nextFeet)),"Saved position is blocked");scene=nextScene;feet=nextFeet;score=nextScore;removed=std::move(nextRemoved);activated=std::move(nextActivated);inventory=std::move(nextInventory);questProgress=std::move(nextQuests);journalOpen=false;journalPage=0;dialogueNode.clear();dialogueChoice=0;clock=0;attackClock=-1;soundEvents.clear();dialogueObject.clear();inventoryOpen=false;inventoryPage=0;toastTime=0;error.clear();return true;
 }catch(const std::exception& e){error=e.what();return false;}}
}
