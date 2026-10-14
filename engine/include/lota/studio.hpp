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
struct StudioObjective { std::string type,target; int quantity=1; };
struct StudioQuest { std::string name,description,rewardItem; int rewardQuantity=1; std::vector<StudioObjective> objectives; };
struct StudioQuestProgress { bool completed=false; std::vector<bool> events; };
struct StudioChoice {
 std::string text,next,action,quest,conditionQuest,conditionState,requiredItem;
 int quantity=1;
};
struct StudioDialogueNode { std::string text; std::vector<StudioChoice> choices; };
struct StudioInteraction {
 std::string kind,openSprite,item,requiredItem,speaker,portrait;
 Color speakerColor;bool customSpeakerColor=false;
 int quantity=1; bool consumeKey=false,switchOnly=false;
 std::vector<std::string> pages,targets;
 std::string dialogueStart;
 std::map<std::string,StudioDialogueNode> nodes;
};
struct StudioCondition { std::string kind,target,state;int quantity=1;bool value=true; };
struct StudioEventAction { std::string type,target,text;int quantity=1;bool value=true;Vec2 arrival; };
struct StudioEvent { std::string id,name,trigger,scene,object;Rect area;bool once=true;std::vector<StudioCondition> conditions;std::vector<StudioEventAction> actions; };
class StudioGame {
public:
 Project project; Atlas atlas;
 Color uiPanel{18,28,34,230},uiModal{18,28,34,245},uiText{219,226,211},uiAccent{233,200,138},uiMuted{153,181,171},uiSelection{48,65,49,255};
 void loadAppearance(const Json& project);
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
 std::map<std::string,StudioQuest> quests;
 std::map<std::string,StudioQuestProgress> questProgress;
 bool journalOpen=false; int journalPage=0,dialogueChoice=0;
 std::string dialogueNode;
 void loadNarrative(const Json& project);
 std::string questState(const std::string& id) const;
 bool objectiveDone(const std::string& quest,size_t index) const;
 void questEvent(const std::string& type,const std::string& target);
 bool acceptQuest(const std::string& id);
 bool completeQuest(const std::string& id);
 std::vector<size_t> availableChoices() const;
 bool chooseDialogue(size_t visibleIndex);
 void drawNarrative(Canvas& canvas) const;
 bool inventoryOpen=false; int inventoryPage=0,dialoguePage=0;
 std::string dialogueObject,dialogueSpeaker,toast;float toastTime=0;
 Scene collisionScene(const std::string& scene,const std::vector<std::string>& removed,const std::set<std::string>& activated) const;
 std::string visibleSprite(const Object& object) const;
 bool giveItem(const std::string& item,int quantity);
 void interact(const Object& object);
 std::string projectPath,progressPath,preferencesPath,frontend="playing",settingsReturn="title",confirmAction,pendingBinding;
 std::string titleArtwork,titleLabel,subtitle,menuNew="New game",menuContinue="Continue",menuSettings="Settings",menuQuit="Quit";
 int menuIndex=0;bool canContinue=false,progressDirty=false,fullscreen=false,frontendStarted=false,controllerMode=false,eventBlocked=false;
 float userMusic=1,userEffects=1,typewriterSpeed=0,dialogueTime=0;
 std::map<Action,std::string> keyBindings,buttonBindings;
 std::map<std::string,bool> flagDefaults,flags;
 std::vector<StudioEvent> events;std::set<std::string> firedEvents,insideTriggers;
 std::string eventScene;
 void loadReleaseFeatures(const Json& project);
 void beginSession(const std::string& savePath,bool playtest=false);
 void startNewGame();
 bool updateFrontend(const Input& input);
 void drawFrontend(Canvas& canvas) const;
 std::vector<std::string> menuOptions() const;
 void saveProgress();void loadProgress();
 void savePreferences();void loadPreferences();
 bool captureBinding(const std::string& key);
 std::string controlLabel(Action action) const;
 bool runEvents(const std::string& trigger,const std::string& target);
 bool fireEvent(const StudioEvent& event);
 void updateTriggers();
 std::string dialogueText() const;
 bool dialogueRevealed() const;
 std::string revealText(const std::string& text) const;
 Color speakerTint() const;
 void drawPortrait(Canvas& canvas,int x,int y) const;
 explicit StudioGame(const std::string &path);
 void update(const Input &input,float dt);
 Canvas draw() const;
 bool save(const std::string &path,std::string &error) const;
 bool load(const std::string &path,std::string &error);
 Rect pose() const;
};
}
