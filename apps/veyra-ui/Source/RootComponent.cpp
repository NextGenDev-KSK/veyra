#include "RootComponent.h"

#include "veyra/Preset.h"

namespace veyra::ui {

namespace {
const char* kScreenNames[] = {"Home", "Presets", "Apps", "Devices",
                              "Sound Lab", "Gamer Mode", "Settings"};
} // namespace

RootComponent::RootComponent()
{
    setLookAndFeel(&laf_);
    themeManager_.addChangeListener(this);

    addAndMakeVisible(background_);
    addAndMakeVisible(topBar_);
    addAndMakeVisible(sidebar_);
    addChildComponent(home_);
    addChildComponent(presets_);
    addChildComponent(settings_);
    addChildComponent(placeholder_);

    home_.attachBackdrop(&background_);
    presets_.attachBackdrop(&background_);
    settings_.attachBackdrop(&background_);
    placeholder_.attachBackdrop(&background_);

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
    settings_.onReduceMotion   = [this](bool b) { home_.setReduceMotion(b); };
    settings_.onOpacity        = [](double) { /* deep opacity wiring lands with persisted appearance config */ };
    settings_.onBackgroundMode = [](int) { /* ditto */ };

    applyPalette();
    settings_.setCurrentTheme(themeManager_.currentId());

    current_ = &home_;
    home_.setVisible(true);
    sidebar_.setActive(0);

    setSize(1180, 760);

    // Connect to the service; notifications hop to the message thread.
    client_.start([safe = juce::Component::SafePointer<RootComponent>(this)]
    {
        juce::MessageManager::callAsync([safe]
        {
            if (safe != nullptr)
                safe->refreshFromService();
        });
    });
}

RootComponent::~RootComponent()
{
    client_.stop(); // join the background thread before the rest tears down
    themeManager_.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void RootComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    applyPalette();
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
    settings_.setPalette(p);
    placeholder_.setPalette(p);
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
    if (auto c = client_.config())
        applyConfig(*c);
    presets_.setPresets(client_.presets(), juce::String(working_.activePresetUuid));
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

    // All screens share the content rect; only the current one is visible.
    home_.setBounds(b);
    presets_.setBounds(b);
    settings_.setBounds(b);
    placeholder_.setBounds(b);
}

} // namespace veyra::ui
