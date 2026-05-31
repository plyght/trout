# Trout PRD: Open Source Cotypist Alternative

## Purpose

Build an open source, local-first macOS writing assistant that provides fast inline text completions across ordinary Mac text fields. The product should feel closer to a smarter keyboard/autocomplete layer than a chatbot: suggestions appear while the user is already writing, the user stays in control, and accepting text should be possible word-by-word or as a full completion.

This document is based on a product reconnaissance pass through Cotypist 2026.1.1 settings in the local app at `~/Applications/Cotypist.app`, plus public product, pricing, and compatibility pages from `cotypist.app`.

## Product Principles

- Local-first by default: completions, context processing, and personalization should run on-device.
- Flow-preserving: suggestions should appear where the user is already typing, without opening a chat panel or prompting workflow.
- User voice over generated prose: optimize for completing what the user was likely going to write, not replacing the writer.
- Explicit user control: ignored suggestions should disappear naturally; accepted suggestions should be deliberate and reversible.
- Privacy-visible: every feature that reads screen, clipboard, or typing history needs clear controls and explanations.
- App-aware: behavior should be tunable per app/domain because autocomplete is useful in some contexts and disruptive in others.

## Target Platform

Initial target should be macOS. Cotypist currently positions itself around macOS 14+ on Apple Silicon, but this project can decide whether to support Intel Macs based on model performance and developer bandwidth.

Observed Cotypist implementation signals:

- The inspected app is a menu-bar/background agent (`LSUIElement`) rather than a normal Dock app.
- No bundled keyboard/input-method extension was visible in the app bundle, and no Cotypist input method was found under `~/Library/Input Methods` during inspection.
- A separate local project under `/Users/nicojaffer/indent` contains an `IndentInputMethod` using `InputMethodKit`; this is not Cotypist evidence, but it is a useful local architecture reference for a keyboard/input-method approach.
- The app relies on macOS Accessibility permission for reading/inserting text and Screen Recording permission for screen-aware context and/or visual placement.
- Local support files include a GGUF model, SQLite database files, completion statistics JSON, and preferences/cache files under `~/Library/Application Support/app.cotypist.Cotypist`.
- The app uses a custom URL scheme (`cotypist://`) and Sparkle update feed.

Suggested implementation direction:

- Core language runtime: C or C++.
- Inference backend: `llama.cpp` or another local GGUF-compatible runtime.
- Settings/debug UI: Dear ImGui is plausible for a portable open source admin window, especially if avoiding SwiftUI/AppKit-heavy settings UI. Native AppKit glue will still likely be needed for macOS accessibility, input monitoring, window overlays, menu bar behavior, and permissions.
- Persistence: SQLite or a small embedded store for settings, app profiles, local typing history metadata, and statistics.

## MVP Feature Set

### 1. Inline Completion Engine

The app observes focused text fields and generates short completions based on the current text context.

Requirements:

- Detect active text field and cursor context through macOS Accessibility APIs where possible.
- Generate completions with a local model.
- Show suggestions inline when the host app exposes enough text geometry.
- Provide a non-inline fallback overlay or mirror window when text geometry is unavailable.
- Hide or refresh suggestions as the user keeps typing.
- Avoid password fields and other protected secure input areas.
- Keep latency low enough that suggestions appear during normal typing rather than after the user has moved on.

Acceptance behaviors:

- Accept next word with `Tab`.
- Accept full completion with a configurable shortcut.
- Continue typing to reject or update the suggestion.
- Support completing text at the end of a sentence first; mid-line completion can be a later feature.

### 2. Word-by-Word Acceptance

Cotypist treats partial acceptance as a core interaction. This alternative should too.

Requirements:

- Pressing the primary accept key inserts only the next word by default.
- Repeated presses continue accepting one word at a time.
- A separate shortcut accepts the full completion.
- Optional setting to include the trailing space when accepting a single word.

Default shortcuts observed in Cotypist:

- Next word: `Tab`
- Full completion: backtick key
- Force activate completions: `Ctrl` + backtick
- Temporarily toggle completions in current app: `Ctrl` + `Option` + `Command` + backtick
- Global completion toggle: unset by default

