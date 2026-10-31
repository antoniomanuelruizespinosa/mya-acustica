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

// One instance per input channel; feed() runs on the message thread.
class RTAEstimator {
 static constexpr int capacity=32768;
 int size=16384;
 using Complex=std::complex<double>;
 std::array<Complex,capacity> spectrum{};
 std::array<double,capacity/2+1> power{};
 int filled=0,frames=0;
 double sampleRate=0;
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
  double windowSum=0;
  for(int i=0;i<size;++i){double w=SignalWindow::coefficient(windowType,i,size);spectrum[(size_t)i]*=w;windowSum+=w;}
  fft(spectrum,size);
  const double amplitudeScale=2./windowSum;
  for(int k=1;k<=size/2;++k){double gain=(k==size/2?.5:1.)*amplitudeScale;double p=std::norm(spectrum[(size_t)k])*gain*gain;power[(size_t)k]=frames?power[(size_t)k]*.75+p*.25:p;}
  ++frames;++revisionNumber;valid=true;
 }
public:
 void setFFTSize(int value){if(value<128||value>capacity||(value&(value-1))!=0||size==value)return;size=value;reset();}
 int fftSize()const{return size;}
 void reset(){filled=frames=0;sampleRate=0;valid=false;spectrum.fill({});power.fill(0);multi->reset();++revisionNumber;}
 void setWindowType(SignalWindow::Type type){if(windowType==type)return;windowType=type;reset();multi->setWindowType(type);}
 SignalWindow::Type getWindowType()const{return windowType;}
 void setMultiWindowEnabled(bool enabled){if(multiWindowEnabled==enabled)return;multiWindowEnabled=enabled;reset();}
 bool isMultiWindowEnabled()const{return multiWindowEnabled;}
 void feed(const TransferFunctionCore::RawBlock& block){if(block.count<=0||block.count>TransferFunctionCore::blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;if(sampleRate>0&&std::abs(sampleRate-block.sampleRate)>1.e-6)reset();sampleRate=block.sampleRate;for(int i=0;i<block.count;++i){float sample=block.mic[(size_t)i];if(!std::isfinite(sample)){reset();return;}if(multiWindowEnabled)multi->feed(sample,0,block.sampleRate);spectrum[(size_t)filled]={sample,0};if(++filled==size){calculate();filled=0;}}}
 std::optional<double> dbfs(double hz)const{if(!valid||hz<=0||sampleRate<=0)return {};if(multiWindowEnabled)if(auto value=multi->rtaDbfs(hz))return value;int k=(int)std::round(hz*size/sampleRate);if(k<1||k>size/2)return {};return 10.*std::log10(std::max(1.e-20,power[(size_t)k]));}
 std::optional<double> smoothedDbfs(double hz,double divisions)const{auto original=dbfs(hz);if(!original||divisions<=0)return original;double ratio=std::pow(2.,.5/divisions);int samples=std::clamp((int)std::ceil(MultiWindowSpectrum::pointsPerOctave/divisions),4,32);double total=0;int count=0;for(int i=0;i<=samples;++i){auto value=dbfs(hz*std::pow(ratio,2.*i/samples-1.));if(value){total+=std::pow(10.,*value/10.);++count;}}return count?std::optional<double>(10.*std::log10(std::max(1.e-20,total/count))):original;}
 bool ready()const{return valid;}
 std::uint64_t revision()const{return revisionNumber+multi->revision();}
};
