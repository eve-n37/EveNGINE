#include "lota/platform.hpp"
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <stdexcept>
#include <vector>

namespace lota {
namespace {
class SDLPlatform final : public Platform {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;
  SDL_AudioStream *audio = nullptr;
  SDL_AudioStream *musicStream = nullptr;
  const AudioClip *musicClip = nullptr;
  size_t musicOffset=0;
  std::vector<SDL_AudioStream*> effects;
  SDL_Gamepad *gamepad = nullptr;
  std::array<bool, size_t(Action::Count)> keyboard{}, controller{};
  bool running = true, closeConfirmation=false, closeRequested=false;
  bool customInput=false, padActive=false;std::string rawKey;std::map<Action,std::string> configuredKeys,configuredButtons;
  int mapping(SDL_Scancode code) {
    if(customInput){switch(code){case SDL_SCANCODE_LEFT:return int(Action::Left);case SDL_SCANCODE_RIGHT:return int(Action::Right);case SDL_SCANCODE_UP:return int(Action::Up);case SDL_SCANCODE_DOWN:return int(Action::Down);case SDL_SCANCODE_RETURN:return int(Action::Interact);case SDL_SCANCODE_ESCAPE:return int(Action::Pause);default:break;}for(auto& binding:configuredKeys)if(SDL_GetScancodeFromName(binding.second.c_str())==code)return int(binding.first);return -1;}
    switch (code) {
    case SDL_SCANCODE_A:
    case SDL_SCANCODE_LEFT:
      return int(Action::Left);
    case SDL_SCANCODE_D:
    case SDL_SCANCODE_RIGHT:
      return int(Action::Right);
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_UP:
      return int(Action::Up);
    case SDL_SCANCODE_S:
    case SDL_SCANCODE_DOWN:
      return int(Action::Down);
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
      return int(Action::Run);
    case SDL_SCANCODE_E:
    case SDL_SCANCODE_RETURN:
      return int(Action::Interact);
    case SDL_SCANCODE_SPACE:
      return int(Action::Attack);
    case SDL_SCANCODE_F:
      return int(Action::Dive);
    case SDL_SCANCODE_C:
      return int(Action::Hop);
    case SDL_SCANCODE_R:
      return int(Action::Reset);
    case SDL_SCANCODE_H:
      return int(Action::Potion);
    case SDL_SCANCODE_1:
      return int(Action::One);
    case SDL_SCANCODE_2:
      return int(Action::Two);
    case SDL_SCANCODE_3:
      return int(Action::Three);
    case SDL_SCANCODE_DELETE:
    case SDL_SCANCODE_BACKSPACE:
      return int(Action::Clear);
    case SDL_SCANCODE_J:
      return int(Action::Journal);
    case SDL_SCANCODE_I:
    case SDL_SCANCODE_Q:
      return int(Action::Menu);
    case SDL_SCANCODE_F5:
      return int(Action::Save);
    case SDL_SCANCODE_F9:
      return int(Action::Load);
    case SDL_SCANCODE_F3:
      return int(Action::Debug);
    case SDL_SCANCODE_ESCAPE:
      return int(Action::Quit);
    default:
      return -1;
    }
  }
  void cleanup() {
    if (gamepad)
      SDL_CloseGamepad(gamepad);
    if (audio)
      SDL_DestroyAudioStream(audio);
    if (musicStream) SDL_DestroyAudioStream(musicStream);
    for(auto stream:effects)SDL_DestroyAudioStream(stream);
    if (texture)
      SDL_DestroyTexture(texture);
    if (renderer)
      SDL_DestroyRenderer(renderer);
    if (window)
      SDL_DestroyWindow(window);
    SDL_Quit();
  }

public:
  SDLPlatform(const Atlas &atlas, const std::string &title) {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
      throw std::runtime_error(SDL_GetError());
    window = SDL_CreateWindow(title.c_str(), Width * 3, Height * 3,
                              SDL_WINDOW_RESIZABLE);
    if (window)
      renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer)
      texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STATIC, atlas.width,
                                  atlas.height);
    if (!texture) {
      std::string error = SDL_GetError();
      cleanup();
      throw std::runtime_error(error);
    }
    SDL_SetRenderLogicalPresentation(renderer, Width, Height,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, nullptr, atlas.pixels.data(),
                      atlas.width * int(sizeof(Color)));
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
      SDL_AudioSpec spec{};
      spec.format = SDL_AUDIO_F32;
      spec.channels = 1;
      spec.freq = 22050;
      audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        &spec, nullptr, nullptr);
      if (audio)
        SDL_ResumeAudioStreamDevice(audio);
    }
    int count = 0;
    auto pads = SDL_GetGamepads(&count);
    if (count > 0)
      gamepad = SDL_OpenGamepad(pads[0]);
    SDL_free(pads);
  }
  ~SDLPlatform() override { cleanup(); }
  void configureInput(const std::map<Action,std::string>& keys,const std::map<Action,std::string>& buttons) override{customInput=true;configuredKeys=keys;configuredButtons=buttons;}
  std::string takeKeyPress() override{auto key=rawKey;rawKey.clear();return key;}
  bool usingGamepad() const override{return padActive;}
  bool setFullscreen(bool enabled) override{return SDL_SetWindowFullscreen(window,enabled);}
  void confirmWindowClose(bool enabled) override { closeConfirmation=enabled; }
  bool takeCloseRequest() override { bool requested=closeRequested;closeRequested=false;return requested; }
  bool poll(Input &input) override {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        if(closeConfirmation)closeRequested=true;else running=false;
      }
      if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        keyboard.fill(false);
      if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){rawKey=SDL_GetScancodeName(event.key.scancode);padActive=false;}int i = mapping(event.key.scancode);
        // Query actual keys below, so releasing one binding doesn't cancel
        // another.
        if (i >= 0 && event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
          input.pressed[size_t(i)] = true;
      }
      if(event.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN)padActive=true;
      if (event.type == SDL_EVENT_GAMEPAD_ADDED && !gamepad)
        gamepad = SDL_OpenGamepad(event.gdevice.which);
      if (event.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad &&
          SDL_GetGamepadID(gamepad) == event.gdevice.which) {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
      }
    }
    keyboard.fill(false);
    controller.fill(false);
    int count = 0;
    auto keys = SDL_GetKeyboardState(&count);
    if (SDL_GetKeyboardFocus() == window)
      for (int i = 0; i < count; ++i) {
        int action = mapping(SDL_Scancode(i));
        if (action >= 0 && keys[i])
          keyboard[size_t(action)] = true;
      }
    if (gamepad) {
      auto button = [&](Action a, SDL_GamepadButton b) {
        controller[size_t(a)] = SDL_GetGamepadButton(gamepad, b);
      };
      button(Action::Left, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
      button(Action::Right, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
      button(Action::Up, SDL_GAMEPAD_BUTTON_DPAD_UP);
      button(Action::Down, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
      int x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX),
          y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
      controller[size_t(Action::Left)] |= x < -10000;
      controller[size_t(Action::Right)] |= x > 10000;
      controller[size_t(Action::Up)] |= y < -10000;
      controller[size_t(Action::Down)] |= y > 10000;
      if(std::abs(x)>10000||std::abs(y)>10000)padActive=true;
      if(customInput){for(auto& b:configuredButtons)if(!b.second.empty()){const std::map<std::string,SDL_GamepadButton> names={{"south",SDL_GAMEPAD_BUTTON_SOUTH},{"east",SDL_GAMEPAD_BUTTON_EAST},{"west",SDL_GAMEPAD_BUTTON_WEST},{"north",SDL_GAMEPAD_BUTTON_NORTH},{"leftshoulder",SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},{"rightshoulder",SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},{"back",SDL_GAMEPAD_BUTTON_BACK},{"start",SDL_GAMEPAD_BUTTON_START},{"leftstick",SDL_GAMEPAD_BUTTON_LEFT_STICK},{"rightstick",SDL_GAMEPAD_BUTTON_RIGHT_STICK}};auto value=names.find(b.second);if(value!=names.end())button(b.first,value->second);}}else{
      button(Action::Run, SDL_GAMEPAD_BUTTON_NORTH);
      button(Action::Interact, SDL_GAMEPAD_BUTTON_SOUTH);
      button(Action::Attack, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
      button(Action::Dive, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
      button(Action::Potion, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
      button(Action::Hop, SDL_GAMEPAD_BUTTON_LEFT_STICK);
      button(Action::Menu, SDL_GAMEPAD_BUTTON_WEST);
      button(Action::Quit, SDL_GAMEPAD_BUTTON_START);
      controller[size_t(Action::Quit)] |=
          SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST);
      }
    }
    for (size_t i = 0; i < keyboard.size(); ++i)
      input.set(Action(i), keyboard[i] || controller[i]);
    return running;
  }
  double seconds() override { return SDL_GetTicksNS() / 1000000000.0; }
  void present(const Canvas &canvas) override {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    for (const auto &d : canvas.draws) {
      if (d.clipped) {
        SDL_Rect clip{int(std::ceil(d.clip.x)), int(std::ceil(d.clip.y)),
                      std::max(0, int(std::ceil(d.clip.x + d.clip.w)) -
                                      int(std::ceil(d.clip.x))),
                      std::max(0, int(std::ceil(d.clip.y + d.clip.h)) -
                                      int(std::ceil(d.clip.y)))};
        SDL_SetRenderClipRect(renderer, &clip);
      } else
        SDL_SetRenderClipRect(renderer, nullptr);
      SDL_FRect dst{d.destination.x, d.destination.y, d.destination.w,
                    d.destination.h};
      if (d.sprite) {
        SDL_SetTextureColorMod(texture, d.color.r, d.color.g, d.color.b);
        SDL_SetTextureAlphaMod(texture, d.color.a);
        SDL_FRect src{d.source.x, d.source.y, d.source.w, d.source.h};
        SDL_RenderTextureRotated(renderer, texture, &src, &dst, 0, nullptr,
                                 d.flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
      } else {
        SDL_SetRenderDrawColor(renderer, d.color.r, d.color.g, d.color.b,
                               d.color.a);
        SDL_RenderFillRect(renderer, &dst);
      }
    }
    SDL_SetRenderClipRect(renderer, nullptr);
    SDL_RenderPresent(renderer);
  }
  SDL_AudioStream* openClip(const AudioClip& clip, float gain) {
    if(!SDL_WasInit(SDL_INIT_AUDIO)||clip.samples.empty())return nullptr;
    SDL_AudioSpec spec{};spec.format=SDL_AUDIO_S16;spec.channels=clip.channels;spec.freq=clip.rate;
    auto stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
    if(stream)SDL_SetAudioStreamGain(stream,std::clamp(gain,0.f,1.f));return stream;
  }
  bool audioAvailable() const override {return audio!=nullptr;}
  void setMusic(const AudioClip* clip,float gain) override {
    if(clip!=musicClip){if(musicStream)SDL_DestroyAudioStream(musicStream);musicStream=nullptr;musicClip=clip;musicOffset=0;if(clip)musicStream=openClip(*clip,gain);if(musicStream){pumpMusic();SDL_ResumeAudioStreamDevice(musicStream);}}
    if(musicStream)SDL_SetAudioStreamGain(musicStream,std::clamp(gain,0.f,1.f));
  }
  void pumpMusic(){
    if(!musicStream||!musicClip||musicClip->samples.empty())return;
    int target=musicClip->rate*musicClip->channels/2; // quarter second in bytes
    std::vector<int16_t> chunk(size_t(2048*musicClip->channels));
    for(int count=0;count<8;count++){int queued=SDL_GetAudioStreamQueued(musicStream);if(queued<0||queued>=target)break;
      for(auto& sample:chunk){sample=musicClip->samples[musicOffset];musicOffset=(musicOffset+1)%musicClip->samples.size();}
      if(!SDL_PutAudioStreamData(musicStream,chunk.data(),int(chunk.size()*2)))break;
    }
  }
  void playSound(const AudioClip& clip,float gain) override {
    for(auto it=effects.begin();it!=effects.end();){if(SDL_GetAudioStreamAvailable(*it)==0&&SDL_GetAudioStreamQueued(*it)==0){SDL_DestroyAudioStream(*it);it=effects.erase(it);}else ++it;}
    if(effects.size()>=8){SDL_DestroyAudioStream(effects.front());effects.erase(effects.begin());}
    auto stream=openClip(clip,gain);if(!stream)return;
    if(!SDL_PutAudioStreamData(stream,clip.samples.data(),int(clip.samples.size()*2))||!SDL_FlushAudioStream(stream)||!SDL_ResumeAudioStreamDevice(stream)){SDL_DestroyAudioStream(stream);return;}effects.push_back(stream);
  }
  void tone(float frequency, float duration) override {
    if (!audio)
      return;
    SDL_ClearAudioStream(audio);
    int n = int(duration * 22050);
    std::vector<float> wave(n);
    for (int i = 0; i < n; ++i)
      wave[i] = 0.06f * std::sin(float(i) * frequency * 6.2831853f / 22050.0f) *
                (1 - float(i) / n);
    SDL_PutAudioStreamData(audio, wave.data(),
                           int(wave.size() * sizeof(float)));
  }
  std::string savePath(const std::string &project) override {
    char *path = SDL_GetPrefPath("LotAEngine", project.c_str());
    if (!path)
      throw std::runtime_error("Cannot find save directory");
    std::string result = std::string(path) + "save.dat";
    SDL_free(path);
    return result;
  }
  void idle() override { pumpMusic(); SDL_Delay(1); }
};
} // namespace
std::unique_ptr<Platform> createPlatform(const Atlas &atlas,
                                         const std::string &title) {
  return std::make_unique<SDLPlatform>(atlas, title);
}
std::string defaultProjectPath() {
  const char *base = SDL_GetBasePath();
  return std::string(base ? base : "") + "samples/valley/game.project";
}
} // namespace lota
