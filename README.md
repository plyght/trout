<div align='center'>
    <h3>trout</h3>
    <p>local-first inline completions for macOS</p>
    <br/>
</div>

Trout is a native macOS writing assistant that provides fast inline text completions across ordinary Mac text fields. It is designed as a smarter autocomplete layer rather than a chatbot: suggestions appear while you write, stay local by default, and can be accepted one word at a time or as a full completion.

## Features

- **Inline Completions**: Observes focused text fields through macOS Accessibility APIs and generates short completions from the surrounding text
- **Word-by-Word Acceptance**: Accepts the next suggested word with `Tab`, or the full suggestion with backtick when a suggestion is visible
- **Local Inference**: Uses `llama.cpp` with GGUF models, Metal, and Accelerate on Apple platforms when enabled
- **Menu-Bar Agent**: Runs as a background accessory app without a Dock icon, with settings and controls available from the menu bar
- **Input Method Bridge**: Includes an InputMethodKit bundle that can consume completion shortcuts and commit accepted text into the active app
- **Fallback Overlay**: Displays suggestions in a lightweight AppKit overlay near the caret when text geometry is available
- **Privacy Controls**: Keeps clipboard and screenshot context disabled unless explicitly enabled in settings
- **Per-App Profiles**: Stores app-specific completion behavior, compatibility settings, and custom instructions
- **Model Management**: Discovers `.gguf` models from Trout's local model folder and includes catalog metadata for recommended Qwen2.5 models
- **Local Persistence**: Stores settings, app profiles, completion stats, and runtime state in SQLite under Application Support

## Install

```bash
# From source
git clone https://github.com/plyght/trout.git
cd trout
git submodule update --init --recursive
scripts/build.sh Release
open build/platform/macos/Trout.app
```

Trout requires macOS 14+, CMake 3.21+, a C++17 compiler, Objective-C++/AppKit toolchains, and the bundled submodules for `llama.cpp`, Dear ImGui, and nlohmann/json.

To build without the `llama.cpp` backend:

```bash
scripts/build.sh Release --no-llama
```

## Usage

```bash
# Build and launch the menu-bar app
scripts/build.sh Release
open build/platform/macos/Trout.app

# Install the input method bundle
scripts/install-inputmethod.sh
```

After installing the input method, open System Settings > Keyboard > Input Sources and add `Trout`.

Trout prompts for Accessibility permission on first launch. Completions are disabled until permission is granted because the app needs Accessibility access to read focused text fields and insert accepted suggestions.

Default shortcuts:

```text
Tab                 Accept next word
`                   Accept full completion
Ctrl+`              Force completion request
Escape              Dismiss current suggestion
Ctrl+Opt+Cmd+`      Toggle completions for current app
```

## Models

Trout looks for local GGUF models in:

```text
~/Library/Application Support/app.trout.Trout/models
```

The model browser recognizes installed `.gguf` files in that folder. Recommended model IDs in the current catalog are:

```text
qwen2.5-0.5b-instruct-q4_k_m
qwen2.5-1.5b-instruct-q4_k_m
qwen2.5-3b-instruct-q4_k_m
```

A selected model must be present before the completion engine can produce suggestions.

## Configuration

Runtime files are stored under:

```text
~/Library/Application Support/app.trout.Trout
```

Important files and folders:

```text
trout.sqlite3    Settings, profiles, and completion statistics
models/          Local GGUF model files
diagnostics/     Local diagnostics output
trout.log        Runtime log file
```

The settings UI controls:

```text
Launch at login
Menu bar icon visibility
Selected model
Global completion enablement
Maximum completion length
Typo suppression
Clipboard and screenshot context
Personalization and input collection
Emoji suggestion preferences
Per-app completion profiles
Shortcut descriptors
```

Clipboard and screenshot context are separate settings and are off by default.

## Architecture

- `app/main.mm`: AppKit entry point, accessory activation policy, and application delegate
- `platform/macos/AppController.*`: Service orchestration, engine lifecycle, key handling, suggestions, and acceptance flow
- `platform/macos/Accessibility.*`: Focused text capture, caret geometry, secure-field checks, and text insertion
- `platform/macos/EventTap.*`: Global key event tap for completion shortcuts and typing debounce
- `platform/macos/Overlay.*`: Suggestion overlay window shown near the caret
- `platform/macos/MenuBarController.*`: Menu-bar status item and app controls
- `platform/macos/SettingsWindow.*`: Dear ImGui settings interface backed by Metal/AppKit
- `platform/macos/Permissions.*`: Accessibility permission checks and prompts
- `inputmethod/*`: InputMethodKit app bundle for shortcut handling and committed text insertion
- `core/include/trout/*`: Portable core APIs for settings, storage, models, inference, and completion orchestration
- `core/src/Engine.cpp`: Context-to-prompt pipeline, worker thread, cancellation, post-processing, and acceptance stats
- `core/src/LlamaBackend.cpp`: `llama.cpp` inference backend
- `core/src/InferenceStub.cpp`: Stub inference backend for non-llama builds
- `core/src/Store.cpp`: SQLite-backed persistence
- `core/src/Models.cpp`: Local GGUF discovery, model catalog, and model import helpers
- `third_party/llama.cpp`: Local inference backend
- `third_party/imgui`: Settings UI renderer
- `third_party/json`: nlohmann/json headers

The project is split between a portable C++ core and macOS-specific Objective-C++ adapters. The core owns settings, storage, model discovery, inference, prompt construction, suggestion post-processing, and statistics. The macOS layer owns permissions, text-field integration, overlays, menu-bar behavior, settings UI, and keyboard event handling.

## Development

```bash
# Configure and build with the default Release profile
scripts/build.sh Release

# Debug build
scripts/build.sh Debug

# Configure manually
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DTROUT_WITH_LLAMA=ON -G "Unix Makefiles"

# Build manually
cmake --build build -j"$(sysctl -n hw.ncpu)"

# Run
open build/platform/macos/Trout.app
```

Useful checks:

```bash
# Clean reconfigure
rm -rf build
scripts/build.sh Debug

# Build without local inference backend
scripts/build.sh Debug --no-llama

# Install or refresh the input method
scripts/install-inputmethod.sh
```

Key dependencies: CMake, C++17, Objective-C++, AppKit, InputMethodKit, ApplicationServices, CoreGraphics, Carbon, Metal, MetalKit, QuartzCore, SQLite, Dear ImGui, nlohmann/json, and `llama.cpp`.