### 3. Global Settings

Observed settings to include:

- Launch automatically at login.
- Show menu bar icon.
- Show floating accessory button near text fields.
- Select AI model.
- Reveal local model folder.
- Enable completions by default.
- Maximum completion length, with short/medium/long style options.
- Anonymous diagnostics opt-in, if telemetry exists at all.
- Hide completions when a typo is suspected.
- Show suggested typo fixes inline.

For open source, telemetry should be disabled by default or absent. If crash reporting exists, it should be self-hostable and clearly opt-in.

### 4. Context Controls

Observed context features:

- Use screenshots for context.
- Use screenshots to improve suggestion appearance.
- Use clipboard for context.

Requirements:

- Treat screen and clipboard access as separate permissions/features.
- Explain what is read, where it is processed, whether it is stored, and how to disable it.
- Clipboard context should be off by default unless the user explicitly enables it.
- Screenshot context should be optional and locally processed only.

### 5. Personalization

Observed personalization features:

- Collect text-field inputs for personalization.
- Store inputs even without accepted completions.
- Local encrypted storage.
- Per-user word-choice personalization slider.
- Existing-data counter and delete-all control.
- Global custom AI instructions field.

Requirements:

- Maintain an optional local writing profile.
- Let the user disable collection globally.
- Let the user choose whether only accepted-completion sessions are stored or all monitored inputs are stored.
- Provide a personalization strength slider.
- Provide global custom instructions for role, tone, language, domain, vocabulary, and writing style.
- Provide data deletion.
- Make storage format and encryption strategy auditable.

Open-source-friendly baseline:

- Start with custom instructions and a local preference profile before building heavy fine-tuning.
- Use retrieval, prompt-prefixing, or lightweight statistical personalization before attempting model adaptation.

### 6. Emoji Suggestions

Observed emoji features:

- Enable emoji suggestions.
- Trigger with colon syntax, e.g. `:sweat_smile:`.
- Preferred skin tone.
- Include neutral variant.
- Preferred gender for person emoji.

Requirements:

- Colon-triggered emoji search.
- Configurable skin tone and gender variants.
- Fallback to macOS emoji picker is acceptable but should not be required for common matches.

### 7. Per-App and Per-Domain Settings

Observed app settings:

- Search apps/domains.
- Track app-specific collected inputs.
- Enable/disable completions per app.
- Enable/disable mid-line completions per app.
- Enable/disable autocorrect per app.
- Disable Tab key per app.
- Compatibility mode per app.
- App-specific custom AI instructions.
- App-specific typing-history collection.
- Delete collected inputs for a specific app.

Requirements:

- Maintain app profiles keyed by bundle ID.
- Maintain domain profiles for browser text fields where the domain is detectable.
- Each setting should support default/inherit, on, and off where appropriate.
- Provide compatibility mode for apps with unusual text-field behavior.
- Allow users to disable completions for password managers, IDE main editors, games, terminals, or any app where suggestions are disruptive.

### 8. App Compatibility Model

Public compatibility notes suggest these categories:

- Works out of the box in many browsers, email apps, notes apps, messaging apps, and standard text fields.
- Some browser/document apps need accessibility setup, such as Google Docs screen reader/braille support.
- Some Chromium-based browsers may need text metrics or renderer accessibility flags for inline suggestions.
- Some apps may only support a mirror/overlay suggestion window.
- Code editors should prioritize sidebar AI chat prompts rather than main code editors.
- Terminals should activate for AI-agent prompts but avoid interfering with regular shell completions by default.

Observed compatibility matrix from Cotypist:

- Browsers: Safari, Chrome, Brave, and Edge work; Arc and Dia need setup; Firefox and Zen Browser are mirror-window-only.
- Email: Apple Mail, Gmail in browser, Outlook, and Mimestream work; Thunderbird is not supported.
- Writing/documents: Microsoft Word, Google Docs with setup, and TextEdit work; Pages and Scrivener are not supported.
- Notes/knowledge: Apple Notes, Notion, and Obsidian work; OneNote is not supported.
- Messaging: Messages, Teams, and WhatsApp work; Slack is partial.
- Code editors: VS Code, Cursor, and Windsurf are sidebar-chat-only; BBEdit and Sublime Text are not supported.
- Terminals: Terminal.app and iTerm work; Ghostty, cmux, and Warp are not supported.

