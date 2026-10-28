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
 std::array<double,bins> refPower{},micPower{};
 std::array<Point,bins> result{};
 int filled=0,averages=0;
 double rateHz=0;
 std::uint64_t lastSequence=0;
 bool readyFlag=false;
 static void fft(std::array<Complex,fftSize>& a){
  for(int i=1,j=0;i<fftSize;++i){int bit=fftSize>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int len=2;len<=fftSize;len<<=1){double angle=-2.*3.14159265358979323846/len;Complex step{std::cos(angle),std::sin(angle)};for(int start=0;start<fftSize;start+=len){Complex w{1,0};for(int j=0;j<len/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+len/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+len/2)]=u-v;w*=step;}}}
 }
 void calculate(){
  double refEnergy=0;for(double value:ref)refEnergy+=value*value;
  // Below -60 dBFS RMS there is no dependable excitation for a live TF.
  if(refEnergy<fftSize*1.e-6){averages=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});return;}
  for(int i=0;i<fftSize;++i){double w=.5-.5*std::cos(2.*3.14159265358979323846*i/(fftSize-1));xm[(size_t)i]={mic[(size_t)i]*w,0};xr[(size_t)i]={ref[(size_t)i]*w,0};}
  fft(xm);fft(xr);
  constexpr double alpha=.15;
  for(int k=1;k<bins;++k){auto c=xm[(size_t)k]*std::conj(xr[(size_t)k]);double rp=std::norm(xr[(size_t)k]),mp=std::norm(xm[(size_t)k]);if(averages==0){cross[(size_t)k]=c;refPower[(size_t)k]=rp;micPower[(size_t)k]=mp;}else{cross[(size_t)k]=cross[(size_t)k]*(1-alpha)+c*alpha;refPower[(size_t)k]=refPower[(size_t)k]*(1-alpha)+rp*alpha;micPower[(size_t)k]=micPower[(size_t)k]*(1-alpha)+mp*alpha;}}
  ++averages;readyFlag=averages>=8;
  if(!readyFlag)return;
  for(int k=1;k<bins;++k){auto& p=result[(size_t)k];p.frequency=k*rateHz/fftSize;p.valid=refPower[(size_t)k]>1.e-12&&micPower[(size_t)k]>1.e-12;if(!p.valid)continue;Complex h=cross[(size_t)k]/refPower[(size_t)k];p.magnitudeDb=20.*std::log10(std::max(std::abs(h),1.e-12));p.phaseDegrees=std::arg(h)*180./3.14159265358979323846;p.coherence=std::clamp(std::norm(cross[(size_t)k])/(refPower[(size_t)k]*micPower[(size_t)k]),0.,1.);}
 }
public:
 void reset(){filled=averages=0;rateHz=0;lastSequence=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});}
 bool ready()const{return readyFlag;}
 int averageCount()const{return averages;}
 double sampleRate()const{return rateHz;}
 void feed(const RawBlock& block){
  if(block.count<=0||block.count>blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;
  if((lastSequence&&block.sequence!=lastSequence+1)||(rateHz>0&&std::abs(block.sampleRate-rateHz)>1.e-6))reset();
  lastSequence=block.sequence;rateHz=block.sampleRate;
  for(int i=0;i<block.count;++i){double m=block.mic[(size_t)i],r=block.ref[(size_t)i];if(!std::isfinite(m)||!std::isfinite(r)){reset();return;}mic[(size_t)filled]=m;ref[(size_t)filled]=r;if(++filled==fftSize){calculate();filled=0;}}
 }
 std::optional<Point> at(double hz)const{if(!readyFlag||hz<=0||rateHz<=0)return {};double index=hz*fftSize/rateHz;int nearest=(int)std::round(index);if(nearest<1||nearest>=bins||!result[(size_t)nearest].valid)return {};return result[(size_t)nearest];}
};
}
