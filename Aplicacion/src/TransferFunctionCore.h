#pragma once
#include "SignalWindow.h"
#include "MultiWindowSpectrum.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdint>
#include <optional>
#include <memory>
#include <vector>

namespace TransferFunctionCore {
constexpr int blockCapacity=2048;
constexpr int fftSize=16384;
constexpr int bins=fftSize/2+1;
constexpr int maxFftSize=32768;
constexpr int maxBins=maxFftSize/2+1;
constexpr double impulseTimeLimitMs=85.;
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
 int activeSize=fftSize;
 int activeBins()const{return activeSize/2+1;}
 std::array<double,maxFftSize> mic{},ref{};
 std::array<double,maxFftSize> previousRef{};
 std::array<Complex,maxFftSize> xm{},xr{};
 std::array<Complex,maxBins> cross{};
 std::array<double,maxBins> refPower{},micPower{};
 std::array<Point,maxBins> result{};
 std::array<Complex,maxBins> transfer{};
 std::array<double,maxBins> powerIntegral{},weightIntegral{};
 std::vector<double> coherenceIntegral=std::vector<double>(maxBins,0.);
 std::array<Complex,maxBins> phaseIntegral{};
 std::array<int,maxBins> invalidIntegral{};
 std::array<double,maxFftSize> impulse{};
 std::array<double,maxFftSize> envelope{};
 int filled=0,averages=0;
 double infiniteWindows=0;
 double rateHz=0;
 double delayMs=0;
 int averageDepth=8;
 MultiWindowSpectrum::TfAverageMode averageMode=MultiWindowSpectrum::TfAverageMode::Depth;
 SignalWindow::Type windowType=SignalWindow::Type::Hann;
 bool multiWindowEnabled=true;
 std::unique_ptr<MultiWindowSpectrum> multi=std::make_unique<MultiWindowSpectrum>();
 std::uint64_t revisionNumber=0;
 std::uint64_t lastSequence=0;
 bool readyFlag=false,sweepMeasurement=false,pendingAlignmentReset=false;
 void rebuildSmoothing(){powerIntegral.fill(0);weightIntegral.fill(0);std::fill(coherenceIntegral.begin(),coherenceIntegral.end(),0.);phaseIntegral.fill({});invalidIntegral.fill(0);if(!readyFlag)return;for(int k=1;k<activeBins();++k){const auto& point=result[(size_t)k];bool valid=point.valid&&point.frequency>0;double weight=valid?std::log((k+.5)/(k-.5)):0.;auto h=transfer[(size_t)k]*std::polar(1.,2.*3.14159265358979323846*point.frequency*delayMs/1000.);double magnitude=std::abs(h);powerIntegral[(size_t)k]=powerIntegral[(size_t)(k-1)]+(valid?magnitude*magnitude*weight:0.);weightIntegral[(size_t)k]=weightIntegral[(size_t)(k-1)]+weight;coherenceIntegral[(size_t)k]=coherenceIntegral[(size_t)(k-1)]+(valid?point.coherence*weight:0.);phaseIntegral[(size_t)k]=phaseIntegral[(size_t)(k-1)]+(valid&&magnitude>1.e-15?h/magnitude*weight:Complex{});invalidIntegral[(size_t)k]=invalidIntegral[(size_t)(k-1)]+!valid;}}
 static void fft(std::array<Complex,maxFftSize>& a,int size,bool inverse=false){
  for(int i=1,j=0;i<size;++i){int bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int len=2;len<=size;len<<=1){double angle=(inverse?2.:-2.)*3.14159265358979323846/len;Complex step{std::cos(angle),std::sin(angle)};for(int start=0;start<size;start+=len){Complex w{1,0};for(int j=0;j<len/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+len/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+len/2)]=u-v;w*=step;}}}
  if(inverse)for(int i=0;i<size;++i)a[(size_t)i]/=(double)size;
 }
 void rebuildImpulse(){
  impulse.fill(0);envelope.fill(0);if(!readyFlag||rateHz<=0)return;
  std::array<Complex,maxFftSize> spectrum{};
  for(int k=1;k<activeBins();++k){auto h=transfer[(size_t)k]*std::polar(1.,2.*3.14159265358979323846*k*delayMs*rateHz/(activeSize*1000.));spectrum[(size_t)k]=h;if(k<activeSize/2)spectrum[(size_t)(activeSize-k)]=std::conj(h);}
  auto analytic=spectrum;for(int k=1;k<activeSize/2;++k)analytic[(size_t)k]*=2.;for(int k=activeSize/2+1;k<activeSize;++k)analytic[(size_t)k]={};fft(analytic,activeSize,true);
  fft(spectrum,activeSize,true);double peak=0;for(int i=0;i<activeSize;++i)peak=std::max(peak,std::abs(spectrum[(size_t)i].real()));
  if(peak>1.e-12)for(int i=0;i<activeSize;++i){impulse[(size_t)i]=spectrum[(size_t)i].real()/peak;envelope[(size_t)i]=std::abs(analytic[(size_t)i])/peak;}
 }
 void calculate(){
  sweepMeasurement=false;
  if(pendingAlignmentReset){averages=0;infiniteWindows=0;cross.fill({});refPower.fill(0);micPower.fill(0);pendingAlignmentReset=false;}
  double refEnergy=0;for(int i=0;i<activeSize;++i)refEnergy+=ref[(size_t)i]*ref[(size_t)i];
  // Below -60 dBFS RMS there is no dependable excitation for a live TF.
  if(refEnergy<activeSize*1.e-6){averages=0;infiniteWindows=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});transfer.fill({});impulse.fill(0);envelope.fill(0);return;}
  int alignedSamples=std::clamp((int)std::llround(delayMs*rateHz/1000.),0,activeSize-1);
  for(int i=0;i<activeSize;++i){double w=SignalWindow::coefficient(windowType,i,activeSize);xm[(size_t)i]={mic[(size_t)i]*w,0};double aligned=i>=alignedSamples?ref[(size_t)(i-alignedSamples)]:previousRef[(size_t)(activeSize+i-alignedSamples)];xr[(size_t)i]={aligned*w,0};}
  previousRef=ref;
  fft(xm,activeSize);fft(xr,activeSize);
  double alpha=averageMode==MultiWindowSpectrum::TfAverageMode::Fast?1.-std::exp(-activeSize/(rateHz*.125)):averageMode==MultiWindowSpectrum::TfAverageMode::Slow?1.-std::exp(-activeSize/rateHz):averageMode==MultiWindowSpectrum::TfAverageMode::Infinite?1./(infiniteWindows+1.):1./averageDepth;
  for(int k=1;k<activeBins();++k){auto c=xm[(size_t)k]*std::conj(xr[(size_t)k]);double rp=std::norm(xr[(size_t)k]),mp=std::norm(xm[(size_t)k]);if(averages==0){cross[(size_t)k]=c;refPower[(size_t)k]=rp;micPower[(size_t)k]=mp;}else{cross[(size_t)k]=cross[(size_t)k]*(1-alpha)+c*alpha;refPower[(size_t)k]=refPower[(size_t)k]*(1-alpha)+rp*alpha;micPower[(size_t)k]=micPower[(size_t)k]*(1-alpha)+mp*alpha;}}
  averages=std::min(averages+1,1000000);infiniteWindows+=1.;readyFlag=averages>=(averageMode==MultiWindowSpectrum::TfAverageMode::Depth?averageDepth:1);
  if(!readyFlag)return;
  for(int k=1;k<activeBins();++k){auto& p=result[(size_t)k];p.frequency=k*rateHz/activeSize;p.valid=refPower[(size_t)k]>1.e-12&&micPower[(size_t)k]>1.e-12;if(!p.valid){transfer[(size_t)k]={};continue;}Complex h=cross[(size_t)k]/refPower[(size_t)k]*std::polar(1.,-2.*3.14159265358979323846*p.frequency*alignedSamples/rateHz);transfer[(size_t)k]=h;p.magnitudeDb=20.*std::log10(std::max(std::abs(h),1.e-12));p.phaseDegrees=std::arg(h)*180./3.14159265358979323846;p.coherence=std::clamp(std::norm(cross[(size_t)k])/(refPower[(size_t)k]*micPower[(size_t)k]),0.,1.);}
  rebuildSmoothing();rebuildImpulse();++revisionNumber;
 }
