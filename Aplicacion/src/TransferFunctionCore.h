#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdint>
#include <optional>

namespace TransferFunctionCore {
constexpr int blockCapacity=2048;
constexpr int fftSize=8192;
constexpr int bins=fftSize/2+1;
struct RawBlock {
 std::uint64_t sequence=0;
 double sampleRate=0;
 int count=0;
 std::array<float,blockCapacity> mic{},ref{};
};
template<size_t capacity=32> class Queue {
 std::array<RawBlock,capacity+1> items{};
 std::atomic<size_t> write{0},read{0};
public:
 bool push(const RawBlock& block)noexcept{auto w=write.load(std::memory_order_relaxed),next=(w+1)%(capacity+1);if(next==read.load(std::memory_order_acquire))return false;items[w]=block;write.store(next,std::memory_order_release);return true;}
 bool pop(RawBlock& block)noexcept{auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;block=items[r];read.store((r+1)%(capacity+1),std::memory_order_release);return true;}
};
struct Point {double frequency=0,magnitudeDb=0,phaseDegrees=0,coherence=0;bool valid=false;};
class Estimator {
 using Complex=std::complex<double>;
 std::array<double,fftSize> mic{},ref{};
 std::array<Complex,fftSize> xm{},xr{};
 std::array<Complex,bins> cross{};
 std::array<Complex,bins> transfer{};
 std::array<double,bins> refPower{},micPower{};
 std::array<Point,bins> result{};
 std::array<double,fftSize> impulse{};
 int filled=0,averages=0;
 int averageDepth=8;bool exponential=true;double delayMs=0.;
 double rateHz=0;
 std::uint64_t lastSequence=0;
 bool readyFlag=false;
 static void fft(std::array<Complex,fftSize>& a,bool inverse=false){
  for(int i=1,j=0;i<fftSize;++i){int bit=fftSize>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int len=2;len<=fftSize;len<<=1){double angle=(inverse?1.:-1.)*2.*3.14159265358979323846/len;Complex step{std::cos(angle),std::sin(angle)};for(int start=0;start<fftSize;start+=len){Complex w{1,0};for(int j=0;j<len/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+len/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+len/2)]=u-v;w*=step;}}}if(inverse)for(auto& v:a)v/=fftSize;
 }
 void calculate(){
  double refEnergy=0;for(double value:ref)refEnergy+=value*value;
  // Below -60 dBFS RMS there is no dependable excitation for a live TF.
  if(refEnergy<fftSize*1.e-6){averages=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});return;}
  for(int i=0;i<fftSize;++i){double w=.5-.5*std::cos(2.*3.14159265358979323846*i/(fftSize-1));xm[(size_t)i]={mic[(size_t)i]*w,0};xr[(size_t)i]={ref[(size_t)i]*w,0};}
  fft(xm);fft(xr);
  double alpha=exponential?2./(averageDepth+1.):1./std::min(averageDepth,averages+1);
  for(int k=1;k<bins;++k){auto c=xm[(size_t)k]*std::conj(xr[(size_t)k]);double rp=std::norm(xr[(size_t)k]),mp=std::norm(xm[(size_t)k]);if(averages==0){cross[(size_t)k]=c;refPower[(size_t)k]=rp;micPower[(size_t)k]=mp;}else{cross[(size_t)k]=cross[(size_t)k]*(1-alpha)+c*alpha;refPower[(size_t)k]=refPower[(size_t)k]*(1-alpha)+rp*alpha;micPower[(size_t)k]=micPower[(size_t)k]*(1-alpha)+mp*alpha;}}
  ++averages;readyFlag=averages>=averageDepth;
  if(!readyFlag)return;
  for(int k=1;k<bins;++k){auto& p=result[(size_t)k];p.frequency=k*rateHz/fftSize;p.valid=refPower[(size_t)k]>1.e-12&&micPower[(size_t)k]>1.e-12;if(!p.valid){transfer[(size_t)k]={};continue;}Complex h=cross[(size_t)k]/refPower[(size_t)k];transfer[(size_t)k]=h;p.magnitudeDb=20.*std::log10(std::max(std::abs(h),1.e-12));p.phaseDegrees=std::arg(h)*180./3.14159265358979323846;p.coherence=std::clamp(std::norm(cross[(size_t)k])/(refPower[(size_t)k]*micPower[(size_t)k]),0.,1.);}
  std::array<Complex,fftSize> spectrum{};for(int k=1;k<bins;++k){spectrum[(size_t)k]=transfer[(size_t)k];if(k<fftSize/2)spectrum[(size_t)(fftSize-k)]=std::conj(transfer[(size_t)k]);}fft(spectrum,true);for(int i=0;i<fftSize;++i)impulse[(size_t)i]=spectrum[(size_t)i].real();
 }
