#include "lota/studio.hpp"
#include <sstream>
#include <stdexcept>

namespace lota {
namespace {
void need(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
std::string identifier(const Json& v){auto s=v.text();need(v.type==Json::String&&!s.empty()&&s.size()<=64&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos,"Invalid story identifier");return s;}
std::string label(const Json& v,size_t limit,bool empty=false){need(v.type==Json::String&&v.string.size()<=limit&&(empty||!v.string.empty()),"Invalid story text");return v.string;}
int quantity(const Json& v){need(v.type==Json::Number&&std::floor(v.number)==v.number&&v.number>=1&&v.number<=99,"Story quantities must be 1-99");return int(v.number);}
void list(const Json& v,size_t lo,size_t hi){need(v.type==Json::Array&&v.array.size()>=lo&&v.array.size()<=hi,"Invalid story list");}
std::vector<std::string> wrap(const std::string& value,size_t width){std::istringstream words(value);std::string word,line;std::vector<std::string> lines;while(words>>word){while(word.size()>width){if(!line.empty()){lines.push_back(line);line.clear();}lines.push_back(word.substr(0,width));word.erase(0,width);}if(!line.empty()&&line.size()+word.size()+1>width){lines.push_back(line);line.clear();}if(!line.empty())line+=' ';line+=word;}if(!line.empty())lines.push_back(line);return lines;}
}

void StudioGame::loadNarrative(const Json& j){
 if(j.has("quests")){list(j["quests"],0,64);for(auto& v:j["quests"].array){auto name=identifier(v["id"]);StudioQuest q;q.name=label(v["name"],160);q.description=label(v["description"],720,true);list(v["objectives"],1,8);std::set<std::string> seen;
  for(auto& a:v["objectives"].array){StudioObjective o;o.type=a["type"].text();o.target=label(a["target"],129);if(a.has("quantity"))o.quantity=quantity(a["quantity"]);need(seen.insert(o.type+":"+o.target).second,"Duplicate quest objective");
   if(o.type=="item")need(items.count(o.target),"Unknown quest item");
   else if(o.type=="visit")need(project.scenes.count(o.target)&&o.quantity==1,"Unknown quest map");
   else{auto i=interactions.find(o.target);need(i!=interactions.end()&&o.quantity==1,"Unknown quest object");need(o.type=="talk"?i->second.kind=="npc":o.type=="activate"&&(i->second.kind=="door"||i->second.kind=="chest"||i->second.kind=="switch"),"Invalid quest objective type");}
   q.objectives.push_back(o);
  }
  if(v.has("rewardItem")){q.rewardItem=label(v["rewardItem"],64,true);need(q.rewardItem.empty()||items.count(q.rewardItem),"Unknown quest reward");}if(v.has("rewardQuantity"))q.rewardQuantity=quantity(v["rewardQuantity"]);need(quests.emplace(name,q).second,"Duplicate quest");
 }}
 for(auto& sceneJson:j["scenes"].array)for(auto& o:sceneJson["objects"].array)if(o.has("dialogue")){
  auto& data=interactions.at(sceneJson["id"].text()+":"+o["id"].text());need(data.kind=="npc","Only NPCs can have dialogue trees");auto& d=o["dialogue"];data.dialogueStart=identifier(d["start"]);list(d["nodes"],1,64);
  for(auto& v:d["nodes"].array){auto name=identifier(v["id"]);StudioDialogueNode node;node.text=label(v["text"],720);list(v["choices"],0,3);
   for(auto& a:v["choices"].array){StudioChoice c;c.text=label(a["text"],160);c.next=label(a["next"],64,true);if(!c.next.empty())identifier(a["next"]);
    if(a.has("action"))c.action=label(a["action"],16,true);if(a.has("quest"))c.quest=label(a["quest"],64,true);need(c.action.empty()?c.quest.empty():(c.action=="accept"||c.action=="complete")&&quests.count(c.quest),"Invalid dialogue quest action");
    if(a.has("conditionQuest"))c.conditionQuest=label(a["conditionQuest"],64,true);if(a.has("conditionState"))c.conditionState=label(a["conditionState"],16,true);need(c.conditionQuest.empty()?c.conditionState.empty():quests.count(c.conditionQuest)&&(c.conditionState=="available"||c.conditionState=="active"||c.conditionState=="ready"||c.conditionState=="completed"),"Invalid dialogue quest condition");
    if(a.has("requiredItem"))c.requiredItem=label(a["requiredItem"],64,true);need(c.requiredItem.empty()||items.count(c.requiredItem),"Unknown choice item");if(a.has("quantity"))c.quantity=quantity(a["quantity"]);node.choices.push_back(c);
   }
   need(data.nodes.emplace(name,node).second,"Duplicate dialogue node");
  }
  need(data.nodes.count(data.dialogueStart),"Missing starting dialogue node");for(auto& n:data.nodes)for(auto& c:n.second.choices)need(c.next.empty()||data.nodes.count(c.next),"Missing dialogue destination");
  std::set<std::string> reachable;std::vector<std::string> pending{data.dialogueStart};while(!pending.empty()){auto name=pending.back();pending.pop_back();if(!reachable.insert(name).second)continue;for(auto& c:data.nodes.at(name).choices)if(!c.next.empty())pending.push_back(c.next);}need(reachable.size()==data.nodes.size(),"Unreachable dialogue node");
 }
}
bool StudioGame::objectiveDone(const std::string& name,size_t index) const{
 auto p=questProgress.find(name);if(p==questProgress.end())return false;if(p->second.completed)return true;auto& o=quests.at(name).objectives.at(index);if(o.type=="item"){auto i=inventory.find(o.target);return i!=inventory.end()&&i->second>=o.quantity;}return p->second.events.at(index);
}
std::string StudioGame::questState(const std::string& name) const{auto p=questProgress.find(name);if(p==questProgress.end())return "available";if(p->second.completed)return "completed";for(size_t i=0;i<quests.at(name).objectives.size();i++)if(!objectiveDone(name,i))return "active";return "ready";}
void StudioGame::questEvent(const std::string& type,const std::string& target){for(auto& p:questProgress)if(!p.second.completed){auto& q=quests.at(p.first);for(size_t i=0;i<q.objectives.size();i++)if(q.objectives[i].type==type&&q.objectives[i].target==target){if(!p.second.events[i])progressDirty=true;p.second.events[i]=true;}}}
bool StudioGame::acceptQuest(const std::string& name){if(!quests.count(name)||questProgress.count(name))return false;progressDirty=true;questProgress[name]={false,std::vector<bool>(quests.at(name).objectives.size(),false)};questEvent("visit",scene);for(auto& key:activated)questEvent("activate",key);if(!dialogueObject.empty())questEvent("talk",dialogueObject);toast="Quest accepted: "+quests.at(name).name;toastTime=3;return true;}
bool StudioGame::completeQuest(const std::string& name){if(!quests.count(name)||questState(name)!="ready"){toast="Finish the quest objectives first.";toastTime=3;return false;}
 auto& q=quests.at(name);auto next=inventory;for(auto& o:q.objectives)if(o.type=="item"){next[o.target]-=o.quantity;if(next[o.target]==0)next.erase(o.target);}if(!q.rewardItem.empty()){if(next[q.rewardItem]+q.rewardQuantity>999){toast="Make room in your bag for the reward.";toastTime=3;return false;}next[q.rewardItem]+=q.rewardQuantity;}
 inventory=std::move(next);questProgress.at(name).completed=true;progressDirty=true;toast="Quest completed: "+q.name;toastTime=3;cue("chest");return true;
}
std::vector<size_t> StudioGame::availableChoices() const{std::vector<size_t> out;if(dialogueObject.empty()||dialogueNode.empty())return out;auto& choices=interactions.at(dialogueObject).nodes.at(dialogueNode).choices;for(size_t i=0;i<choices.size();i++){auto& c=choices[i];if(!c.conditionQuest.empty()){auto state=questState(c.conditionQuest);if(state!=c.conditionState&&!(c.conditionState=="active"&&state=="ready"))continue;}if(!c.requiredItem.empty()){auto item=inventory.find(c.requiredItem);if(item==inventory.end()||item->second<c.quantity)continue;}out.push_back(i);}return out;}
bool StudioGame::chooseDialogue(size_t index){auto choices=availableChoices();if(index>=choices.size())return false;auto c=interactions.at(dialogueObject).nodes.at(dialogueNode).choices[choices[index]];if(c.action=="accept"&&!acceptQuest(c.quest)){toast="This quest is already accepted.";toastTime=3;return false;}if(c.action=="complete"&&!completeQuest(c.quest))return false;dialogueNode=c.next;dialogueChoice=0;dialogueTime=0;progressDirty=true;if(dialogueNode.empty())dialogueObject.clear();return true;}
void StudioGame::drawNarrative(Canvas& c) const{
 if(!dialogueObject.empty()&&!dialogueNode.empty()){
  if(toastTime>0){c.rect({6,68,388,20},uiModal);c.text(toast.substr(0,60),12,74,uiAccent);}
  c.rect({6,96,388,118},uiModal);drawPortrait(c,346,110);c.text(dialogueSpeaker.substr(0,40),12,102,speakerTint());auto& node=interactions.at(dialogueObject).nodes.at(dialogueNode);auto lines=wrap(revealText(node.text),53);for(size_t i=0;i<std::min(size_t(4),lines.size());i++)c.text(lines[i],12,116+int(i)*9,uiText);auto choices=dialogueRevealed()?availableChoices():std::vector<size_t>{};for(size_t i=0;i<choices.size();i++){auto& ch=node.choices[choices[i]];if(int(i)==dialogueChoice)c.rect({10,float(147+int(i)*14),376,13},uiSelection);c.text(std::to_string(i+1)+". "+ch.text.substr(0,40),12,150+int(i)*14,int(i)==dialogueChoice?uiAccent:uiText);}c.text(controlLabel(Action::Interact)+" SELECT  "+controlLabel(Action::Attack)+" CLOSE",12,201,uiMuted);
 }
 if(journalOpen){
  c.rect({12,36,376,178},uiModal);c.text("QUEST JOURNAL",22,44,uiAccent);if(questProgress.empty()){c.text("NO ACCEPTED QUESTS YET",22,72,uiText);c.text(controlLabel(Action::Journal)+" CLOSE",22,201,uiMuted);return;}
  auto p=questProgress.begin();std::advance(p,std::clamp(journalPage,0,int(questProgress.size())-1));auto& q=quests.at(p->first);c.text(q.name.substr(0,50),22,60,uiText);c.text(questState(p->first)=="ready"?"READY TO TURN IN":p->second.completed?"COMPLETED":"IN PROGRESS",22,73,uiAccent);auto lines=wrap(q.description,55);for(size_t i=0;i<std::min(size_t(2),lines.size());i++)c.text(lines[i],22,86+int(i)*9,uiMuted);for(size_t i=0;i<q.objectives.size();i++){auto& o=q.objectives[i];std::string name=o.target;if(o.type=="item")name=items.at(o.target).name+" x"+std::to_string(o.quantity);else if(o.type=="visit")name=project.scenes.at(o.target).title;else if(o.type=="talk")name=interactions.at(o.target).speaker;else name=o.target.substr(o.target.find(':')+1);auto verb=o.type=="item"?"DELIVER ":o.type=="talk"?"TALK ":o.type=="visit"?"VISIT ":"USE ";c.text(std::string(objectiveDone(p->first,i)?"DONE: ":"TODO: ")+verb+name.substr(0,38),22,111+int(i)*10,uiText);}c.text(controlLabel(Action::Left)+"/"+controlLabel(Action::Right)+" PAGE  "+controlLabel(Action::Journal)+" CLOSE  "+std::to_string(journalPage+1)+"/"+std::to_string(questProgress.size()),22,201,uiMuted);
 }
}
}
