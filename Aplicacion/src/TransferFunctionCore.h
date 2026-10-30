#pragma once
#include "SignalWindow.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdint>
#include <optional>
#include <vector>

namespace TransferFunctionCore {
constexpr int blockCapacity=2048;
constexpr int fftSize=16384;
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
 std::array<double,fftSize> previousRef{};
 std::array<Complex,fftSize> xm{},xr{};
 std::array<Complex,bins> cross{};
 std::array<double,bins> refPower{},micPower{};
 std::array<Point,bins> result{};
 std::array<Complex,bins> transfer{};
 std::array<double,bins> magnitudeIntegral{};
 std::array<Complex,bins> phaseIntegral{};
 std::array<int,bins> invalidIntegral{};
 std::array<double,fftSize> impulse{};
 std::array<double,fftSize> envelope{};
 int filled=0,averages=0;
 double rateHz=0;
 double delayMs=0;
 int averageDepth=8;
 SignalWindow::Type windowType=SignalWindow::Type::Hann;
 std::uint64_t lastSequence=0;
 bool readyFlag=false,sweepMeasurement=false,pendingAlignmentReset=false;
 void rebuildSmoothing(){magnitudeIntegral.fill(0);phaseIntegral.fill({});invalidIntegral.fill(0);if(!readyFlag)return;for(int k=1;k<bins;++k){bool valid=result[(size_t)k].valid;auto h=transfer[(size_t)k]*std::polar(1.,2.*3.14159265358979323846*result[(size_t)k].frequency*delayMs/1000.);magnitudeIntegral[(size_t)k]=magnitudeIntegral[(size_t)(k-1)]+(valid?result[(size_t)k].magnitudeDb:0.);phaseIntegral[(size_t)k]=phaseIntegral[(size_t)(k-1)]+(valid?h:Complex{});invalidIntegral[(size_t)k]=invalidIntegral[(size_t)(k-1)]+!valid;}}
 static void fft(std::array<Complex,fftSize>& a,bool inverse=false){
  for(int i=1,j=0;i<fftSize;++i){int bit=fftSize>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int len=2;len<=fftSize;len<<=1){double angle=(inverse?2.:-2.)*3.14159265358979323846/len;Complex step{std::cos(angle),std::sin(angle)};for(int start=0;start<fftSize;start+=len){Complex w{1,0};for(int j=0;j<len/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+len/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+len/2)]=u-v;w*=step;}}}
  if(inverse)for(auto& value:a)value/=(double)fftSize;
 }
 void rebuildImpulse(){
  impulse.fill(0);envelope.fill(0);if(!readyFlag||rateHz<=0)return;
  std::array<Complex,fftSize> spectrum{};
  for(int k=1;k<bins;++k){auto h=transfer[(size_t)k]*std::polar(1.,2.*3.14159265358979323846*k*delayMs*rateHz/(fftSize*1000.));spectrum[(size_t)k]=h;if(k<fftSize/2)spectrum[(size_t)(fftSize-k)]=std::conj(h);}
  auto analytic=spectrum;for(int k=1;k<fftSize/2;++k)analytic[(size_t)k]*=2.;for(int k=fftSize/2+1;k<fftSize;++k)analytic[(size_t)k]={};fft(analytic,true);
  fft(spectrum,true);double peak=0;for(const auto& value:spectrum)peak=std::max(peak,std::abs(value.real()));
  if(peak>1.e-12)for(int i=0;i<fftSize;++i){impulse[(size_t)i]=spectrum[(size_t)i].real()/peak;envelope[(size_t)i]=std::abs(analytic[(size_t)i])/peak;}
 }
 void calculate(){
  sweepMeasurement=false;
  if(pendingAlignmentReset){averages=0;cross.fill({});refPower.fill(0);micPower.fill(0);pendingAlignmentReset=false;}
  double refEnergy=0;for(double value:ref)refEnergy+=value*value;
  // Below -60 dBFS RMS there is no dependable excitation for a live TF.
  if(refEnergy<fftSize*1.e-6){averages=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});transfer.fill({});impulse.fill(0);envelope.fill(0);return;}
  int alignedSamples=std::clamp((int)std::llround(delayMs*rateHz/1000.),0,fftSize-1);
  for(int i=0;i<fftSize;++i){double w=SignalWindow::coefficient(windowType,i,fftSize);xm[(size_t)i]={mic[(size_t)i]*w,0};double aligned=i>=alignedSamples?ref[(size_t)(i-alignedSamples)]:previousRef[(size_t)(fftSize+i-alignedSamples)];xr[(size_t)i]={aligned*w,0};}
  previousRef=ref;
  fft(xm);fft(xr);
  double alpha=1./averageDepth;
  for(int k=1;k<bins;++k){auto c=xm[(size_t)k]*std::conj(xr[(size_t)k]);double rp=std::norm(xr[(size_t)k]),mp=std::norm(xm[(size_t)k]);if(averages==0){cross[(size_t)k]=c;refPower[(size_t)k]=rp;micPower[(size_t)k]=mp;}else{cross[(size_t)k]=cross[(size_t)k]*(1-alpha)+c*alpha;refPower[(size_t)k]=refPower[(size_t)k]*(1-alpha)+rp*alpha;micPower[(size_t)k]=micPower[(size_t)k]*(1-alpha)+mp*alpha;}}
  averages=std::min(averages+1,1000000);readyFlag=averages>=averageDepth;
  if(!readyFlag)return;
  for(int k=1;k<bins;++k){auto& p=result[(size_t)k];p.frequency=k*rateHz/fftSize;p.valid=refPower[(size_t)k]>1.e-12&&micPower[(size_t)k]>1.e-12;if(!p.valid){transfer[(size_t)k]={};continue;}Complex h=cross[(size_t)k]/refPower[(size_t)k]*std::polar(1.,-2.*3.14159265358979323846*p.frequency*alignedSamples/rateHz);transfer[(size_t)k]=h;p.magnitudeDb=20.*std::log10(std::max(std::abs(h),1.e-12));p.phaseDegrees=std::arg(h)*180./3.14159265358979323846;p.coherence=std::clamp(std::norm(cross[(size_t)k])/(refPower[(size_t)k]*micPower[(size_t)k]),0.,1.);}
  rebuildSmoothing();rebuildImpulse();
 }
