#pragma once
#include "SignalWindow.h"
#include "TransferFunctionCore.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <optional>
#include <utility>

// One instance per input channel; feed() runs on the message thread.
class RTAEstimator {
 static constexpr int size=4096;
 using Complex=std::complex<double>;
 std::array<Complex,size> spectrum{};
 std::array<double,size/2+1> power{};
 int filled=0,frames=0;
 double sampleRate=0;
 bool valid=false;
 SignalWindow::Type windowType=SignalWindow::Type::Hann;
 static void fft(std::array<Complex,size>& a){
  for(int i=1,j=0;i<size;++i){int bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int length=2;length<=size;length<<=1){auto step=std::polar(1.,-2.*3.14159265358979323846/length);for(int start=0;start<size;start+=length){Complex w{1,0};for(int j=0;j<length/2;++j){auto u=a[(size_t)(start+j)],v=a[(size_t)(start+j+length/2)]*w;a[(size_t)(start+j)]=u+v;a[(size_t)(start+j+length/2)]=u-v;w*=step;}}}
 }
 void calculate(){
  double windowSum=0;
  for(int i=0;i<size;++i){double w=SignalWindow::coefficient(windowType,i,size);spectrum[(size_t)i]*=w;windowSum+=w;}
  fft(spectrum);
  const double amplitudeScale=2./windowSum;
  for(int k=1;k<=size/2;++k){double gain=(k==size/2?.5:1.)*amplitudeScale;double p=std::norm(spectrum[(size_t)k])*gain*gain;power[(size_t)k]=frames?power[(size_t)k]*.75+p*.25:p;}
  ++frames;valid=true;
 }
public:
 void reset(){filled=frames=0;sampleRate=0;valid=false;spectrum.fill({});power.fill(0);}
 void setWindowType(SignalWindow::Type type){if(windowType==type)return;windowType=type;reset();}
 SignalWindow::Type getWindowType()const{return windowType;}
 void feed(const TransferFunctionCore::RawBlock& block){if(block.count<=0||block.count>TransferFunctionCore::blockCapacity||!std::isfinite(block.sampleRate)||block.sampleRate<=0)return;if(sampleRate>0&&std::abs(sampleRate-block.sampleRate)>1.e-6)reset();sampleRate=block.sampleRate;for(int i=0;i<block.count;++i){float sample=block.mic[(size_t)i];if(!std::isfinite(sample)){reset();return;}spectrum[(size_t)filled]={sample,0};if(++filled==size){calculate();filled=0;}}}
 std::optional<double> dbfs(double hz)const{if(!valid||hz<=0||sampleRate<=0)return {};int k=(int)std::round(hz*size/sampleRate);if(k<1||k>size/2)return {};return 10.*std::log10(std::max(1.e-20,power[(size_t)k]));}
 bool ready()const{return valid;}
};