public:
 void reset(){filled=averages=0;rateHz=0;lastSequence=0;readyFlag=false;cross.fill({});transfer.fill({});refPower.fill(0);micPower.fill(0);result.fill({});impulse.fill(0);}
 void setAveraging(int depth,bool useExponential=true){depth=std::clamp(depth,1,64);if(depth!=averageDepth||useExponential!=exponential){averageDepth=depth;exponential=useExponential;reset();}}
 int averagingDepth()const{return averageDepth;}bool exponentialAveraging()const{return exponential;}
 void setDelayMs(double value){delayMs=std::clamp(value,-2000.,2000.);}double delayMilliseconds()const{return delayMs;}
 bool ready()const{return readyFlag;}
 int averageCount()const{return averages;}
 double sampleRate()const{return rateHz;}
 void feed(const RawBlock& block){
  if(block.count<=0||block.count>blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;
  if((lastSequence&&block.sequence!=lastSequence+1)||(rateHz>0&&std::abs(block.sampleRate-rateHz)>1.e-6))reset();
  lastSequence=block.sequence;rateHz=block.sampleRate;
  for(int i=0;i<block.count;++i){double m=block.mic[(size_t)i],r=block.ref[(size_t)i];if(!std::isfinite(m)||!std::isfinite(r)){reset();return;}mic[(size_t)filled]=m;ref[(size_t)filled]=r;if(++filled==fftSize){calculate();filled=0;}}
 }
 std::optional<Point> at(double hz)const{if(!readyFlag||hz<=0||rateHz<=0)return {};double index=hz*fftSize/rateHz;int nearest=(int)std::round(index);if(nearest<1||nearest>=bins||!result[(size_t)nearest].valid)return {};auto p=result[(size_t)nearest];p.phaseDegrees=std::remainder(p.phaseDegrees+360.*p.frequency*delayMs*.001,360.);return p;}
 std::optional<Point> smoothedAt(double hz,double octaveFraction)const{
  if(octaveFraction<=0)return at(hz);if(!readyFlag||hz<=0||rateHz<=0)return {};
  double half=std::pow(2.,.5*octaveFraction),lo=hz/half,hi=hz*half;int first=std::max(1,(int)std::ceil(lo*fftSize/rateHz)),last=std::min(bins-1,(int)std::floor(hi*fftSize/rateHz));Complex sum{};double coherence=0,weight=0;
  for(int k=first;k<=last;++k)if(result[(size_t)k].valid){double w=std::max(refPower[(size_t)k],1e-20);sum+=transfer[(size_t)k]*w;coherence+=result[(size_t)k].coherence*w;weight+=w;}if(weight<=0)return {};sum/=weight;Point p;p.frequency=hz;p.valid=true;p.magnitudeDb=20.*std::log10(std::max(std::abs(sum),1e-12));p.phaseDegrees=std::remainder(std::arg(sum)*180./3.14159265358979323846+360.*hz*delayMs*.001,360.);p.coherence=coherence/weight;return p;
 }
 std::optional<double> impulseAt(double timeMs)const{if(!readyFlag||rateHz<=0)return {};long long sample=std::llround((timeMs+delayMs)*.001*rateHz);sample%=fftSize;if(sample<0)sample+=fftSize;return impulse[(size_t)sample];}
};
}
