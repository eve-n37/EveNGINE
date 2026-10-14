#pragma once
#include "core.hpp"
#include "audio.hpp"
#include <memory>
namespace lota {
// No SDK types cross this boundary. Projects and simulation are platform neutral.
class Platform {
public:
    virtual ~Platform() = default;
    virtual bool poll(Input& input) = 0;
    virtual double seconds() = 0;
    virtual void present(const Canvas& canvas) = 0;
    virtual void tone(float frequency, float duration) = 0;
    // Existing platform implementations may keep the silent defaults.
    // Music clip storage must outlive playback; pass nullptr before releasing it.
    // Sound samples are queued during playSound and may be released afterwards.
    virtual void setMusic(const AudioClip* clip, float volume) {}
    virtual void playSound(const AudioClip& clip, float volume) {}
    virtual void configureInput(const std::map<Action,std::string>& keys,const std::map<Action,std::string>& buttons) {}
    virtual std::string takeKeyPress() { return {}; }
    virtual bool usingGamepad() const { return false; }
    virtual bool setFullscreen(bool enabled) { return false; }
    virtual bool audioAvailable() const { return false; }
    virtual std::string savePath(const std::string& project) = 0;
    virtual void idle() = 0;
};
std::unique_ptr<Platform> createPlatform(const Atlas& atlas, const std::string& title);
std::string defaultProjectPath();
}
