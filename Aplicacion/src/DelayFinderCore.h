#pragma once
#include "DspMath.h"
#include "TransferFunctionCore.h"
#include <optional>
#include <vector>

namespace DelayFinderCore {
struct Result {double milliseconds=0.,samples=0.,confidence=0.;bool valid=false;};
class Finder {
 std::vector<double> mic,ref;double rate=0.;double captureSeconds=1.;size_t wanted=65536;
public:
 void configure(double seconds){captureSeconds=std::clamp(seconds,0.1,4.0);reset();}
 void reset(){mic.clear();ref.clear();rate=0.;}
 bool ready()const{return rate>0&&mic.size()>=wanted&&ref.size()>=wanted;}
 void feed(const TransferFunctionCore::RawBlock& b){if(b.count<=0||b.sampleRate<=0)return;if(rate>0&&std::abs(rate-b.sampleRate)>.1)reset();rate=b.sampleRate;wanted=(size_t)std::llround(captureSeconds*rate);for(int i=0;i<b.count&&mic.size()<wanted;++i){mic.push_back(b.mic[(size_t)i]);ref.push_back(b.ref[(size_t)i]);}}
 Result calculate(double maxDelayMs=2000.)const{
  Result out;if(!ready())return out;int n=MyaDSP::nextPowerOfTwo((int)mic.size()*2);std::vector<MyaDSP::Complex> a((size_t)n),b((size_t)n);
  double em=0,er=0;for(size_t i=0;i<mic.size();++i){a[i]=mic[i];b[i]=ref[i];em+=mic[i]*mic[i];er+=ref[i]*ref[i];}if(em<1e-10||er<1e-10)return out;
  MyaDSP::fft(a);MyaDSP::fft(b);for(int i=0;i<n;++i){auto v=a[(size_t)i]*std::conj(b[(size_t)i]);double mag=std::abs(v);a[(size_t)i]=mag>1e-15?v/mag:MyaDSP::Complex{};}MyaDSP::fft(a,true);
  int limit=std::min((int)std::llround(maxDelayMs*.001*rate),n/2-1),best=0;double peak=0,second=0;
  for(int lag=-limit;lag<=limit;++lag){double value=std::abs(a[(size_t)(lag>=0?lag:n+lag)]);if(value>peak){second=peak;peak=value;best=lag;}else if(value>second&&std::abs(lag-best)>2)second=value;}
  out.samples=best;out.milliseconds=1000.*best/rate;out.confidence=peak>0?std::clamp((peak-second)/peak,0.,1.):0.;out.valid=peak>1e-6&&out.confidence>.02;return out;
 }
};
}
