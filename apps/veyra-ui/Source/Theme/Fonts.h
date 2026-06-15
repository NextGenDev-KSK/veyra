#pragma once

// Embedded typefaces (Orbitron / Inter / JetBrains Mono, all OFL). Provides the
// three roles from the design system: display (uppercase brand), body (UI text),
// and mono (numeric readouts).

#include "VeyraGui.h"

namespace veyra::ui::fonts {

// Load typefaces once (call early, e.g. on app start).
void initialise();

juce::Font display(float height);                 // Orbitron — caller upper-cases
juce::Font body(float height, bool semibold = false);
juce::Font mono(float height, bool medium = false);

} // namespace veyra::ui::fonts
