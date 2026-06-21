#include "RootComponent.h"

#include "veyra/CrashReport.h"
#include "veyra/Paths.h"
#include "veyra/Preset.h"

namespace veyra::ui {

namespace {
const char* kScreenNames[] = {"Home", "Presets", "Apps", "Devices",
                              "Sound Lab", "Gamer Mode", "Settings"};
} // namespace

RootComponent::RootComponent()
{
    setLookAndFeel(&laf_);
    setOpaque(false); // let the window's acrylic backdrop show through the glass
    themeManager_.addChangeListener(this);

    addAndMakeVisible(background_);
    addAndMakeVisible(topBar_);
    addAndMakeVisible(sidebar_);
    addChildComponent(home_);
    addChildComponent(presets_);
    addChildComponent(apps_);
    addChildComponent(settings_);
    addChildComponent(effects_);
    addChildComponent(devices_);
    addChildComponent(soundLab_);
    addChildComponent(gamer_);
    addChildComponent(placeholder_);

    home_.attachBackdrop(&background_);
    presets_.attachBackdrop(&background_);
    apps_.attachBackdrop(&background_);
    settings_.attachBackdrop(&background_);
    effects_.attachBackdrop(&background_);
    devices_.attachBackdrop(&background_);
    soundLab_.attachBackdrop(&background_);
    gamer_.attachBackdrop(&background_);
    placeholder_.attachBackdrop(&background_);

    devices_.onBridgeChanged = [this](const veyra::BridgeConfig& b) { working_.bridge = b; pushConfig(); };
    devices_.setBridge(working_.bridge);

    // Apps: master per-app-switching toggle.
    apps_.onSwitchingChanged = [this](bool on) { working_.appSwitching = on; pushConfig(); };
    apps_.setSwitchingEnabled(working_.appSwitching);

    // Gamer Mode dashboard -> the gamer / spatial / voice / loudness blocks.
    gamer_.onGamerChanged    = [this](const veyra::GamerModeConfig& g) { working_.gamerMode = g; pushConfig(); };
    gamer_.onSpatialChanged  = [this](const veyra::SpatialConfig& s)   { working_.spatial = s;   settings_.setSpatialConfig(s); pushConfig(); };
    gamer_.onVoiceChanged    = [this](const veyra::VoiceConfig& v)     { working_.voice = v;     settings_.setMicConfig(v); pushConfig(); };
    gamer_.onLoudnessChanged = [this](const veyra::LoudnessConfig& l)  { working_.loudness = l;  settings_.setLoudnessConfig(l); pushConfig(); };
    gamer_.onTestInSoundLab  = [this] { sidebar_.setActive(4); showScreen(4); };
    gamer_.setGamer(working_.gamerMode);
    gamer_.setSpatial(working_.spatial);
    gamer_.setVoice(working_.voice);
    gamer_.setLoudness(working_.loudness);

    // Crash-recovery banner: surface a previous session's crash report (§15).
    addChildComponent(crashBanner_);
    crashBanner_.onDismiss = [this] { crashBanner_.setVisible(false); resized(); };
    crashBanner_.onOpenFolder = [this]
    {
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Veyra").getChildFile("crashes").revealToUser();
    };
    if (auto crash = veyra::latestCrashReport(veyra::paths::crashesDir()))
    {
        crashBanner_.setMessage("Veyra recovered from a crash on "
                                + juce::String(crash->timestamp) + ". Your settings are safe.");
        crashBanner_.setVisible(true);
    }

    // First-run onboarding overlay (on top; shown until finished/skipped).
    addAndMakeVisible(onboarding_);
    onboarding_.onFinished = [this]
    {
        working_.onboardingComplete = true;
        pushConfig();
        onboarding_.setVisible(false);
    };
    onboarding_.setVisible(!working_.onboardingComplete);

    // Home "More Effects" tile -> effects rack overview; back returns Home.
    home_.onMoreEffects = [this]
    {
        effects_.setConfig(working_);
        if (current_ != nullptr) current_->setVisible(false);
        current_ = &effects_;
        effects_.setVisible(true);
        resized();
    };
    effects_.onBack = [this] { sidebar_.setActive(0); showScreen(0); };

    // Per-app rules: auto-apply a preset when the foreground app matches a rule.
    appRules_.setRulesFile(veyra::paths::appDataDir() / "app_rules.json");

    // Presets screen actions.
    presets_.onApply       = [this](juce::String uuid) { client_.loadPreset(uuid.toStdString()); };
    presets_.onDelete      = [this](juce::String uuid) { client_.deletePreset(uuid.toStdString()); };
    presets_.onSaveCurrent = [this](juce::String name) { saveCurrentAsPreset(name); };
    presets_.onExport      = [this](juce::String uuid) { exportPreset(uuid); };
    presets_.onImport      = [this] { importPreset(); };

    // Seed the working config from the initial control state.
    working_.enhancement = home_.enhancement();
    working_.theme       = themeManager_.currentId().toStdString();

    // Mini-mode window (hidden until invoked) sharing this shell's config/client.
    mini_ = std::make_unique<MiniWindow>();
    auto& mc = mini_->content();
    mc.setPalette(themeManager_.palette());
    mc.onMasterToggle = [this](bool on) { setMasterEnabled(on); };
    mc.onMasterVolume = [this](double g) { setMasterVolume(g); };
    mc.onExpand       = [this] { enterFullMode(); };
    mc.onClose        = [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };

    // System tray.
    tray_.onOpen          = [this] { enterFullMode(); };
    tray_.onMini          = [this] { enterMiniMode(); };
    tray_.onQuit          = [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };
    tray_.onToggleMaster  = [this](bool on) { setMasterEnabled(on); };
    tray_.isMasterEnabled = [this] { return working_.masterEnabled; };

    // Navigation.
    sidebar_.onNavigate = [this](int i) { showScreen(i); };
    sidebar_.onMiniMode = [this] { enterMiniMode(); };

    // Master controls (top bar).
    topBar_.onMasterToggle = [this](bool on) { setMasterEnabled(on); };
    topBar_.onMasterVolume = [this](double g) { setMasterVolume(g); };
    topBar_.onOpenSettings = [this] { sidebar_.setActive(6); showScreen(6); };

    // Enhancement params (home knobs + EQ).
    home_.onEnhancementChanged = [this](const EnhancementConfig& e)
    {
        working_.enhancement = e;
        pushConfig();
    };

    // Appearance (settings).
    settings_.onThemeSelected = [this](const juce::String& id)
    {
        themeManager_.setTheme(id); // -> changeListenerCallback -> applyPalette
        working_.theme = id.toStdString();
        pushConfig();
    };
    settings_.onReduceMotion   = [this](bool b)
    {
        working_.reduceMotion = b;
        home_.setReduceMotion(b);
        pushConfig();
    };
    settings_.onOpacity        = [this](double v)
    {
        working_.uiOpacity = v;
        background_.setOpacity((float) v);
        pushConfig();
    };
    settings_.onBackgroundMode = [this](int m)
    {
        working_.backgroundMode = m;
        background_.setBackgroundMode(m);
        pushConfig();
    };
    settings_.onMicChanged     = [this](const veyra::VoiceConfig& v) { working_.voice = v; pushConfig(); };
    settings_.onSpatialChanged = [this](const veyra::SpatialConfig& s) { working_.spatial = s; pushConfig(); };
    settings_.onLoudnessChanged = [this](const veyra::LoudnessConfig& l) { working_.loudness = l; pushConfig(); };
    settings_.onAudioEngineChanged = [this](const veyra::AudioEngineConfig& e) { working_.audioEngine = e; pushConfig(); };
    settings_.onResetSettings  = [this]
    {
        applyConfig(veyra::Config{}); // restore all defaults across the UI
        pushConfig();                 // persist the reset
    };

    applyPalette();
    settings_.setCurrentTheme(themeManager_.currentId());
    settings_.setAppearance(working_.uiOpacity, working_.backgroundMode, working_.reduceMotion);
    settings_.setMicConfig(working_.voice);
    settings_.setSpatialConfig(working_.spatial);
    settings_.setLoudnessConfig(working_.loudness);
    settings_.setAudioEngineConfig(working_.audioEngine);

    // Apply the initial appearance to the live background.
    background_.setOpacity((float) working_.uiOpacity);
    background_.setBackgroundMode(working_.backgroundMode);
    home_.setReduceMotion(working_.reduceMotion);

    current_ = &home_;
    home_.setVisible(true);
    sidebar_.setActive(0);

    setSize(1200, 675);

    // Connect to the service; notifications hop to the message thread.
    client_.start([safe = juce::Component::SafePointer<RootComponent>(this)]
    {
        juce::MessageManager::callAsync([safe]
        {
            if (safe != nullptr)
                safe->refreshFromService();
        });
    });

    startTimerHz(30); // poll the service's live metering block for the visualizer

    // Global hotkeys (master/volume/preset/mini). The callback hops to the
    // message thread inside HotkeyManager; the SafePointer guards teardown.
    hotkeys_.start(working_.hotkeys,
                   [safe = juce::Component::SafePointer<RootComponent>(this)](veyra::HotkeyAction a)
                   {
                       if (safe != nullptr)
                           safe->handleHotkey(a);
                   });
}

RootComponent::~RootComponent()
{
    stopTimer();
    hotkeys_.stop();
    client_.stop(); // join the background thread before the rest tears down
    themeManager_.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void RootComponent::handleHotkey(veyra::HotkeyAction action)
{
    switch (action)
    {
    case veyra::HotkeyAction::ToggleMaster: setMasterEnabled(!working_.masterEnabled); break;
    case veyra::HotkeyAction::VolumeUp:     setMasterVolume(juce::jmin(1.5, working_.masterVolumeGain + 0.05)); break;
    case veyra::HotkeyAction::VolumeDown:   setMasterVolume(juce::jmax(0.0, working_.masterVolumeGain - 0.05)); break;
    case veyra::HotkeyAction::NextPreset:   cyclePreset(+1); break;
    case veyra::HotkeyAction::PrevPreset:   cyclePreset(-1); break;
    case veyra::HotkeyAction::ToggleMini:
        if (mini_ != nullptr && mini_->isVisible()) enterFullMode();
        else                                        enterMiniMode();
        break;
    default: break;
    }
}

void RootComponent::cyclePreset(int direction)
{
    const auto presets = client_.presets();
    if (presets.empty())
        return;
    int idx = -1;
    for (int i = 0; i < (int) presets.size(); ++i)
        if (presets[(size_t) i].uuid == working_.activePresetUuid)
            idx = i;
    const int n = (int) presets.size();
    const int next = (idx < 0) ? 0 : ((idx + direction) % n + n) % n;
    client_.loadPreset(presets[(size_t) next].uuid);
}

void RootComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    applyPalette();
}

void RootComponent::timerCallback()
{
    // ~1 Hz: apply a per-app rule when the foreground app changes (timer is 30 Hz).
    if (++ruleTick_ >= 30)
    {
        ruleTick_ = 0;
        if (working_.appSwitching)
            if (auto uuid = appRules_.poll())
                client_.loadPreset(*uuid);
    }

    // Open the analyzer block lazily (the service may start after the UI).
    if (analyzerData_ == nullptr)
    {
        if (analyzerRegion_.open(veyra::ipc::kSharedAnalyzerName, sizeof(veyra::ipc::VeyraAnalyzerData)))
            analyzerData_ = static_cast<const veyra::ipc::VeyraAnalyzerData*>(analyzerRegion_.data());
        else
            return;
    }

    veyra::ipc::VeyraAnalyzerPayload p;
    if (veyra::ipc::readAnalyzer(analyzerData_, p))
        home_.pushVisualizerFrame(p.bars, veyra::ipc::kAnalyzerBars,
                                  p.vuL, p.vuR, p.peakL, p.peakR, p.clip != 0);
}

void RootComponent::applyPalette()
{
    const auto& p = themeManager_.palette();
    laf_.setPalette(p);
    background_.setPalette(p);
    topBar_.setPalette(p);
    sidebar_.setPalette(p);
    home_.setPalette(p);
    presets_.setPalette(p);
    apps_.setPalette(p);
    settings_.setPalette(p);
    effects_.setPalette(p);
    devices_.setPalette(p);
    soundLab_.setPalette(p);
    gamer_.setPalette(p);
    crashBanner_.setPalette(p);
    placeholder_.setPalette(p);
    onboarding_.setPalette(p);
    if (mini_ != nullptr)
        mini_->content().setPalette(p);
    tray_.updateIcon(p);
    repaint();
}

void RootComponent::showScreen(int navIndex)
{
    juce::Component* next = nullptr;
    if (navIndex == 0)
        next = &home_;
    else if (navIndex == 1)
        next = &presets_;
    else if (navIndex == 2)
        next = &apps_;
    else if (navIndex == 3)
        next = &devices_;
    else if (navIndex == 4)
        next = &soundLab_;
    else if (navIndex == 5)
        next = &gamer_;
    else if (navIndex == 6)
        next = &settings_;
    else
    {
        placeholder_.setTitle(kScreenNames[juce::jlimit(0, 6, navIndex)]);
        next = &placeholder_;
    }

    if (next == current_)
        return;
    if (current_ != nullptr)
        current_->setVisible(false);
    current_ = next;
    current_->setVisible(true);
    resized();
}

void RootComponent::applyConfig(const veyra::Config& c)
{
    working_ = c;
    topBar_.setMasterEnabled(c.masterEnabled);
    topBar_.setMasterVolume(c.masterVolumeGain);
    mini_->content().setMasterEnabled(c.masterEnabled);
    mini_->content().setMasterVolume(c.masterVolumeGain);
    home_.applyEnhancement(c.enhancement);
    settings_.setMicConfig(c.voice);
    settings_.setSpatialConfig(c.spatial);
    settings_.setLoudnessConfig(c.loudness);
    settings_.setAudioEngineConfig(c.audioEngine);
    apps_.setSwitchingEnabled(c.appSwitching);
    gamer_.setGamer(c.gamerMode);
    gamer_.setSpatial(c.spatial);
    gamer_.setVoice(c.voice);
    gamer_.setLoudness(c.loudness);
    if (c.onboardingComplete)
        onboarding_.setVisible(false); // a saved profile already finished onboarding
    devices_.setBridge(c.bridge);
    effects_.setConfig(c);

    // Appearance: opacity / background mode / reduce-motion.
    settings_.setAppearance(c.uiOpacity, c.backgroundMode, c.reduceMotion);
    background_.setOpacity((float) c.uiOpacity);
    background_.setBackgroundMode(c.backgroundMode);
    home_.setReduceMotion(c.reduceMotion);

    const juce::String themeId(c.theme);
    if (themeId.isNotEmpty() && themeId != themeManager_.currentId())
    {
        themeManager_.setTheme(themeId); // -> applyPalette via listener
        settings_.setCurrentTheme(themeId);
    }
}

void RootComponent::pushConfig()
{
    client_.updateConfig(working_);
}

void RootComponent::refreshFromService()
{
    const auto st = client_.status();
    const bool connected = st.state == ConnectionState::Connected;
    topBar_.setConnection(connected, juce::String(st.serviceVersion.c_str()));
    mini_->content().setConnection(connected);
    settings_.setServiceStatus(connected, juce::String(st.serviceVersion.c_str()));
    if (auto c = client_.config())
        applyConfig(*c);
    presets_.setPresets(client_.presets(), juce::String(working_.activePresetUuid));
    apps_.setPresets(client_.presets());

    // Reflect the active preset name in the mini widget + the top-bar chip.
    juce::String presetName = "Custom";
    for (const auto& p : client_.presets())
        if (p.uuid == working_.activePresetUuid)
            presetName = juce::String(p.name);
    mini_->content().setPreset(presetName);
    topBar_.setActivePreset(presetName);
}

void RootComponent::saveCurrentAsPreset(const juce::String& name)
{
    veyra::Preset p;
    p.uuid             = ("u-" + juce::Uuid().toDashedString()).toStdString();
    p.name             = name.toStdString();
    p.category         = "Custom";
    p.author           = "User";
    p.builtIn          = false;
    p.masterVolumeGain = working_.masterVolumeGain;
    p.enhancement      = working_.enhancement;
    client_.savePreset(p);
}

void RootComponent::exportPreset(const juce::String& uuid)
{
    veyra::Preset found;
    bool ok = false;
    for (const auto& p : client_.presets())
        if (juce::String(p.uuid) == uuid) { found = p; ok = true; break; }
    if (!ok)
        return;

    chooser_ = std::make_unique<juce::FileChooser>(
        "Export Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                             .getChildFile(juce::String(found.name) + ".vpreset"),
        "*.vpreset");
    const auto fbFlags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;
    chooser_->launchAsync(fbFlags, [found](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file != juce::File())
            file.replaceWithText(juce::String(found.toJson()));
    });
}

