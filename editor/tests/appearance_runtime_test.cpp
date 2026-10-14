#include "evengine/studio.hpp"
#include <filesystem>
#include <iostream>
using namespace evengine;
int main(int argc,char** argv){try{if(argc!=3)throw std::runtime_error("Expected themed project and output directory");StudioGame g(argv[1]);int checks=0;auto check=[&](bool ok,const char* name){if(!ok)throw std::runtime_error(name);checks++;std::cout<<"OK "<<name<<'\n';};
 auto equal=[](Color a,Color b){return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;};check(equal(g.uiPanel,{19,27,58,255})&&equal(g.uiModal,g.uiPanel),"panel color and opacity loaded");check(equal(g.uiText,{230,238,255,255})&&equal(g.uiAccent,{128,214,255,255})&&equal(g.uiMuted,{169,183,215,255})&&equal(g.uiSelection,{41,70,104,255}),"all UI roles loaded");
 auto rect=[&](Canvas c,int x,int y,Color color){for(auto& d:c.draws)if(!d.sprite&&d.destination.x==x&&d.destination.y==y&&equal(d.color,color))return true;return false;};
 check(rect(g.draw(),6,6,g.uiPanel),"HUD uses project panel color");SoftwareRenderer r;r.render(g.draw(),g.atlas);check(equal(r.pixels[7*Width+7],g.uiPanel),"rendered HUD pixel matches chosen color");
 const Object* keeper=nullptr;for(auto& o:g.project.scenes.at(g.scene).objects)if(o.id=="keeper")keeper=&o;check(keeper!=nullptr,"dialogue fixture found");g.interact(*keeper);check(rect(g.draw(),6,96,g.uiModal)&&rect(g.draw(),10,147,g.uiSelection),"branching dialogue and selection use chosen colors");r.render(g.draw(),g.atlas);r.writePPM((std::filesystem::path(argv[2])/"appearance-dialogue.ppm").string());
 g.dialogueObject.clear();g.acceptQuest("keepers-errand");g.journalOpen=true;check(rect(g.draw(),12,36,g.uiModal),"journal uses chosen panel color");r.render(g.draw(),g.atlas);r.writePPM((std::filesystem::path(argv[2])/"appearance-journal.ppm").string());g.journalOpen=false;g.inventoryOpen=true;check(rect(g.draw(),42,40,g.uiModal),"bag uses chosen panel color");
 g.inventoryOpen=false;g.dialogueObject.clear();StudioGame legacy((std::filesystem::path(argv[1]).parent_path()/"Keys-and-Gates.eve").string());legacy.loadAppearance(Json::load(argv[1]));for(auto& o:legacy.project.scenes.at(legacy.scene).objects)if(o.id=="keeper")legacy.interact(o);check(rect(legacy.draw(),6,145,g.uiModal),"simple dialogue pages use chosen panel color");
 std::string error,path=(std::filesystem::path(argv[2])/"appearance-save.dat").string();check(g.save(path,error)&&g.load(path,error)&&equal(g.uiAccent,{128,214,255,255}),"progress reload retains project theme");
 std::cout<<checks<<" appearance runtime checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
