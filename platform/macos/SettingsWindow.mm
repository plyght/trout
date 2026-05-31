#import "SettingsWindow.h"
#import "AppController.h"
#import "Permissions.h"

#include "trout/Settings.h"
#include "trout/Store.h"
#include "trout/Models.h"
#include "trout/Compat.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_osx.h"

#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

using namespace trout;

static ImVec2 io_size(MTKView* v);

// ---------------------------------------------------------------------------
// MTKView delegate that drives the ImGui frame loop.
// ---------------------------------------------------------------------------
@interface TroutImGuiRenderer : NSObject <MTKViewDelegate>
- (instancetype)initWithDevice:(id<MTLDevice>)device app:(AppController*)app;
@end

@implementation TroutImGuiRenderer {
    id<MTLCommandQueue> _queue;
    __weak AppController* _app;
    int _tab;
    // transient UI buffers
    char _globalInstr[1024];
    char _appInstr[1024];
    char _appSearch[128];
    bool _loadedBuffers;
}

- (instancetype)initWithDevice:(id<MTLDevice>)device app:(AppController*)app {
    if ((self = [super init])) {
        _queue = [device newCommandQueue];
        _app = app;
        _tab = 0;
        _loadedBuffers = false;
        _globalInstr[0] = 0; _appInstr[0] = 0; _appSearch[0] = 0;
    }
    return self;
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {}

- (void)drawInMTKView:(MTKView*)view {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = view.bounds.size.width;
    io.DisplaySize.y = view.bounds.size.height;
    CGFloat scale = view.window.screen.backingScaleFactor ?: 1.0f;
    io.DisplayFramebufferScale = ImVec2(scale, scale);

    id<MTLCommandBuffer> cmd = [_queue commandBuffer];
    MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
    if (!rpd) { return; }
    rpd.colorAttachments[0].clearColor = MTLClearColorMake(0.09, 0.09, 0.10, 1.0);

    id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
    [enc pushDebugGroup:@"trout-imgui"];

    ImGui_ImplMetal_NewFrame(rpd);
    ImGui_ImplOSX_NewFrame(view);
    ImGui::NewFrame();

    [self buildUI:view];

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd, enc);

    [enc popDebugGroup];
    [enc endEncoding];
    [cmd presentDrawable:view.currentDrawable];
    [cmd commit];
}

- (Settings*)settings { return (Settings*)[_app settingsPtr]; }
- (Store*)store { return (Store*)[_app storePtr]; }
- (Models*)models { return (Models*)[_app modelsPtr]; }
- (Compat*)compat { return (Compat*)[_app compatPtr]; }

