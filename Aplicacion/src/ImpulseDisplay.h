#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <optional>
#include <vector>

// Display of a synthetic IR only. No deconvolution and no conversion of TF into an IR.
class ImpulseDisplay {
 std::vector<double> signal,analyticMagnitude;
 static void fft(std::vector<std::complex<double>>& a,bool inverse){
  size_t n=a.size();for(size_t i=1,j=0;i<n;++i){size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
  for(size_t length=2;length<=n;length<<=1){auto step=std::polar(1.,(inverse?2.:-2.)*std::acos(-1.)/(double)length);for(size_t i=0;i<n;i+=length){std::complex<double> w=1.;for(size_t j=0;j<length/2;++j){auto even=a[i+j],odd=a[i+j+length/2]*w;a[i+j]=even+odd;a[i+j+length/2]=even-odd;w*=step;}}}
  if(inverse)for(auto& value:a)value/=(double)n;
 }
public:
 static constexpr double spanMs=80.,floorDb=-120.;
 static double timeMs(double u){return (u-.5)*spanMs;}
 static std::vector<double> envelope(const std::vector<double>& input){
  size_t n=input.size();if(n<2||(n&(n-1))!=0)return {};
  std::vector<std::complex<double>> analytic;analytic.reserve(n);for(double value:input){if(!std::isfinite(value))return {};analytic.emplace_back(value,0.);}
  fft(analytic,false);for(size_t i=1;i<n/2;++i)analytic[i]*=2.;for(size_t i=n/2+1;i<n;++i)analytic[i]=0.;fft(analytic,true);
  std::vector<double> out;out.reserve(n);for(const auto& value:analytic)out.push_back(std::abs(value));return out;
 }
 ImpulseDisplay(){constexpr int count=4096;signal.reserve(count);for(int i=0;i<count;++i){double u=i/(double)count;signal.push_back(std::exp(-std::pow((u-.5)*65,2))*std::cos((u-.5)*240)*.42);}analyticMagnitude=envelope(signal);}
 // mode 0: signed Lin, 1: log amplitude ref 1, 2: ETC from the analytic envelope.
 std::optional<double> physical(double u,int mode) const{
  if(!std::isfinite(u)||u<0||u>1||mode<0||mode>2)return std::nullopt;
  const auto& values=mode==2?analyticMagnitude:signal;if(values.empty())return std::nullopt;
  double pos=std::min(u*(double)values.size(),(double)(values.size()-1));auto i=(size_t)pos;auto next=std::min(i+1,values.size()-1);double value=values[i]+(values[next]-values[i])*(pos-(double)i);
  if(mode==0)return value;return std::max(floorDb,20*std::log10(std::max(1.e-6,std::abs(value))));
 }
 std::optional<double> normalized(double u,int mode) const{auto value=physical(u,mode);if(!value)return std::nullopt;return mode==0?*value/2.:(*value+60.)/120.;}
};
