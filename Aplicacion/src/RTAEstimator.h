#pragma once
#include "SignalWindow.h"
#include "MultiWindowSpectrum.h"
#include "TransferFunctionCore.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <optional>
#include <memory>
#include <utility>
#include <vector>

// One instance per input channel; feed() runs on the message thread.
class RTAEstimator {
 static constexpr int capacity=32768;
 int size=16384;
 using Complex=std::complex<double>;
 std::array<Complex,capacity> spectrum{};
 std::array<double,capacity> recent{};
 std::array<double,capacity/2+1> power{};
 std::array<double,capacity/2+1> temporalSum{};
 std::vector<double> temporalFrames;
 MultiWindowSpectrum::TemporalMode temporalMode=MultiWindowSpectrum::TemporalMode::Fast;
 int temporalDepth=1,temporalPosition=0,temporalCount=0;
 int filled=0,frames=0;
 double sampleRate=0;
 double bandPowerCorrection=1.;
 bool valid=false;
 SignalWindow::Type windowType=SignalWindow::Type::Hann;
 bool multiWindowEnabled=true;
 std::unique_ptr<MultiWindowSpectrum> multi=std::make_unique<MultiWindowSpectrum>();
 std::uint64_t revisionNumber=0;
 static void fft(std::array<Complex,capacity>& a,int size){
  for(int i=1,j=0;i<size;++i){int bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int length=2;length<=size;length<<=1){auto step=std::polar(1.,-2.*3.14159265358979323846/length);for(int start=0;start<size;start+=length){Complex w{1,0};for(int j=0;j<length/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+length/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+length/2)]=u-v;w*=step;}}}
 }
 void calculate(){
  double windowSum=0,windowPower=0;
  for(int i=0;i<size;++i){double w=SignalWindow::coefficient(windowType,i,size);spectrum[(size_t)i]*=w;windowSum+=w;windowPower+=w*w;}
  bandPowerCorrection=windowSum*windowSum/(size*windowPower);
  fft(spectrum,size);
  const double amplitudeScale=2./windowSum;
  double alpha=temporalMode==MultiWindowSpectrum::TemporalMode::Fast?1.-std::exp(-size/(sampleRate*.125)):temporalMode==MultiWindowSpectrum::TemporalMode::Slow?1.-std::exp(-size/sampleRate):1.;
  for(int k=1;k<=size/2;++k){double gain=(k==size/2?.5:1.)*amplitudeScale;double p=std::norm(spectrum[(size_t)k])*gain*gain;if(temporalMode==MultiWindowSpectrum::TemporalMode::Linear){size_t slot=(size_t)temporalPosition*(size/2+1)+(size_t)k;temporalSum[(size_t)k]+=p-temporalFrames[slot];temporalFrames[slot]=p;power[(size_t)k]=temporalSum[(size_t)k]/std::min(temporalCount+1,temporalDepth);}else if(temporalMode==MultiWindowSpectrum::TemporalMode::None)power[(size_t)k]=p;else power[(size_t)k]=frames?power[(size_t)k]*(1-alpha)+p*alpha:p;}
  if(temporalMode==MultiWindowSpectrum::TemporalMode::Linear){temporalPosition=(temporalPosition+1)%temporalDepth;temporalCount=std::min(temporalCount+1,temporalDepth);}
  ++frames;++revisionNumber;valid=true;
 }
