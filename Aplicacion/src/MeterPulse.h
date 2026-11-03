#pragma once
#include <cmath>
#include <optional>

// Presentation-only alert for a valid digital peak. SPL values are not dBFS.
namespace MeterPulse {
enum class Level { None, Orange, Red };
inline Level classify(std::optional<double> dbfs, bool clipped = false) {
 if (clipped) return Level::Red;
 if (!dbfs || !std::isfinite(*dbfs)) return Level::None;
 return *dbfs >= 0.0 ? Level::Red : *dbfs >= -12.0 ? Level::Orange : Level::None;
}
inline float opacity(double milliseconds) {
 return static_cast<float>(0.52 + 0.28 * std::sin(milliseconds * 0.009));
}
}