MVP compatibility targets:

- Safari/Chrome text fields.
- Apple Mail.
- Notes/TextEdit.
- Slack or another common Electron text field.
- Terminal.app/iTerm when typing natural-language AI-agent prompts.

Known difficult targets:

- Apps with custom text rendering and insufficient accessibility support.
- Main code editor buffers in VS Code/Cursor/Windsurf.
- Some terminal emulators and browser variants.

Compatibility recipes to document and potentially automate:

- Google Docs requires Google Docs Accessibility mode. Manual setup is Tools > Accessibility, then enable screen reader support and braille support. Recommend disabling Google Docs Smart Compose under Tools > Preferences to avoid conflicting suggestions. Google Sheets and Google Slides are out of scope for this recipe, and Google Docs in Firefox is not supported by Cotypist.
- Arc and Dia can use inline suggestions if Chromium text metrics are enabled at `chrome://accessibility`; this may need to stay open/pinned and may not persist across restarts or Space switches.
- Arc and Dia can also be launched with `--force-renderer-accessibility=complete` for a more persistent session-level workaround.
- Firefox and Zen Browser should start with mirror-window support only unless a reliable inline text-metrics path is discovered.
- Terminal integrations should detect AI-agent prompt contexts and remain quiet for ordinary shell input unless the user force-activates completions.

### 8.1 Positioning, Use Cases, and Voice

The product is not a general AI writing generator. It should complete the user's sentence in the user's voice.

Observed website positioning:

- "Augmenting your writing, not replacing it."
- No chatbot detour: the user should not stop, prompt, wait, and edit.
- Accuracy improves with longer thoughtful writing where there is more context.
- Ignored suggestions should adapt within a letter or two where possible.
- Partial acceptance is important because often only part of a suggestion is right.

Primary use cases:

- Email replies.
- AI prompts for ChatGPT, Claude, Codex, and other coding agents.
- Marketing copy.
- Social posts.
- Customer support replies.
- Documentation.
- Longer-form writing.

Accessibility and inclusion use cases:

- Non-native English writers.
- Multilingual/code-switching writers.
- Dyslexic typists.
- One-handed typists.
- Users with hand pain, arthritis, RSI, or slow typing speed.

Language expectations:

- English should be the strongest default.
- Multilingual text should be supported where the selected model can handle it.
- Support UI in Cotypist mentioned English and German; this project should not hard-code that as a limit.

### 8.2 System and Performance Expectations

Observed Cotypist constraints:

- macOS 14+.
- Apple Silicon only.
- Recommended class: M1 Pro or M2 or newer with 16 GB RAM.
- Active memory usage claim: roughly 1-2.5 GB.
- Suggestions should feel almost instant on recommended hardware.
- Local inference can be battery-intensive, so model choice and generation limits matter.

Open source requirements:

- Run a first-launch benchmark and recommend a model based on latency and memory.
- Prefer short completions by default because long completions are slower and more likely to diverge from intent.
- Expose energy/performance tradeoffs in model selection.
- Keep a degraded mode for weaker machines, such as smaller models, shorter context, and overlay-only suggestions.

### 9. Autocorrect

Observed autocorrect behavior:

- Detect suspected typo in the current word and hide completions to avoid extending bad input.
- Optionally show suggested fixes inline as strikethrough typo plus replacement.
- Positioned as current-word correction, not a full spell-checker.

Requirements:

- Current-word typo detection.
- Suppress completion when typo confidence is high.
- Optional inline correction suggestion.
- Accept correction through the same completion flow.

### 10. Statistics

Observed statistics:

- Today's completions, words, and characters.
- Completion statistics chart.
- Time range selector.
- Group-by selector.
- Metric selector.
- Total and daily average over active days.

Requirements:

