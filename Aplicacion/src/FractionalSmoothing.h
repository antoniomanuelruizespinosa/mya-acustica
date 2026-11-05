#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
#include <limits>

// Display-only fractional-octave integration. Never modifies captured data.
// Magnitude averages linear energy per logarithmic frequency interval; phase
// averages unit phasors so wrapped angles do not cancel at +/-180 degrees.
class FractionalSmoothing {
 static constexpr int count = 32768;
 static constexpr double low = 20., high = 20000., step = (high-low)/count;
 std::vector<double> power, powerIntegral,weights,weightIntegral;
 std::vector<int> invalidCells;
 static std::complex<double> invalid(){return {std::numeric_limits<double>::quiet_NaN(),0.};}
 static bool valid(std::complex<double> v){return std::isfinite(v.real())&&std::isfinite(v.imag())&&std::abs(v)>1.e-15;}
 std::vector<std::complex<double>> values,phaseValues,phaseIntegral;
 template<class T> T integralAt(double frequency, const std::vector<T>& samples,
                                const std::vector<T>& integral) const {
  double position = (std::clamp(frequency,low,high)-low)/step;
  auto i = static_cast<size_t>(std::min(count-1,static_cast<int>(position)));
  double t = position-static_cast<double>(i);
  return integral[i]+step*(samples[i]*t+(samples[i+1]-samples[i])*(t*t*.5));
 }
public:
 template<class Sample> void rebuild(Sample sample) {
  power.resize(count+1);weights.resize(count+1);values.resize(count+1);phaseValues.resize(count+1);invalidCells.assign(count+1,0);
  powerIntegral.assign(count+1,0.);weightIntegral.assign(count+1,0.);phaseIntegral.assign(count+1,{});
  for(size_t i=0;i<=static_cast<size_t>(count);++i){
   double frequency=low+static_cast<double>(i)*step;values[i]=sample(frequency);
   double weight=1./frequency,magnitude=std::abs(values[i]);weights[i]=weight;
   power[i]=valid(values[i])?magnitude*magnitude*weight:0.;
   phaseValues[i]=valid(values[i])?values[i]/magnitude*weight:std::complex<double>{};
   if(i){invalidCells[i]=invalidCells[i-1]+int(!valid(values[i-1])||!valid(values[i]));
    powerIntegral[i]=powerIntegral[i-1]+step*.5*(power[i-1]+power[i]);
    weightIntegral[i]=weightIntegral[i-1]+step*.5*(weights[i-1]+weights[i]);
    phaseIntegral[i]=phaseIntegral[i-1]+step*.5*(phaseValues[i-1]+phaseValues[i]);}
  }
 }
 std::complex<double> at(double frequency,double divisions,bool phase) const {
  if(values.empty()||!std::isfinite(frequency)||!std::isfinite(divisions)||frequency<low||frequency>high||divisions<=0)return invalid();
  double ratio=std::pow(2.,.5/divisions);
  double a=std::max(low,frequency/ratio),b=std::min(high,frequency*ratio);
  if(!(b>a))return invalid();
  // Any invalid cell invalidates its display band; do not bridge gaps or invent zeros.
  auto first=static_cast<size_t>(std::clamp(int(std::floor((a-low)/step)),0,count-1));
  auto last=static_cast<size_t>(std::clamp(int(std::ceil((b-low)/step)),1,count));
  if(invalidCells[last]!=invalidCells[first])return invalid();
  double weight=integralAt(b,weights,weightIntegral)-integralAt(a,weights,weightIntegral);
  if(weight<=0.)return invalid();
  if(phase)return (integralAt(b,phaseValues,phaseIntegral)-integralAt(a,phaseValues,phaseIntegral))/weight;
  double mean=(integralAt(b,power,powerIntegral)-integralAt(a,power,powerIntegral))/weight;
  return {std::sqrt(std::max(0.,mean)),0.};
 }
};
