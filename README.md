# Granular Drums

Granular Drums is a JUCE-based granular drum sampler plugin. It analyzes dropped audio, maps slices across a 16-pad grid, and includes granular playback controls, sequencing, modulation, and master character processing.

## Formats

- VST3
- Audio Unit
- Standalone app

## Requirements

- CMake 3.22 or newer
- A C++20 compiler
- macOS build tools/Xcode command line tools for AU, VST3, and standalone builds
- Network access on first configure so CMake can fetch JUCE 8.0.4

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

Built plugin formats are written under `build/GranularDrums_artefacts/`.

## Source Layout

- `Source/PluginProcessor.*` contains plugin state, parameters, sequencing, analysis, and audio processing.
- `Source/PluginEditor.*` contains the main UI.
- `Source/DSP/` contains analysis, slicing, playback, granular, and character processing.
- `Source/UI/` contains reusable UI components.

## Notes

This repository does not currently include a license. Add one before publishing if you want others to have explicit rights to use, modify, or distribute the code.