- Track accepted completions, accepted words, accepted characters.
- Track active days and daily averages.
- Keep stats local.
- Support at least day/week/month time ranges.
- Provide enough stats for the user to judge whether the tool is actually saving typing.

### 11. Setup and Permissions

Observed setup checklist:

- Accessibility permission.
- Screen Recording permission.
- AI model download.
- macOS text suggestions disabled to avoid conflicts.
- Clipboard context prompt.

Requirements:

- First-run setup checklist.
- Permission status detection.
- Clear explanation of why each permission is needed.
- Model download/import flow.
- Ability to disable or warn about conflicting macOS text suggestions if possible.
- No account requirement for open source baseline.

### 12. Support and Diagnostics

Observed support window:

- Subject category.
- Message body.
- Name/email.
- Attached screenshot.
- Attached `MainPreferences.json`.
- Supported languages shown as English and German in support UI.

Open source adaptation:

- Replace hosted support form with diagnostics export.
- Let users generate a redacted support bundle.
- Include app version, OS version, selected model, settings JSON, logs, and compatibility traces.
- Never include screenshots, clipboard, or writing history unless explicitly selected.

## Feature Tiers For Project Planning

Because this is open source, avoid Cotypist's commercial tiering. Use phases instead.

### Phase 0: Research Prototype

- Global hotkey.
- Read focused text from a small set of native text fields.
- Generate local completion from current paragraph.
- Display a simple overlay.
- Accept full suggestion.

### Phase 1: Usable MVP

- Menu bar app.
- First-run permissions.
- Model management.
- Inline or overlay suggestions.
- Next-word `Tab` acceptance.
- Full-completion shortcut.
- Global enable/disable.
- Per-app enable/disable.
- Completion length setting.
- Local-only stats.
- Basic typo suppression.

### Phase 2: Strong Daily Driver

- App/domain profiles.
- App-specific instructions.
- Clipboard context.
- Screenshot context for surrounding text and placement.
- Emoji suggestions.
- Current-word autocorrect suggestions.
- Terminal AI-agent detection.
- Compatibility mode and mirror window fallback.
- Diagnostics export.

### Phase 3: Differentiators

- Local personalization profile.
- Encrypted local typing-history store.
- Personalization strength slider.
- Mid-line completions.
- Better multilingual handling.
- Compatibility recipe system for specific apps.
- Optional community-shared app compatibility database.

## Non-Goals

- Do not build a chatbot.
- Do not send user writing to a hosted model by default.
- Do not clone Cotypist's branding, copy, paid tier structure, or visual identity.
- Do not target code completion in main IDE buffers as a primary feature.
- Do not implement cloud accounts or subscription logic for the open source baseline.

## Privacy Requirements

- All model inference must run locally unless the user explicitly configures a remote endpoint.
- Clipboard context off by default.
- Typing history off by default or introduced with an explicit consent screen.
- User can clear global and per-app stored data.
- Password fields and secure inputs must be ignored.
- Logs must avoid storing raw user text by default.
- Diagnostics exports must be inspectable before sharing.
- The app should make clear that the user's words and data stay on-device in the default configuration.
- The app should not claim ownership over text the user writes with assistance; position it like a spell checker or editor, while avoiding legal guarantees.

## Open Technical Questions

- Can a C/C++ app reliably handle the macOS Accessibility, InputMethod, overlay-window, and permissions flows without too much Objective-C/AppKit glue?
- Should Dear ImGui be used only for settings/debug UI, while suggestion overlays remain native transparent windows?
- Which local model size gives acceptable latency on common Apple Silicon Macs?
- Should personalization be implemented with prompt/context retrieval, local embeddings, LoRA/fine-tuning, or simpler frequency/style heuristics?
- How much text context can be retrieved from each target app without privacy or performance problems?
- What is the safest way to detect terminal AI-agent prompts without reading arbitrary shell content too aggressively?

## Reference Architecture Sketch

