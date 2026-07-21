# Neusicplus

Neusicplus is the separate native C++/JUCE version of the Neusic DAW. It is intentionally isolated from NeusicLabV1 so native development cannot break the existing web apps.

## Current native MVP

- JUCE 8.0.13 application shell
- Cross-platform CMake build
- Native audio input/output initialization
- Low-latency microphone monitoring
- 24-bit stereo WAV recording
- Safe mono/stereo input handling
- Mobile-first transport and four-track arrangement surface
- Bottom tabs for Arrange, Record, Sounds, Mixer, and Export

## Requirements

- CMake 3.22+
- C++20 compiler
- Xcode for macOS/iOS
- Android Studio + NDK for Android
- Internet access during first configure so CMake can fetch JUCE

## Build on macOS

```bash
git clone https://github.com/bnsceo/Neusicplus.git
cd Neusicplus
cmake -S . -B build -G Xcode
cmake --build build --config Debug
open "build/NeusicStudio_artefacts/Debug/Neusic Studio.app"
```

## Build with Ninja

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Recording output

Recordings are written to:

```text
Documents/NeusicStudio/Recordings/
```

## Roadmap

1. Transport clock and clip playback
2. Audio import
3. Editable clip model
4. Independent track buses
5. Volume, pan, mute, and solo
6. Project save/load
7. Mixdown export
8. iOS and Android packaging
9. Effects rack
10. MIDI and piano roll

## Architecture rule

Audio and project state live in C++. Mobile screens are designed for touch from the beginning. The app must never copy the web DAW's desktop layout directly into a phone viewport.
