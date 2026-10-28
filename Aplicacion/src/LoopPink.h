#pragma once
#include <algorithm>
#include <cstdint>

// Base pink sample is limited to -40 dBFS peak; engine trim caps it at -20 dBFS peak.
// No input signal is ever returned to the output.
class LoopPink {
 std::uint32_t seed=0x7182ac41u;
 float b0=0.f,b1=0.f,b2=0.f;
public:
 static constexpr float maximum=0.01f;
 float next() noexcept {
  seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
  const float white=(float)((seed>>8)&0x00ffffffu)*(2.f/16777215.f)-1.f;
  b0=.99765f*b0+white*.0990460f;
  b1=.96300f*b1+white*.2965164f;
  b2=.57000f*b2+white*1.0526913f;
  return std::clamp((b0+b1+b2+white*.1848f)*.002f,-maximum,maximum);
 }
};
