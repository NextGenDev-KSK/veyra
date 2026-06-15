#pragma once

// Single place to pull in the JUCE modules the UI uses. juce_gui_extra
// transitively includes gui_basics, graphics, events, data_structures and core.
// We do NOT use a generated JuceHeader, so everything is referenced as juce::*.

#include <juce_gui_extra/juce_gui_extra.h>

// Embedded assets (fonts) from juce_add_binary_data(veyra_assets ...).
#include "BinaryData.h"
