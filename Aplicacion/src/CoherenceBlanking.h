#pragma once
#include <algorithm>
#include <cmath>

namespace CoherenceBlanking {
inline double displayed(double raw,bool squared){
 if(!std::isfinite(raw))return 0.;
 raw=std::clamp(raw,0.,1.);
 return squared?raw:std::sqrt(raw);
}
inline bool passes(double raw,double thresholdPercent,bool squared){
 return thresholdPercent<=0.||displayed(raw,squared)*100.>=thresholdPercent;
}
}
