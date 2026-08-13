# WhisperFlow

Local speech-to-text desktop app. Record from the microphone or drop in an
audio file, get a transcript — everything runs on-device via
[whisper.cpp](https://github.com/ggml-org/whisper.cpp). No audio ever leaves
your machine.

Qt 6 Widgets, C++17. Builds and runs on macOS and Windows from the same tree.

## Features

- **One-button recording** with live mic level, animated record control
- **Global hotkey** — `⌘⇧R` (macOS) / `Ctrl+Shift+R` (Windows), works while
  the app is in the background
- **Model manager** — pick Tiny → Large (v3 turbo) in Settings; models
  download once from Hugging Face and are cached locally
- **Transcript history** — searchable cards, expand/collapse, copy, delete;
  persists across restarts
- **Play & re-transcribe** — recorded audio is kept per transcript, so you
  can replay it or run it again after switching to a better model
- **Drop any audio file** (wav/mp3/m4a/…) onto the window to transcribe it

## Models

| Model | Download | Notes |
|---|---|---|
| Tiny | ~78 MB | fastest, least accurate |
| Base | ~148 MB | default |
| Small | ~488 MB | good speed/accuracy balance |
| Medium | ~1.5 GB | slower, more accurate |
| Large v3 turbo | ~1.6 GB | best accuracy |

Downloaded on demand in **Settings** from the
[whisper.cpp Hugging Face repo](https://huggingface.co/ggerganov/whisper.cpp),
stored in the app's local data directory.

## Building

Requirements:

- CMake ≥ 3.19, git (FetchContent clones whisper.cpp on first configure)
- Qt 6.5+ with modules: Widgets, Svg, Network, Multimedia, Concurrent
- macOS: Xcode command line tools · Windows: MSVC 2022 or MinGW

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Or open `CMakeLists.txt` in Qt Creator and hit Run.

On Windows, deploy Qt DLLs next to the exe before distributing:

```
windeployqt --release build\WhisperFlow.exe
```

> whisper.cpp is built CPU-only on purpose so one tree builds identically on
> both platforms. For a faster Mac-only build, flip `GGML_METAL` to `ON` in
> `CMakeLists.txt`.

## Layout

```
src/
  ui/        window, cards, record button, settings dialog, theme, icons
  core/      whisper engine, model catalog/manager, audio capture & storage
  platform/  global hotkey backends (Carbon / Win32)
resources/   Info.plist template (mic permission)
i18n/        translations
```
