# Native audio API

Include `<evengine/audio.hpp>` and `<evengine/platform.hpp>`. Link `EveNGINE::Core` and `EveNGINE::Platform`; `EveNGINE::Studio` is optional.

`AudioClip` holds the sample rate, channel count and interleaved signed 16-bit PCM samples. `decodeWavBytes` parses a byte vector; `decodeWav` accepts its base64 form. Both apply Studio's PCM WAV limits. Playback uses SDL3 audio streams with native resampling.

```cpp
const auto clip = evengine::decodeWavBytes(wavBytes);
platform->setMusic(&clip, 0.5f);  // loop, then keep this clip alive
platform->playSound(clip, 0.7f); // queue one effect
// Call platform->idle() in the normal render loop to refill music.
platform->setMusic(nullptr, 0); // stop before releasing music storage
```

`setMusic` retains a pointer: the clip and its samples must remain alive and unchanged until music is stopped or replaced. Passing the same pointer adjusts the volume without restarting it. `playSound` queues its samples during the call, so the caller can release that effect's storage afterwards. Gains are clamped to 0–1. Up to eight effects play concurrently; the oldest is replaced on overflow. Music is queued in short chunks rather than copying a complete track on each loop.

`audioAvailable()` indicates whether the desktop playback stream opened. Headless/custom platforms keep silent default methods. The original generated `tone()` method remains available. The bundled SDL3 3.2 backend implements playback; no mixer addon or audio codec package is required.

`StudioGame::music()` returns the current map's retained clip. `soundEvents` contains audio asset IDs queued by simulation; play and clear them after fixed-step updates. The bundled runtime is an example of this integration. `musicVolume` and `sfxVolume` hold authored gains.

Official SDL stream reference: https://wiki.libsdl.org/SDL3/SDL_OpenAudioDeviceStream
