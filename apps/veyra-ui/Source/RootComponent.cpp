#include "RootComponent.h"

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
    addChildComponent(settings_);
    addChildComponent(placeholder_);

    home_.attachBackdrop(&background_);
    settings_.attachBackdrop(&background_);
    placeholder_.attachBackdrop(&background_);

    // Seed the working config from the initial control state.
    working_.enhancement = home_.enhancement();
    working_.theme       = themeManager_.currentId().toStdString();

    // Navigation.
    sidebar_.onNavigate = [this](int i) { showScreen(i); };
    sidebar_.onMiniMode = [] { /* mini-mode window: later 4c step */ };

    // Master controls (top bar).
    topBar_.onMasterToggle = [this](bool on) { working_.masterEnabled = on; pushConfig(); };
    topBar_.onMasterVolume = [this](double g) { working_.masterVolumeGain = g; pushConfig(); };

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
    settings_.setPalette(p);
    placeholder_.setPalette(p);
    repaint();
}

void RootComponent::showScreen(int navIndex)
{
    juce::Component* next = nullptr;
    if (navIndex == 0)
        next = &home_;
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
    topBar_.setConnection(st.state == ConnectionState::Connected,
                          juce::String(st.serviceVersion.c_str()));
    if (auto c = client_.config())
        applyConfig(*c);
}

void RootComponent::resized()
{
    auto b = getLocalBounds();
    background_.setBounds(b);
    topBar_.setBounds(b.removeFromTop(56));
    sidebar_.setBounds(b.removeFromLeft(200));

    // All screens share the content rect; only the current one is visible.
    home_.setBounds(b);
    settings_.setBounds(b);
    placeholder_.setBounds(b);
}

} // namespace veyra::ui
