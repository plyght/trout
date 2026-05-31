# Trout

Trout is a free, open source, local-first macOS writing assistant — an
alternative to Cotypist. It provides fast inline text completions across
ordinary Mac text fields. It feels like a smarter autocomplete layer, not a
chatbot: suggestions appear while you write, you stay in control, and you can
accept text word-by-word or all at once.

All inference runs **on-device** by default. Your words stay on your machine.

## Architecture

Trout is written in **C/C++** with a thin **Objective-C++** layer for macOS
APIs. The GUI is built with **Dear ImGui** (Metal backend). Local inference uses
**llama.cpp**.

| Component | Language | Purpose |
|-----------|----------|---------|
| `trout-core` | C++17 | Settings, SQLite store, stats, completion engine, model registry, emoji, app-compat database, inference abstraction |
| `LlamaBackend` | C++ | llama.cpp GGUF inference (Metal-accelerated) |
| `platform/macos` | Objective-C++ | Menu-bar agent, Accessibility read/insert, CGEvent tap, transparent overlay, permissions, ImGui settings window |
| `inputmethod` | Objective-C++ | InputMethodKit input source (`IMKInputController`) — the preferred long-term typing integration |

The macOS app is a background menu-bar agent (`LSUIElement`), not a Dock app.

### Data locations

Everything lives under `~/Library/Application Support/app.trout.Trout`:

- `trout.sqlite3` — settings, per-app profiles, stats, optional typing history
- `models/` — your `.gguf` models
- `trout.log` — logs (no raw user text by default)

## Building

Requirements: macOS 14+, Apple Silicon recommended, Xcode command line tools,
CMake 3.21+.

```bash
git clone --recurse-submodules <repo-url> trout
cd trout
scripts/build.sh Release
```

Outputs:

- `build/platform/macos/Trout.app` — the menu-bar app
- `build/inputmethod/TroutIM.app` — the input method bundle

To build without the llama.cpp backend (uses a deterministic stub backend,
useful for UI work):

```bash
scripts/build.sh Debug --no-llama
```

## Running

1. Get a GGUF model (e.g. a small instruct model like Qwen2.5 0.5B/1.5B) and
   place it in `~/Library/Application Support/app.trout.Trout/models/`.
   The Model tab lists suggested downloads.
2. Launch `Trout.app`. Grant **Accessibility** permission when prompted
   (System Settings > Privacy & Security > Accessibility).
3. Open **Settings** from the menu-bar fish icon, go to the **Model** tab, pick
   your model, and click **Apply & Load Model**.
4. Start typing in a supported text field. A ghost suggestion appears near the
   caret.

### Keys

| Action | Default |
|--------|---------|
| Accept next word | `Tab` |
| Accept full completion | `` ` `` (backtick) |
| Force activate completions | `Ctrl` + `` ` `` |
| Dismiss suggestion | `Esc` |

### Optional: install the input method

The CGEvent-tap path works out of the box. To use the InputMethodKit path:

```bash
scripts/install-inputmethod.sh
```

Then add **Trout** under System Settings > Keyboard > Input Sources.

## Permissions

- **Accessibility** (required): read focused text fields and insert completions.
- **Screen Recording** (optional): screenshot context and overlay placement in
  apps that render text without exposing it to Accessibility.

## Privacy

- All model inference runs locally. Nothing is sent off the machine in the
  default configuration.
- Clipboard context is **off** by default.
- Typing-history collection is **off** by default.
- Password and secure-input fields are ignored.
- You can delete all collected data from the Personalization tab.

## App compatibility

Trout ships a built-in compatibility database (see the Apps tab) describing how
well known apps expose their text fields, with setup recipes for browsers like
Arc/Dia and document apps like Google Docs.

## Status

This is an early implementation covering the Phase 0/1 feature set: menu-bar
app, permissions, model management, Accessibility-based read/insert, overlay
suggestions, word-by-word and full acceptance, per-app enable/disable, local
stats, basic typo suppression, emoji shortcode matching, and the InputMethodKit
scaffold. See `prd.md` for the full roadmap.

## License

Open source. Bundled dependencies (Dear ImGui, llama.cpp, nlohmann/json) retain
their own permissive licenses.
