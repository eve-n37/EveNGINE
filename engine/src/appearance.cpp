#include "lota/studio.hpp"
#include <stdexcept>
namespace lota {
namespace {
int nibble(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("Invalid appearance color or pixel");}
Color color(const Json& v){if(v.type!=Json::String||v.string.size()!=7||v.string[0]!='#')throw std::runtime_error("UI colors must use #RRGGBB");auto& s=v.string;return {uint8_t(nibble(s[1])*16+nibble(s[2])),uint8_t(nibble(s[3])*16+nibble(s[4])),uint8_t(nibble(s[5])*16+nibble(s[6])),255};}
}
void StudioGame::loadAppearance(const Json& j){
 if(j.has("uiTheme")){auto& theme=j["uiTheme"];if(theme.type!=Json::Object)throw std::runtime_error("Invalid UI theme");
  for(auto& entry:theme.object){auto& key=entry.first;if(key=="panel"){auto c=color(entry.second);uiPanel.r=uiModal.r=c.r;uiPanel.g=uiModal.g=c.g;uiPanel.b=uiModal.b=c.b;}
   else if(key=="text")uiText=color(entry.second);else if(key=="accent")uiAccent=color(entry.second);else if(key=="muted")uiMuted=color(entry.second);else if(key=="selection")uiSelection=color(entry.second);
   else if(key=="panelOpacity"){auto& v=entry.second;if(v.type!=Json::Number||!std::isfinite(v.number)||v.number<0||v.number>1)throw std::runtime_error("UI panel opacity must be 0-1");uiPanel.a=uiModal.a=uint8_t(std::round(v.number*255));}
   else throw std::runtime_error("Unknown UI theme field");
  }
 }
 if(j.has("appIcon")){auto& icon=j["appIcon"];auto& size=icon["size"];auto& pixels=icon["pixels"];
  if(icon.type!=Json::Object||size.type!=Json::Number||std::floor(size.number)!=size.number||size.number<16||size.number>512||pixels.type!=Json::String||pixels.string.size()!=size_t(size.number*size.number*8))throw std::runtime_error("Invalid game icon: use square RGBA artwork, 16-512 pixels");
  for(char c:pixels.string)nibble(c);
 }
}
}