- `troutd`: background daemon/service responsible for accessibility observation, context extraction, inference scheduling, and insertion.
- `trout-ui`: menu bar/settings application, potentially C++/ImGui plus AppKit shell.
- `trout-overlay`: native transparent overlay layer for suggestions and mirror windows.
- `trout-models`: model registry, downloader/importer, GGUF metadata, benchmark/recommendation logic.
- `trout-store`: SQLite settings, per-app profiles, local stats, optional encrypted personalization history.
- `trout-compat`: app/domain compatibility profiles and workarounds.

## macOS Input and Permission Model

Preferred direction after research: build around an InputMethodKit input source as the main typing integration, then use Accessibility, event taps, and overlays as supporting systems.

Apple's `IMKInputController` is the input-method-side controller for each client input session; `IMKServer` creates a controller per session. This makes InputMethodKit the closest native fit for owning text input, receiving typed text, and inserting committed text through the client where supported. Apple's Quartz Event Services provide event taps for observing/intercepting events globally, and AXUIElement APIs are the supported route for assistive apps to inspect and control accessible UI elements across processes.

Core split:

- InputMethodKit owns active text-session integration where the user has selected the Trout input source.
- Accessibility observes focused app, focused UI element, selected text, cursor movement, and nearby text when IMK does not expose enough context.
- Screen Recording remains optional but useful for visual context, screenshot-aware suggestions, and overlay placement in hostile/custom-rendered apps.
- Transparent overlay or mirror windows show suggestions when the host client cannot render marked/inline text itself.
- Event taps or hotkey registration handle global shortcuts, fallback accept/dismiss keys, and app-level toggles.
- Per-app/domain compatibility profiles remain necessary because each host app exposes text fields differently.

The implementation should not assume InputMethodKit removes the need for Accessibility. IMK helps with key ownership and committed text insertion, but AX is still needed for cross-app context, cursor geometry, app detection, nonstandard web editors, compatibility diagnostics, and mirror-window fallback.

Local architecture reference from `/Users/nicojaffer/indent`:

- `IndentInputMethod` subclasses `IMKInputController`, handles Tab keydown, consumes it, and posts a distributed notification.
- A paired app uses a `CGEvent.tapCreate` keydown event tap to intercept accept/dismiss keys when a suggestion is visible.
- Overlay placement is done with a borderless transparent floating `NSWindow`, positioned via `AXFocusedUIElement`, `AXSelectedTextRange`, and `AXBoundsForRange`.
- Text insertion is done by reading and setting `kAXValueAttribute`, then updating `kAXSelectedTextRangeAttribute`.
- Accessibility permission is checked with `AXIsProcessTrustedWithOptions`.

Implementation note: InputMethodKit should be the default main path for Trout, but with a proof-of-compatibility milestone before deep product work. The first prototype should verify that an IMK input source can:

- Be installed and selected reliably from macOS Keyboard > Input Sources.
- Receive ordinary typing events in native AppKit text fields, browser text fields, Electron apps, and terminal prompts.
- Insert committed text with `IMKTextInput.insertText(...replacementRange:)` or equivalent client APIs.
- Preserve normal typing latency and not create obvious input-source switching lag.
- Handle Tab/accept semantics without breaking apps where Tab means indentation or focus traversal.
- Fall back cleanly to AX/event-tap insertion when the client does not cooperate.

Risks found during research:

- InputMethodKit examples often require special bundle metadata such as `InputMethodConnectionName`, `InputMethodServerControllerClass`, a background-only app, and a bundle identifier containing `.inputmethod.`.
- IMK is old and under-documented; modern Swift concurrency can be awkward. A C++/Objective-C++ or Objective-C wrapper around IMK may be more predictable than a pure Swift implementation.
- Event taps can be disabled by timeout or user input and must be re-enabled.
- Secure Keyboard Input can prevent other apps from intercepting key events while passwords or private fields are active.
- AX calls can fail when apps do not implement the accessibility surface fully, return stale elements, or expose custom-rendered editors poorly.

## Success Metrics

- Median suggestion latency feels instantaneous during normal typing.
- User can accept next-word completions repeatedly without breaking flow.
- User can disable or tune behavior per app in under 30 seconds.
- No raw user writing leaves the machine in the default configuration.
- The app works in at least five common macOS writing surfaces.
- The user can inspect, export, and delete local data.
