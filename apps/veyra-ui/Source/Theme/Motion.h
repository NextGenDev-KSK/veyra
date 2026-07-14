#pragma once

// App-wide motion tokens. Three durations — fast (state feedback), med
// (movement), slow (entrances) — and all of them return 0 while Reduce Motion
// is on, so animations collapse to instant state changes. Audio meters are
// never animated regardless (they repaint from live data at refresh rate).

#include "veyra/ThemeTokens.h"

#include <atomic>

namespace veyra::ui::motion {

inline std::atomic<bool>& reducedFlag()
{
    static std::atomic<bool> f{false};
    return f;
}

inline void setReducedMotion(bool on) { reducedFlag().store(on); }
inline bool reducedMotion() { return reducedFlag().load(); }

inline int fastMs() { return veyra::theme::motion::fastMs(reducedMotion()); }
inline int medMs()  { return veyra::theme::motion::medMs(reducedMotion()); }
inline int slowMs() { return veyra::theme::motion::slowMs(reducedMotion()); }

} // namespace veyra::ui::motion
