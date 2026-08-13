# WhisperFlow

Small desktop app for local speech-to-text. Hit record (or press the global
hotkey), talk, and it transcribes with [whisper.cpp](https://github.com/ggml-org/whisper.cpp).
You can also drag an audio file onto the window. Nothing gets uploaded
anywhere, transcription runs on your CPU.

Written in C++17 with Qt 6 Widgets. Same tree builds on macOS and Windows.

Hotkey is Cmd+Shift+R on Mac, Ctrl+Shift+R on Windows. It works even when
the app is in the background.

## Models

Pick a model in Settings. It gets downloaded from the
[whisper.cpp HF repo](https://huggingface.co/ggerganov/whisper.cpp) the first
time and cached in the app data folder.

| Model | Size |
|---|---|
| Tiny | ~78 MB |
| Base (default) | ~148 MB |
| Small | ~488 MB |
| Medium | ~1.5 GB |
| Large v3 turbo | ~1.6 GB |

Tiny is fine for quick notes. Small is the sweet spot if your machine can
take it. Transcripts are kept as JSON, and the recorded audio is saved per
transcript so you can replay it or re-run it later with a bigger model.

## Building

You need CMake 3.19+, git, and Qt 6.5+ with the Widgets, Svg, Network,
Multimedia and Concurrent modules. First configure clones whisper.cpp via
FetchContent, so network is required once.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Or just open CMakeLists.txt in Qt Creator.

On Windows run windeployqt on the built exe if you want to move it to
another machine. whisper.cpp is compiled CPU-only so the same setup works on
both platforms; if you only care about Mac, set GGML_METAL to ON in
CMakeLists.txt and it gets a lot faster.

## Code layout

```
src/ui        window, transcript cards, record button, settings dialog
src/core      whisper engine, model downloads, audio capture and storage
src/platform  global hotkey (Carbon on Mac, RegisterHotKey on Windows)
```

## License

MIT, see [LICENSE](LICENSE).
