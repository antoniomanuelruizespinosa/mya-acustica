#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
#include <limits>

// Display-only fractional-octave integration. Never modifies captured data.
// Magnitude averages dB; phase averages complex values, avoiding phase-wrap errors.
class FractionalSmoothing {
 static constexpr int count = 32768;
 static constexpr double low = 20., high = 20000., step = (high-low)/count;
 std::vector<double> db, dbIntegral;
 std::vector<int> invalidCells;
 static std::complex<double> invalid(){return {std::numeric_limits<double>::quiet_NaN(),0.};}
 static bool valid(std::complex<double> v){return std::isfinite(v.real())&&std::isfinite(v.imag())&&std::abs(v)>1.e-15;}
 std::vector<std::complex<double>> values, complexIntegral;
 template<class T> T integralAt(double frequency, const std::vector<T>& samples,
                                const std::vector<T>& integral) const {
  double position = (std::clamp(frequency,low,high)-low)/step;
  auto i = static_cast<size_t>(std::min(count-1,static_cast<int>(position)));
  double t = position-static_cast<double>(i);
  return integral[i]+step*(samples[i]*t+(samples[i+1]-samples[i])*(t*t*.5));
 }
public:
 template<class Sample> void rebuild(Sample sample) {
  db.resize(count+1); values.resize(count+1);invalidCells.assign(count+1,0);
  dbIntegral.assign(count+1,0.); complexIntegral.assign(count+1,{});
  for(size_t i=0;i<=static_cast<size_t>(count);++i){
   values[i]=sample(low+static_cast<double>(i)*step);
   db[i]=valid(values[i])?20.*std::log10(std::abs(values[i])):0.;
   if(i){invalidCells[i]=invalidCells[i-1]+int(!valid(values[i-1])||!valid(values[i]));
    dbIntegral[i]=dbIntegral[i-1]+step*.5*(db[i-1]+db[i]);
    complexIntegral[i]=complexIntegral[i-1]+step*.5*((valid(values[i-1])?values[i-1]:std::complex<double>{})+(valid(values[i])?values[i]:std::complex<double>{}));}
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
  if(phase)return (integralAt(b,values,complexIntegral)-integralAt(a,values,complexIntegral))/(b-a);
  double mean=(integralAt(b,db,dbIntegral)-integralAt(a,db,dbIntegral))/(b-a);
  return {std::pow(10.,mean/20.),0.};
 }
};