public:
 void setFFTSize(int value){if(value<128||value>capacity||(value&(value-1))!=0||size==value)return;size=value;if(temporalMode==MultiWindowSpectrum::TemporalMode::Linear)temporalFrames.assign((size_t)temporalDepth*(size/2+1),0.);reset();}
 int fftSize()const{return size;}
 void reset(){filled=frames=temporalPosition=temporalCount=0;sampleRate=0;valid=false;bandPowerCorrection=1.;spectrum.fill({});power.fill(0);temporalSum.fill(0);std::fill(temporalFrames.begin(),temporalFrames.end(),0.);multi->reset();++revisionNumber;}
 void clearAveragePreservingHistory(){frames=temporalPosition=temporalCount=0;power.fill(0);temporalSum.fill(0);std::fill(temporalFrames.begin(),temporalFrames.end(),0.);multi->clearAveragePreservingHistory();if(valid&&sampleRate>0){for(int i=0;i<size;++i)spectrum[(size_t)i]={recent[(size_t)i],0};calculate();for(int i=0;i<filled;++i)spectrum[(size_t)i]={recent[(size_t)i],0};}++revisionNumber;}
 void setTemporalAveraging(MultiWindowSpectrum::TemporalMode mode,int depth=1){depth=std::clamp(depth,1,64);if(temporalMode==mode&&temporalDepth==depth)return;temporalMode=mode;temporalDepth=depth;temporalPosition=temporalCount=frames=0;temporalFrames.clear();temporalSum.fill(0);if(mode==MultiWindowSpectrum::TemporalMode::Linear)temporalFrames.assign((size_t)depth*(size/2+1),0.);multi->setRtaTemporalMode(mode,depth);++revisionNumber;}
 void setWindowType(SignalWindow::Type type){if(windowType==type)return;windowType=type;reset();multi->setWindowType(type);}
 SignalWindow::Type getWindowType()const{return windowType;}
 void setMultiWindowEnabled(bool enabled){if(multiWindowEnabled==enabled)return;multiWindowEnabled=enabled;reset();}
 bool isMultiWindowEnabled()const{return multiWindowEnabled;}
 void feed(const TransferFunctionCore::RawBlock& block){if(block.count<=0||block.count>TransferFunctionCore::blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;if(sampleRate>0&&std::abs(sampleRate-block.sampleRate)>1.e-6)reset();sampleRate=block.sampleRate;for(int i=0;i<block.count;++i){float sample=block.mic[(size_t)i];if(!std::isfinite(sample)){reset();return;}if(multiWindowEnabled)multi->feed(sample,0,block.sampleRate);recent[(size_t)filled]=sample;spectrum[(size_t)filled]={sample,0};if(++filled==size){calculate();filled=0;}}}
 std::optional<double> dbfs(double hz)const{if(!valid||hz<=0||sampleRate<=0)return {};if(multiWindowEnabled)if(auto value=multi->rtaDbfs(hz))return value;int k=(int)std::round(hz*size/sampleRate);if(k<1||k>size/2)return {};return 10.*std::log10(std::max(1.e-20,power[(size_t)k]));}
 std::optional<double> smoothedDbfs(double hz,double divisions)const{if(divisions<=0)return dbfs(hz);if(!valid||hz<=0||sampleRate<=0)return {};double index=std::round(divisions*std::log2(hz/20.));double center=20.*std::pow(2.,index/divisions);double lower=center*std::pow(2.,-.5/divisions),upper=std::min(sampleRate*.5,center*std::pow(2.,.5/divisions));if(upper<=lower)return {};if(multiWindowEnabled)if(auto energy=multi->rtaBandPower(lower,upper))return 10.*std::log10(std::max(1.e-20,*energy));double binHz=sampleRate/size,total=0;bool found=false;int first=std::max(1,(int)std::floor(lower/binHz-.5));int last=std::min(size/2,(int)std::ceil(upper/binHz+.5));for(int k=first;k<=last;++k){double overlap=std::max(0.,std::min(upper,(k+.5)*binHz)-std::max(lower,(k-.5)*binHz))/binHz;if(overlap>0){total+=power[(size_t)k]*overlap;found=true;}}return found?std::optional<double>(10.*std::log10(std::max(1.e-20,total*bandPowerCorrection))):dbfs(center);}
 bool ready()const{return valid;}
 std::uint64_t revision()const{return revisionNumber+multi->revision();}
};
