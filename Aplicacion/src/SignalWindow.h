#pragma once

#include <cmath>

namespace SignalWindow {

enum class Type { Rectangular, Hann, Hamming, Blackman };

inline float coefficient(Type type, int sample, int totalSamples) {
 if (totalSamples <= 1) return 1.0f;
 constexpr double pi = 3.14159265358979323846;
 const double phase = 2.0 * pi * static_cast<double>(sample)
                    / static_cast<double>(totalSamples - 1);
 switch (type) {
  case Type::Rectangular: return 1.0f;
  case Type::Hann: return static_cast<float>(0.5 - 0.5 * std::cos(phase));
  case Type::Hamming: return static_cast<float>(0.54 - 0.46 * std::cos(phase));
  case Type::Blackman: return static_cast<float>(0.42 - 0.5 * std::cos(phase)
                                                   + 0.08 * std::cos(2.0 * phase));
 }
 return 1.0f;
}

} // namespace SignalWindow
