# Audio API

This page covers playing audio from C++. For adding audio in Studio, see the
[Studio guide](STUDIO.md#audio).

Include `<evengine/audio.hpp>` and `<evengine/platform.hpp>`, and link `EveNGINE::Core` and
`EveNGINE::Platform`.

## Loading clips

An `AudioClip` holds a sample rate, a channel count and interleaved signed 16-bit PCM samples.

- `decodeWavBytes(bytes)` decodes WAV data from a `std::vector<uint8_t>`.
- `decodeWav(base64)` decodes base64-encoded WAV data, as stored in `.eve` projects.

Both apply the same limits as Studio: 16-bit PCM, mono or stereo, 8–48 kHz, up to 60 seconds and
4 MiB.

## Playing audio

```cpp
const auto clip = evengine::decodeWavBytes(wavBytes);

platform->setMusic(&clip, 0.5f);  // start looping music
platform->playSound(clip, 0.7f);  // play a one-off sound effect

// In your main loop, call platform->idle() every frame to keep music playing.

platform->setMusic(nullptr, 0);   // stop music
```

Things to know:

- **`setMusic` keeps a pointer** to the clip. Keep the clip alive and unchanged until you stop or
  replace the music. Calling it again with the same clip just changes the volume.
- **`playSound` copies the samples** right away, so you can free the clip afterwards.
- Volumes are clamped to 0–1.
- Up to eight sound effects play at once. A ninth replaces the oldest.
- `audioAvailable()` tells you whether an audio device was opened. The headless backend is always
  silent.
- The older `tone(frequency, duration)` method still works for simple generated beeps.

The desktop backend uses [SDL3 audio streams](https://wiki.libsdl.org/SDL3/SDL_OpenAudioDeviceStream),
which handle resampling. No extra mixer or codec libraries are needed.

## Audio in Studio projects

`StudioGame` loads a project's audio for you:

- `music()` returns the current map's music clip (or `nullptr`).
- `soundEvents` lists the audio asset IDs queued during the last update. Play each one, then clear
  the list.
- `audioAssets` maps asset IDs to clips.
- `musicVolume` and `sfxVolume` hold the project's volume levels.

`editor/runtime/main.cpp` shows the complete setup.
