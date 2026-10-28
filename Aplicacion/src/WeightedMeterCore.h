#pragma once
#include "InputLevelCore.h"
#include <complex>

// Digital A/C approximations from the standard analogue pole frequencies.
// This is not an IEC 61672 type or periodic verification of a complete meter.
namespace WeightedMeter {
struct Stage {
 double b0=1,b1=0,a1=0,x1=0,y1=0;
 void configure(double rate,double hz,bool high){double k=std::tan(3.14159265358979323846*hz/rate);a1=(k-1)/(k+1);b0=high?1/(1+k):k/(1+k);b1=high?-b0:b0;x1=y1=0;}
 double process(double x){double y=b0*x+b1*x1-a1*y1;x1=x;y1=y;return y;}
 std::complex<double> response(double rate,double hz)const{auto z=std::polar(1.,-2*3.14159265358979323846*hz/rate);return (b0+b1*z)/(1.+a1*z);}
};
struct Filter {
 std::array<Stage,6> stages{};int count=0;double gain=1;
 void configure(double rate,bool a){count=a?6:4;int i=0;stages[i++].configure(rate,20.6,true);stages[i++].configure(rate,20.6,true);if(a){stages[i++].configure(rate,107.7,true);stages[i++].configure(rate,737.9,true);}stages[i++].configure(rate,12194.,false);stages[i++].configure(rate,12194.,false);std::complex<double> response{1.,0.};for(int j=0;j<count;++j)response*=stages[j].response(rate,1000.);gain=1./std::abs(response);}
 double process(double x){for(int i=0;i<count;++i)x=stages[i].process(x);return x*gain;}
};
struct Channel {
 double rate=0;Filter a,c;std::array<double,3> fast{},slow{};
 void configure(double sampleRate){rate=sampleRate;fast={};slow={};if(rate>=32000&&rate<=384000){a.configure(rate,true);c.configure(rate,false);}}
 void process(const float* samples,int count,InputLevels::Peak& peak){if(!samples||count<=0||rate<32000||!peak.valid)return;const double alphaFast=std::exp(-1./(rate*.125)),alphaSlow=std::exp(-1./rate);for(int i=0;i<count;++i){double x=samples[i];if(!std::isfinite(x)){peak.weightedValid=false;return;}const double value[3]={x,a.process(x),c.process(x)};for(int w=0;w<3;++w){double p=value[w]*value[w];peak.weightedSquares[(size_t)w]+=p;peak.weightedPeak[(size_t)w]=std::max(peak.weightedPeak[(size_t)w],std::abs(value[w]));fast[(size_t)w]=alphaFast*fast[(size_t)w]+(1-alphaFast)*p;slow[(size_t)w]=alphaSlow*slow[(size_t)w]+(1-alphaSlow)*p;}}peak.fastPower=fast;peak.slowPower=slow;peak.weightedValid=true;}
};
inline std::optional<double> db(double power){if(!std::isfinite(power)||power<0)return {};return power>0?10.*std::log10(power):-INFINITY;}
}