- (void)buildUI:(MTKView*)view {
    Settings* settings = [self settings];
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io_size(view));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Trout", nullptr, flags);

    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("General")) { [self generalTab:settings]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Model")) { [self modelTab:settings]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Completions")) { [self completionsTab:settings]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Context")) { [self contextTab:settings]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Personalization")) { [self personalizationTab:settings]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Apps")) { [self appsTab:settings]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Stats")) { [self statsTab]; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Permissions")) { [self permissionsTab]; ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

static ImVec2 io_size(MTKView* v) { return ImVec2(v.bounds.size.width, v.bounds.size.height); }

- (void)generalTab:(Settings*)settings {
    GlobalSettings g = settings->global();
    bool changed = false;
    changed |= ImGui::Checkbox("Launch at login", &g.launchAtLogin);
    changed |= ImGui::Checkbox("Show menu bar icon", &g.showMenuBarIcon);
    changed |= ImGui::Checkbox("Show floating accessory button", &g.showAccessoryButton);
    changed |= ImGui::Checkbox("Enable completions", &g.completionsEnabled);
    ImGui::Separator();
    ImGui::TextWrapped("Trout is local-first. Your text stays on this machine in the default configuration.");
    if (changed) settings->setGlobal(g);
}

- (void)modelTab:(Settings*)settings {
    GlobalSettings g = settings->global();
    Models* models = [self models];

    ImGui::TextWrapped("Selected model (absolute path to a .gguf file):");
    static char buf[1024];
    if (ImGui::IsWindowAppearing()) {
        strncpy(buf, g.selectedModelId.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf)-1] = 0;
    }
    if (ImGui::InputText("##model", buf, sizeof(buf))) {
        g.selectedModelId = buf;
        settings->setGlobal(g);
    }
    if (ImGui::Button("Apply & Load Model")) {
        g.selectedModelId = buf;
        settings->setGlobal(g);
        [_app reloadEngineModel];
    }
    ImGui::SameLine();
    if (ImGui::Button("Reveal Models Folder")) { [_app revealModelsFolder]; }

    ImGui::Separator();
    ImGui::Text("Installed models in folder:");
    for (auto& m : models->installed()) {
        ImGui::BulletText("%s  (%.0f MB)", m.displayName.c_str(), m.sizeBytes / 1e6);
        ImGui::SameLine();
        ImGui::PushID(m.id.c_str());
        if (ImGui::SmallButton("Use")) {
            g.selectedModelId = m.path;
            settings->setGlobal(g);
            strncpy(buf, m.path.c_str(), sizeof(buf)-1); buf[sizeof(buf)-1]=0;
            [_app reloadEngineModel];
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::Text("Suggested downloads (open in browser, save to models folder):");
    for (auto& c : Models::catalog()) {
        ImGui::BulletText("%s  (~%.0f MB)", c.displayName.c_str(), c.approxSizeBytes / 1e6);
        ImGui::TextWrapped("    %s", c.notes.c_str());
    }
}

- (void)completionsTab:(Settings*)settings {
    GlobalSettings g = settings->global();
    bool changed = false;

    const char* lengths[] = {"Short", "Medium", "Long"};
    int li = (int)g.maxCompletionLength;
    if (ImGui::Combo("Max completion length", &li, lengths, 3)) {
        g.maxCompletionLength = (CompletionLength)li; changed = true;
    }
    changed |= ImGui::Checkbox("Include trailing space on word accept", &g.includeTrailingSpaceOnWordAccept);
    ImGui::Separator();
    changed |= ImGui::Checkbox("Hide completions on suspected typo", &g.hideOnSuspectedTypo);
    changed |= ImGui::Checkbox("Show inline typo fix suggestions", &g.showInlineTypoFix);
    ImGui::Separator();
    changed |= ImGui::Checkbox("Emoji suggestions (:shortcode:)", &g.emojiSuggestions);
    {
        const char* tones[] = {"default","light","medium-light","medium","medium-dark","dark"};
        int ti = 0;
        for (int i = 0; i < 6; ++i) if (g.emojiSkinTone == tones[i]) ti = i;
        if (ImGui::Combo("Emoji skin tone", &ti, tones, 6)) { g.emojiSkinTone = tones[ti]; changed = true; }
    }
    ImGui::Separator();
    ImGui::Text("Shortcuts (descriptors):");
    ImGui::BulletText("Accept next word: %s", g.acceptWordShortcut.c_str());
    ImGui::BulletText("Accept full: %s", g.acceptFullShortcut.c_str());
    ImGui::BulletText("Force activate: %s", g.forceActivateShortcut.c_str());

    if (changed) settings->setGlobal(g);
}

- (void)contextTab:(Settings*)settings {
    GlobalSettings g = settings->global();
    bool changed = false;
    ImGui::TextWrapped("These features read additional context. They are off by default and processed locally.");
    ImGui::Separator();
    changed |= ImGui::Checkbox("Use clipboard for context", &g.useClipboardContext);
    ImGui::TextWrapped("    Reads the current clipboard text and includes it in the prompt for the next completion.");
    changed |= ImGui::Checkbox("Use screenshots for context", &g.useScreenshotContext);
    changed |= ImGui::Checkbox("Use screenshots to improve appearance", &g.useScreenshotForAppearance);
    ImGui::TextWrapped("    Requires Screen Recording permission. Locally processed only.");
    if (changed) settings->setGlobal(g);
}

- (void)personalizationTab:(Settings*)settings {
    GlobalSettings g = settings->global();
    Store* store = [self store];
    bool changed = false;

    changed |= ImGui::Checkbox("Collect text-field inputs for personalization", &g.collectInputs);
    changed |= ImGui::Checkbox("Store only accepted-completion sessions", &g.storeOnlyAcceptedSessions);
    changed |= ImGui::SliderFloat("Personalization strength", &g.personalizationStrength, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Global custom instructions:");
    if (ImGui::IsWindowAppearing()) {
        strncpy(_globalInstr, g.globalCustomInstructions.c_str(), sizeof(_globalInstr)-1);
        _globalInstr[sizeof(_globalInstr)-1] = 0;
    }
    if (ImGui::InputTextMultiline("##gi", _globalInstr, sizeof(_globalInstr), ImVec2(-1, 100))) {
        g.globalCustomInstructions = _globalInstr; changed = true;
    }

    ImGui::Separator();
    int count = store ? store->typingSampleCount("") : 0;
    ImGui::Text("Stored typing samples: %d", count);
    if (ImGui::Button("Delete all collected data") && store) {
        store->deleteTypingHistory("");
    }

    if (changed) settings->setGlobal(g);
}

- (void)appsTab:(Settings*)settings {
    Compat* compat = [self compat];
    ImGui::TextWrapped("Per-app behavior. Built-in compatibility info is shown for known apps.");
    ImGui::InputTextWithHint("##search", "Search apps", _appSearch, sizeof(_appSearch));
    ImGui::Separator();

    std::string q = _appSearch;
    for (auto& [bid, p] : compat->all()) {
        if (!q.empty() && p.displayName.find(q) == std::string::npos &&
            bid.find(q) == std::string::npos) continue;
        ImGui::PushID(bid.c_str());
        ImGui::Text("%s", p.displayName.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", Compat::supportLabel(p.support));

        auto prof = settings->appProfile(bid).value_or(AppProfile{});
        prof.bundleId = bid; prof.displayName = p.displayName;
        int sel = (int)prof.completionsEnabled; // 0 inherit,1 on,2 off
        const char* states[] = {"Inherit","On","Off"};
        if (ImGui::Combo("Completions", &sel, states, 3)) {
            prof.completionsEnabled = (TriState)sel;
            settings->setAppProfile(prof);
        }
        if (!p.recipe.empty()) ImGui::TextWrapped("  %s", p.recipe.c_str());
        ImGui::Separator();
        ImGui::PopID();
    }
}

- (void)statsTab {
    Store* store = [self store];
    if (!store) return;
    DayStat t = store->todayStat();
    ImGui::Text("Today");
    ImGui::BulletText("Completions: %d", t.completions);
    ImGui::BulletText("Words: %d", t.words);
    ImGui::BulletText("Characters: %d", t.characters);

    ImGui::Separator();
    ImGui::Text("Last 30 days");
    // Simple inline bar list.
    auto rows = store->dayStats("0000-00-00", "9999-99-99");
    int total = 0;
    for (auto& r : rows) total += r.completions;
    ImGui::Text("Total completions: %d over %zu active days", total, rows.size());
    if (!rows.empty()) ImGui::Text("Daily average: %.1f", (double)total / rows.size());
    for (auto it = rows.rbegin(); it != rows.rend() && std::distance(rows.rbegin(), it) < 14; ++it) {
        ImGui::Text("%s", it->day.c_str());
        ImGui::SameLine(120);
        ImGui::ProgressBar(it->completions ? std::min(1.0f, it->completions / 50.0f) : 0.0f,
                           ImVec2(200, 0), std::to_string(it->completions).c_str());
    }
}

- (void)permissionsTab {
    bool ax = Permissions::accessibilityTrusted(false);
    bool sr = Permissions::screenRecordingGranted();

    ImGui::Text("Accessibility: %s", ax ? "Granted" : "Not granted");
    if (!ax) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Open Settings##ax")) Permissions::openAccessibilitySettings();
    }
    ImGui::TextWrapped("Required to read focused text fields and insert completions.");
    ImGui::Separator();

    ImGui::Text("Screen Recording: %s", sr ? "Granted" : "Not granted");
    ImGui::SameLine();
    if (ImGui::SmallButton("Request##sr")) Permissions::requestScreenRecording();
    if (!sr) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Open Settings##sr")) Permissions::openScreenRecordingSettings();
    }
    ImGui::TextWrapped("Optional. Used for screenshot context and overlay placement in hard-to-read apps.");
    ImGui::Separator();
    ImGui::TextWrapped("Tip: disable macOS built-in text suggestions to avoid conflicts.");
}

@end

// ---------------------------------------------------------------------------
// Window controller wrapper.
// ---------------------------------------------------------------------------
@implementation SettingsWindowController {
    __weak AppController* _app;
    NSWindow* _window;
    MTKView* _mtk;
    TroutImGuiRenderer* _renderer;
    bool _imguiInit;
}

- (instancetype)initWithApp:(AppController*)app {
    if ((self = [super init])) { _app = app; _imguiInit = false; }
    return self;
}

- (void)showWindow {
    if (!_window) [self buildWindow];
    [NSApp activateIgnoringOtherApps:YES];
    [_window makeKeyAndOrderFront:nil];
}

- (void)buildWindow {
    NSRect frame = NSMakeRect(0, 0, 720, 520);
    _window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    _window.title = @"Trout Settings";
    [_window center];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    _mtk = [[MTKView alloc] initWithFrame:frame device:device];
    _mtk.enableSetNeedsDisplay = NO;
    _mtk.paused = NO;
    _mtk.preferredFramesPerSecond = 60;
    _mtk.colorPixelFormat = MTLPixelFormatBGRA8Unorm;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplMetal_Init(device);
    ImGui_ImplOSX_Init(_mtk);
    _imguiInit = true;

    _renderer = [[TroutImGuiRenderer alloc] initWithDevice:device app:_app];
    _mtk.delegate = _renderer;

    _window.contentView = _mtk;
    _window.releasedWhenClosed = NO;
}

@end