public:
 void reset(){filled=averages=0;rateHz=0;lastSequence=0;readyFlag=false;sweepMeasurement=false;pendingAlignmentReset=false;previousRef.fill(0);cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});transfer.fill({});impulse.fill(0);envelope.fill(0);}
 void setWindowType(SignalWindow::Type type){if(windowType==type)return;windowType=type;reset();}
 SignalWindow::Type getWindowType()const{return windowType;}
 bool isSweep()const{return sweepMeasurement;}
 void loadSweepSpectrum(const std::vector<Complex>& values,const std::vector<bool>& valid,double rate){if(values.size()!=bins||valid.size()!=bins||rate<=0)return;reset();rateHz=rate;averages=1;readyFlag=sweepMeasurement=true;for(int k=1;k<bins;++k){auto& p=result[(size_t)k];p.frequency=k*rate/fftSize;p.valid=valid[(size_t)k]&&std::isfinite(values[(size_t)k].real())&&std::isfinite(values[(size_t)k].imag());if(!p.valid)continue;transfer[(size_t)k]=values[(size_t)k];p.magnitudeDb=20.*std::log10(std::max(1.e-12,std::abs(transfer[(size_t)k])));p.phaseDegrees=std::arg(transfer[(size_t)k])*180./3.14159265358979323846;p.coherence=0;}rebuildSmoothing();rebuildImpulse();}
 void setDelayMs(double value){if(!std::isfinite(value)||value<0||value>2000)return;if(std::abs(delayMs-value)>1.e-6)pendingAlignmentReset=true;delayMs=value;rebuildSmoothing();rebuildImpulse();}
 void setAverageDepth(int depth){if(depth<1||depth>64||averageDepth==depth)return;averageDepth=depth;reset();}
 int averaging()const{return averageDepth;}
 double delay()const{return delayMs;}
 std::optional<double> findDelayMs()const{
  if(!readyFlag||rateHz<=0)return {};
  std::array<Complex,fftSize> spectrum{};
  for(int k=1;k<bins;++k){auto h=transfer[(size_t)k];spectrum[(size_t)k]=h;if(k<fftSize/2)spectrum[(size_t)(fftSize-k)]=std::conj(h);}
  fft(spectrum,true);int limit=std::min(fftSize/2-1,(int)std::round(rateHz*.15));int best=0;double peak=0,energy=0;for(int i=0;i<=limit;++i){double value=std::abs(spectrum[(size_t)i].real());energy+=value*value;if(value>peak){peak=value;best=i;}}
  if(peak<1.e-9||peak<5.*std::sqrt(energy/(limit+1)))return {};
  return 1000.*best/rateHz;
 }
 std::optional<double> impulseAt(double timeMs,bool useEnvelope=false)const{
  if(!readyFlag||rateHz<=0||!std::isfinite(timeMs)||std::abs(timeMs)>40.)return {};
  int sample=(int)std::round(timeMs*rateHz/1000.);if(std::abs(sample)>=fftSize/2)return {};
  return (useEnvelope?envelope:impulse)[(size_t)((sample+fftSize)%fftSize)];
 }
 bool ready()const{return readyFlag;}
 int averageCount()const{return averages;}
 double sampleRate()const{return rateHz;}
 void feed(const RawBlock& block){
  if(block.count<=0||block.count>blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;
  if((lastSequence&&block.sequence!=lastSequence+1)||(rateHz>0&&std::abs(block.sampleRate-rateHz)>1.e-6))reset();
  lastSequence=block.sequence;rateHz=block.sampleRate;
  for(int i=0;i<block.count;++i){double m=block.mic[(size_t)i],r=block.ref[(size_t)i];if(!std::isfinite(m)||!std::isfinite(r)){reset();return;}mic[(size_t)filled]=m;ref[(size_t)filled]=r;if(++filled==fftSize){calculate();filled=0;}}
 }
 std::optional<Point> at(double hz)const{if(!readyFlag||hz<=0||rateHz<=0)return {};double index=hz*fftSize/rateHz;int nearest=(int)std::round(index);if(nearest<1||nearest>=bins||!result[(size_t)nearest].valid)return {};auto point=result[(size_t)nearest];point.phaseDegrees=std::arg(transfer[(size_t)nearest]*std::polar(1.,2.*3.14159265358979323846*point.frequency*delayMs/1000.))*180./3.14159265358979323846;return point;}
 std::optional<Point> smoothedAt(double hz,double divisions,bool phase)const{auto point=at(hz);if(!point||divisions<=0)return point;double ratio=std::pow(2.,.5/divisions);int lo=std::max(1,(int)std::ceil(hz/ratio*fftSize/rateHz));int hi=std::min(bins-1,(int)std::floor(hz*ratio*fftSize/rateHz));if(hi<lo||invalidIntegral[(size_t)hi]-invalidIntegral[(size_t)(lo-1)]>0)return {};int count=hi-lo+1;if(phase){auto h=(phaseIntegral[(size_t)hi]-phaseIntegral[(size_t)(lo-1)])/(double)count;if(std::abs(h)<1.e-15)return {};point->phaseDegrees=std::arg(h)*180./3.14159265358979323846;}else point->magnitudeDb=(magnitudeIntegral[(size_t)hi]-magnitudeIntegral[(size_t)(lo-1)])/count;return point;}
};
}
