#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

namespace LoopCalibrationCore {
struct Result {double delayMs=0, gainDb=0, correlation=0, outputDbfs=0, inputDbfs=0;int samples=0;};
// Paired output and physical return from one duplex callback. The result is
// electronic round-trip timing, never acoustic propagation distance.
class Capture {
 std::vector<float> output,returned;
 double rate=0;
 bool invalid=false;
public:
 static constexpr int required=65536;
 void reset(){output.clear();returned.clear();rate=0;invalid=false;}
 bool feed(const float* out,const float* in,int count,double sampleRate){
  if(invalid||!out||!in||count<=0||!std::isfinite(sampleRate)||sampleRate<=0)return false;
  if(rate>0&&std::abs(rate-sampleRate)>.01){invalid=true;return false;}rate=sampleRate;
  int amount=std::min(count,required-(int)output.size());
  for(int i=0;i<amount;++i){if(!std::isfinite(out[i])||!std::isfinite(in[i])||std::abs(in[i])>=.999f){invalid=true;return false;}output.push_back(out[i]);returned.push_back(in[i]);}
  return true;
 }
 bool ready()const{return !invalid&&(int)output.size()==required;}
 bool failed()const{return invalid;}
 int collected()const{return (int)output.size();}
 std::optional<Result> calculate()const{
  if(!ready())return {};
  constexpr int stride=8,maxDelay=8192;
  auto correlationAt=[&](int lag,int step){double cross=0,eo=0,ei=0;for(int i=0;i+lag<required;i+=step){double o=output[(size_t)i],r=returned[(size_t)(i+lag)];cross+=o*r;eo+=o*o;ei+=r*r;}return eo>0&&ei>0?cross/std::sqrt(eo*ei):0.;};
  int coarse=0;double best=-1;
  for(int lag=0;lag<=maxDelay;lag+=stride){double score=std::abs(correlationAt(lag,stride));if(score>best){best=score;coarse=lag;}}
  int exact=coarse;best=-1;
  for(int lag=std::max(0,coarse-stride+1);lag<=std::min(maxDelay,coarse+stride-1);++lag){double score=std::abs(correlationAt(lag,1));if(score>best){best=score;exact=lag;}}
  double eo=0,ei=0;int n=0;for(int i=0;i+exact<required;++i){double o=output[(size_t)i],r=returned[(size_t)(i+exact)];eo+=o*o;ei+=r*r;++n;}
  double outRms=std::sqrt(eo/n),inRms=std::sqrt(ei/n);
  if(best<.7||outRms<.0001||inRms<.00001)return {};
  return Result{1000.*exact/rate,20.*std::log10(outRms/inRms),best,20.*std::log10(outRms),20.*std::log10(inRms),n};
 }
};
}
