#pragma once
#include "lota/core.hpp"
#include "lota/json.hpp"
#include <memory>
#include <set>
#include "lota/audio.hpp"
namespace lota {
struct StudioFrame { Rect source; int ms; };
struct StudioClip { std::string state, direction; std::vector<StudioFrame> frames; };
struct StudioLayer { std::vector<int> base,detail,water; };
struct StudioItem { std::string name,icon; };
struct StudioInteraction {
 std::string kind,openSprite,item,requiredItem,speaker;
 int quantity=1; bool consumeKey=false,switchOnly=false;
 std::vector<std::string> pages,targets;
};
class StudioGame {
public:
 Project project; Atlas atlas;
 std::map<std::string,StudioLayer> layers;
 std::vector<Rect> tileSources;
 std::vector<StudioClip> clips;
 std::map<std::string,AudioClip> audioAssets;
 std::map<std::string,std::string> musicByScene, audioCues;
 std::vector<std::string> soundEvents;
 float musicVolume=.5f,sfxVolume=.7f;
 const AudioClip* music() const;
 void cue(const std::string& name);
 std::string scene, direction="south", message;
 Vec2 feet; int score=0; bool quit=false, debug=false;
 float frameMs=0;
 float speed=64, runMultiplier=1.7f, clock=0, attackClock=-1, portalCooldown=0;
 std::vector<std::string> removed;
 std::map<std::string,StudioItem> items;
 std::map<std::string,int> inventory;
 std::map<std::string,StudioInteraction> interactions;
 std::set<std::string> activated;
 bool inventoryOpen=false; int inventoryPage=0,dialoguePage=0;
 std::string dialogueObject,dialogueSpeaker,toast;float toastTime=0;
 Scene collisionScene(const std::string& scene,const std::vector<std::string>& removed,const std::set<std::string>& activated) const;
 std::string visibleSprite(const Object& object) const;
 bool giveItem(const std::string& item,int quantity);
 void interact(const Object& object);
 explicit StudioGame(const std::string &path);
 void update(const Input &input,float dt);
 Canvas draw() const;
 bool save(const std::string &path,std::string &error) const;
 bool load(const std::string &path,std::string &error);
 Rect pose() const;
};
}
