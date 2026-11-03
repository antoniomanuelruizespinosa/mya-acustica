#pragma once
#include <cmath>
#include <optional>

// Display coordinates only; no acquisition or DSP state lives here.
namespace GraphCursor {
inline std::optional<double> interpolate(double a,double b,double fraction){
 if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(fraction)||fraction<0.||fraction>1.)return {};
 return a+(b-a)*fraction;
}
inline std::optional<double> interpolateLogFrequency(double a,double b,double fraction){
 if(a<=0.||b<=0.)return {};
 auto value=interpolate(std::log(a),std::log(b),fraction);
 return value?std::optional<double>(std::exp(*value)):std::nullopt;
}
inline double screenY(double normalized,double pan,double zoom){return .5-(normalized-pan)*zoom;}
}