public:
 struct DelaySnapshot {int fftSize=0;double sampleRate=0;std::vector<Complex> transfer;};
 std::optional<DelaySnapshot> delaySnapshot()const{if(!readyFlag||rateHz<=0)return {};DelaySnapshot snapshot;snapshot.fftSize=activeSize;snapshot.sampleRate=rateHz;snapshot.transfer.assign(transfer.begin(),transfer.begin()+activeBins());return snapshot;}
 static std::optional<double> findDelayMs(const DelaySnapshot& snapshot){if(snapshot.fftSize<128||snapshot.fftSize>maxFftSize||snapshot.sampleRate<=0||snapshot.transfer.size()!=(size_t)(snapshot.fftSize/2+1))return {};auto spectrum=std::make_unique<std::array<Complex,maxFftSize>>();for(int k=1;k<=snapshot.fftSize/2;++k){auto h=snapshot.transfer[(size_t)k];(*spectrum)[(size_t)k]=h;if(k<snapshot.fftSize/2)(*spectrum)[(size_t)(snapshot.fftSize-k)]=std::conj(h);}fft(*spectrum,snapshot.fftSize,true);int limit=std::min(snapshot.fftSize/2-1,(int)std::round(snapshot.sampleRate*.15));int best=0;double peak=0,energy=0;for(int i=0;i<=limit;++i){double value=std::abs((*spectrum)[(size_t)i].real());energy+=value*value;if(value>peak){peak=value;best=i;}}if(peak<1.e-9||peak<5.*std::sqrt(energy/(limit+1)))return {};return 1000.*best/snapshot.sampleRate;}
 void setFFTSize(int value){if(value<128||value>maxFftSize||(value&(value-1))!=0||activeSize==value)return;activeSize=value;reset();}
 int fftSizeSamples()const{return activeSize;}
 void reset(){filled=averages=0;infiniteWindows=0;rateHz=0;lastSequence=0;readyFlag=false;sweepMeasurement=false;pendingAlignmentReset=false;previousRef.fill(0);cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});transfer.fill({});impulse.fill(0);envelope.fill(0);multi->reset();++revisionNumber;}
 void clearAveragePreservingHistory(){if(sweepMeasurement)return;bool hadMeasurement=readyFlag;averages=0;infiniteWindows=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);result.fill({});transfer.fill({});impulse.fill(0);envelope.fill(0);multi->clearAveragePreservingHistory();if(hadMeasurement&&rateHz>0){int depth=averageDepth;averageDepth=1;calculate();averageDepth=depth;}++revisionNumber;}
 void setWindowType(SignalWindow::Type type){if(windowType==type)return;windowType=type;reset();multi->setWindowType(type);}
 SignalWindow::Type getWindowType()const{return windowType;}
 void setMultiWindowEnabled(bool enabled){if(multiWindowEnabled==enabled)return;multiWindowEnabled=enabled;reset();}
 bool isMultiWindowEnabled()const{return multiWindowEnabled;}
 bool isSweep()const{return sweepMeasurement;}
 void loadSweepSpectrum(const std::vector<Complex>& values,const std::vector<bool>& valid,double rate){if(values.size()!=activeBins()||valid.size()!=activeBins()||rate<=0)return;reset();rateHz=rate;averages=1;readyFlag=sweepMeasurement=true;for(int k=1;k<activeBins();++k){auto& p=result[(size_t)k];p.frequency=k*rate/activeSize;p.valid=valid[(size_t)k]&&std::isfinite(values[(size_t)k].real())&&std::isfinite(values[(size_t)k].imag());if(!p.valid)continue;transfer[(size_t)k]=values[(size_t)k];p.magnitudeDb=20.*std::log10(std::max(1.e-12,std::abs(transfer[(size_t)k])));p.phaseDegrees=std::arg(transfer[(size_t)k])*180./3.14159265358979323846;p.coherence=0;}rebuildSmoothing();rebuildImpulse();}
 void setDelayMs(double value){if(!std::isfinite(value)||value<0||value>2000)return;if(std::abs(delayMs-value)>1.e-6)pendingAlignmentReset=true;delayMs=value;multi->setDelayMs(value);rebuildSmoothing();rebuildImpulse();}
 void setAverageDepth(int depth){if(depth<1||depth>64||averageMode==MultiWindowSpectrum::TfAverageMode::Depth&&averageDepth==depth)return;averageDepth=depth;averageMode=MultiWindowSpectrum::TfAverageMode::Depth;averages=0;infiniteWindows=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);multi->setAverageDepth(depth);multi->setTfAverageMode(averageMode);++revisionNumber;}
 void setAverageMode(MultiWindowSpectrum::TfAverageMode mode){if(averageMode==mode)return;averageMode=mode;averages=0;infiniteWindows=0;readyFlag=false;cross.fill({});refPower.fill(0);micPower.fill(0);multi->setTfAverageMode(mode);++revisionNumber;}
 int averaging()const{return averageDepth;}
 double delay()const{return delayMs;}
 std::optional<double> findDelayMs()const{auto snapshot=delaySnapshot();return snapshot?findDelayMs(*snapshot):std::nullopt;}
 std::optional<double> impulseAt(double timeMs,bool useEnvelope=false)const{
  if(!readyFlag||rateHz<=0||!std::isfinite(timeMs)||std::abs(timeMs)>impulseTimeLimitMs)return {};
  int sample=(int)std::round(timeMs*rateHz/1000.);if(std::abs(sample)>=activeSize/2)return {};
  return (useEnvelope?envelope:impulse)[(size_t)((sample+activeSize)%activeSize)];
 }
 bool ready()const{return readyFlag;}
 int averageCount()const{return averages;}
 std::uint64_t revision()const{return revisionNumber+multi->revision();}
 double sampleRate()const{return rateHz;}
 void feed(const RawBlock& block){
  if(block.count<=0||block.count>blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;
  if((lastSequence&&block.sequence!=lastSequence+1)||(rateHz>0&&std::abs(block.sampleRate-rateHz)>1.e-6))reset();
  lastSequence=block.sequence;rateHz=block.sampleRate;
  for(int i=0;i<block.count;++i){double m=block.mic[(size_t)i],r=block.ref[(size_t)i];if(!std::isfinite(m)||!std::isfinite(r)){reset();return;}if(multiWindowEnabled)multi->feed(m,r,block.sampleRate);mic[(size_t)filled]=m;ref[(size_t)filled]=r;if(++filled==activeSize){calculate();filled=0;}}
 }
 std::optional<Point> at(double hz)const{if(!readyFlag||hz<=0||rateHz<=0)return {};if(multiWindowEnabled&&!sweepMeasurement&&hz<270.)if(auto narrow=multi->transferAt(hz)){Point point;point.frequency=narrow->frequencyHz;point.magnitudeDb=narrow->magnitudeDb;point.phaseDegrees=std::arg(std::polar(1.,narrow->phaseDegrees*3.14159265358979323846/180.+2.*3.14159265358979323846*point.frequency*delayMs/1000.))*180./3.14159265358979323846;point.coherence=narrow->coherence;point.valid=true;return point;}double index=hz*activeSize/rateHz;int nearest=(int)std::round(index);if(nearest<1||nearest>=activeBins()||!result[(size_t)nearest].valid)return {};auto point=result[(size_t)nearest];point.phaseDegrees=std::arg(transfer[(size_t)nearest]*std::polar(1.,2.*3.14159265358979323846*point.frequency*delayMs/1000.))*180./3.14159265358979323846;return point;}
 std::optional<Point> smoothedAt(double hz,double divisions,bool phase)const{
  auto point=at(hz);if(!point||divisions<=0)return point;
  if(multiWindowEnabled&&!sweepMeasurement&&hz<270.&&multi->transferAt(hz)){
   int steps=std::clamp((int)std::ceil(MultiWindowSpectrum::pointsPerOctave/divisions),2,64);double power=0.,coherence=0.;Complex vector{};int count=0;
   for(int i=0;i<=steps;++i){double offset=((double)i/steps-.5)/divisions;auto sample=at(hz*std::pow(2.,offset));if(!sample)continue;double amplitude=std::pow(10.,sample->magnitudeDb/20.);power+=amplitude*amplitude;coherence+=sample->coherence;vector+=std::polar(1.,sample->phaseDegrees*3.14159265358979323846/180.);++count;}
   if(count==0)return {};point->coherence=coherence/count;if(phase){if(std::abs(vector)<1.e-12)return {};point->phaseDegrees=std::arg(vector)*180./3.14159265358979323846;}else point->magnitudeDb=10.*std::log10(std::max(1.e-24,power/count));return point;
  }
  double ratio=std::pow(2.,.5/divisions),spacing=rateHz/activeSize;
  double bandLow=std::max(spacing*.5,hz/ratio),bandHigh=std::min(rateHz*.5,hz*ratio);
  int lo=std::max(1,(int)std::floor(bandLow/spacing+.5));
  int hi=std::min(activeBins()-1,(int)std::ceil(bandHigh/spacing-.5));
  if(hi<lo)return point;
  if(invalidIntegral[(size_t)hi]-invalidIntegral[(size_t)(lo-1)]>0)return {};
  auto clippedWeight=[&](int k){double lower=std::max(bandLow,(k-.5)*spacing),upper=std::min(bandHigh,(k+.5)*spacing);return upper>lower?std::log(upper/lower):0.;};
  double leftMissing=std::log((lo+.5)/(lo-.5))-clippedWeight(lo);
  double rightMissing=lo==hi?0.:std::log((hi+.5)/(hi-.5))-clippedWeight(hi);
  double weight=weightIntegral[(size_t)hi]-weightIntegral[(size_t)(lo-1)]-leftMissing-rightMissing;if(weight<=0.)return {};
  point->coherence=std::clamp((coherenceIntegral[(size_t)hi]-coherenceIntegral[(size_t)(lo-1)]-result[(size_t)lo].coherence*leftMissing-result[(size_t)hi].coherence*rightMissing)/weight,0.,1.);
  if(phase){auto edge=[&](int k){auto h=transfer[(size_t)k]*std::polar(1.,2.*3.14159265358979323846*result[(size_t)k].frequency*delayMs/1000.);return std::abs(h)>1.e-15?h/std::abs(h):Complex{};};auto vector=(phaseIntegral[(size_t)hi]-phaseIntegral[(size_t)(lo-1)]-edge(lo)*leftMissing-edge(hi)*rightMissing)/weight;if(std::abs(vector)<1.e-15)return {};point->phaseDegrees=std::arg(vector)*180./3.14159265358979323846;}
  else{double leftPower=std::norm(transfer[(size_t)lo]),rightPower=std::norm(transfer[(size_t)hi]);double power=(powerIntegral[(size_t)hi]-powerIntegral[(size_t)(lo-1)]-leftPower*leftMissing-rightPower*rightMissing)/weight;point->magnitudeDb=10.*std::log10(std::max(1.e-24,power));}
  return point;
 }
};
}