void RootComponent::importPreset()
{
    chooser_ = std::make_unique<juce::FileChooser>(
        "Import Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.vpreset");
    const auto fbFlags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;
    chooser_->launchAsync(fbFlags, [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;
        if (auto p = veyra::Preset::fromJson(file.loadFileAsString().toStdString()))
        {
            // Import as a fresh user preset so it can't clash with a built-in id.
            p->uuid    = ("u-" + juce::Uuid().toDashedString()).toStdString();
            p->builtIn = false;
            client_.savePreset(*p);
        }
    });
}

void RootComponent::setMasterEnabled(bool on)
{
    working_.masterEnabled = on;
    topBar_.setMasterEnabled(on);
    mini_->content().setMasterEnabled(on);
    pushConfig();
}

void RootComponent::setMasterVolume(double gain)
{
    working_.masterVolumeGain = gain;
    topBar_.setMasterVolume(gain);
    mini_->content().setMasterVolume(gain);
    pushConfig();
}

void RootComponent::enterMiniMode()
{
    if (auto* w = getTopLevelComponent())
        w->setVisible(false);

    // Park the bar at the top-centre of the primary display.
    if (auto* disp = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto area = disp->userArea;
        mini_->setTopLeftPosition(area.getCentreX() - mini_->getWidth() / 2, area.getY() + 24);
    }
    mini_->setVisible(true);
    mini_->toFront(true);
}

void RootComponent::enterFullMode()
{
    mini_->setVisible(false);
    if (auto* w = getTopLevelComponent())
    {
        w->setVisible(true);
        w->toFront(true);
    }
}

void RootComponent::resized()
{
    auto b = getLocalBounds();
    background_.setBounds(b);
    topBar_.setBounds(b.removeFromTop(56));
    sidebar_.setBounds(b.removeFromLeft(200));

    // Crash-recovery banner sits atop the content area when present.
    if (crashBanner_.isVisible())
    {
        crashBanner_.setBounds(b.removeFromTop(48).reduced(24, 6));
        b.removeFromTop(4);
    }

    // All screens share the content rect; only the current one is visible.
    home_.setBounds(b);
    presets_.setBounds(b);
    apps_.setBounds(b);
    settings_.setBounds(b);
    effects_.setBounds(b);
    devices_.setBounds(b);
    soundLab_.setBounds(b);
    gamer_.setBounds(b);
    placeholder_.setBounds(b);
    onboarding_.setBounds(getLocalBounds());
    onboarding_.toFront(false);
}

} // namespace veyra::ui
