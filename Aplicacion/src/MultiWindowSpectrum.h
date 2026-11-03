#pragma once

#include "SignalWindow.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <optional>
#include <vector>

// Five simultaneous, octave-spaced analysis rates. All work is done by feed()
// on the message thread; the audio callback only queues raw samples.
class MultiWindowSpectrum {
public:
 enum class TemporalMode { Legacy, None, Fast, Slow, Linear };
 static constexpr int size=16384;
 static constexpr int levels=5;
 static constexpr int pointsPerOctave=64;
 static int gridCount(){return (int)std::ceil(pointsPerOctave*std::log2(1000.))+1;}
 static double gridFrequency(int index){return std::min(20000.,20.*std::pow(2.,index/(double)pointsPerOctave));}
 using Complex=std::complex<double>;
 struct Transfer {double frequencyHz=0,magnitudeDb=0,phaseDegrees=0,coherence=0;};
private:
 struct Decimator {
  std::array<double,31> mic{},ref{};
  int position=0,phase=0;
  static const std::array<double,31>& taps(){static const auto h=[] {std::array<double,31> result{};double total=0;for(int i=0;i<31;++i){int n=i-15;double sinc=n==0?.44:std::sin(.44*3.14159265358979323846*n)/(3.14159265358979323846*n);double window=.54-.46*std::cos(2.*3.14159265358979323846*i/30.);result[(size_t)i]=sinc*window;total+=result[(size_t)i];}for(auto& v:result)v/=total;return result;}();return h;}
  bool push(double m,double r,double& outM,double& outR){mic[(size_t)position]=m;ref[(size_t)position]=r;position=(position+1)%31;phase^=1;if(phase)return false;outM=outR=0;int at=position;for(double h:taps()){at=(at+30)%31;outM+=h*mic[(size_t)at];outR+=h*ref[(size_t)at];}return true;}
 };
 struct Level {
  static constexpr int history=32768;
  std::array<double,history> mic{},ref{};
  std::array<Complex,size> xm{},xr{};
  std::array<Complex,size/2+1> cross{};
  std::array<double,size/2+1> micPower{},refPower{};
  int write=0,count=0,hop=0,averages=0;
  TemporalMode temporalMode=TemporalMode::Legacy;
  int temporalDepth=1,temporalPosition=0,temporalCount=0;
  std::vector<double> temporalFrames,temporalSum;
  double powerScale=1,bandPowerCorrection=1.;
  std::uint64_t revision=0;
  void reset(){write=count=hop=averages=temporalPosition=temporalCount=0;revision=0;mic.fill(0);ref.fill(0);cross.fill({});micPower.fill(0);refPower.fill(0);std::fill(temporalFrames.begin(),temporalFrames.end(),0.);std::fill(temporalSum.begin(),temporalSum.end(),0.);}
  void resetAverage(){averages=0;cross.fill({});micPower.fill(0);refPower.fill(0);temporalPosition=temporalCount=0;std::fill(temporalFrames.begin(),temporalFrames.end(),0.);std::fill(temporalSum.begin(),temporalSum.end(),0.);if(count>=size)revision=0;}
  void setTemporalMode(TemporalMode mode,int depth){temporalMode=mode;temporalDepth=std::clamp(depth,1,64);temporalFrames.clear();temporalSum.clear();if(mode==TemporalMode::Linear){temporalFrames.assign((size_t)temporalDepth*(size/2+1),0.);temporalSum.assign(size/2+1,0.);}resetAverage();}
  static void fft(std::array<Complex,size>& data){for(int i=1,j=0;i<size;++i){int bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(data[(size_t)i],data[(size_t)j]);}for(int length=2;length<=size;length<<=1){Complex step=std::polar(1.,-2.*3.14159265358979323846/length);for(int start=0;start<size;start+=length){Complex w{1,0};for(int j=0;j<length/2;++j){auto u=data[(size_t)(start+j)],v=data[(size_t)(start+j+length/2)]*w;data[(size_t)(start+j)]=u+v;data[(size_t)(start+j+length/2)]=u-v;w*=step;}}}}
  void calculate(double stageRate,double alignmentMs,int depth,SignalWindow::Type window){int delay=std::clamp((int)std::llround(alignmentMs*stageRate/1000.),0,history-size);double windowSum=0,windowPower=0;for(int i=0;i<size;++i){double w=SignalWindow::coefficient(window,i,size);int index=(write-size+i+history)%history;int reference=(index-delay+history)%history;xm[(size_t)i]={mic[(size_t)index]*w,0};xr[(size_t)i]={ref[(size_t)reference]*w,0};windowSum+=w;windowPower+=w*w;}bandPowerCorrection=windowSum*windowSum/(size*windowPower);fft(xm);fft(xr);double scale=2./windowSum;powerScale=scale*scale;double alpha=1./std::max(1,depth);double temporalAlpha=temporalMode==TemporalMode::Fast?1.-std::exp(-size/(2.*stageRate*.125)):temporalMode==TemporalMode::Slow?1.-std::exp(-size/(2.*stageRate)):1.;for(int k=1;k<=size/2;++k){double gain=(k==size/2?.5:1.)*scale;auto c=xm[(size_t)k]*std::conj(xr[(size_t)k]);double mp=std::norm(xm[(size_t)k])*gain*gain,rp=std::norm(xr[(size_t)k])*gain*gain;if(averages==0){cross[(size_t)k]=c;refPower[(size_t)k]=rp;}else{cross[(size_t)k]=cross[(size_t)k]*(1-alpha)+c*alpha;refPower[(size_t)k]=refPower[(size_t)k]*(1-alpha)+rp*alpha;}if(temporalMode==TemporalMode::Linear){size_t slot=(size_t)temporalPosition*(size/2+1)+(size_t)k;temporalSum[(size_t)k]+=mp-temporalFrames[slot];temporalFrames[slot]=mp;micPower[(size_t)k]=temporalSum[(size_t)k]/std::min(temporalCount+1,temporalDepth);}else if(temporalMode==TemporalMode::None)micPower[(size_t)k]=mp;else if(temporalMode==TemporalMode::Legacy)micPower[(size_t)k]=averages?micPower[(size_t)k]*(1-alpha)+mp*alpha:mp;else micPower[(size_t)k]=averages?micPower[(size_t)k]*(1-temporalAlpha)+mp*temporalAlpha:mp;}if(temporalMode==TemporalMode::Linear){temporalPosition=(temporalPosition+1)%temporalDepth;temporalCount=std::min(temporalCount+1,temporalDepth);}++averages;++revision;}
  void push(double m,double r,double stageRate,double alignmentMs,int depth,SignalWindow::Type window){mic[(size_t)write]=m;ref[(size_t)write]=r;write=(write+1)%history;count=std::min(count+1,history);if(count<size)return;if(++hop>=size/2||(count==size&&revision==0)){hop=0;calculate(stageRate,alignmentMs,depth,window);}}
 };
 std::array<Level,levels-1> stages{};
 std::array<Decimator,levels-1> decimators{};
 double sampleRate=0,delayMs=0;
 int averageDepth=5;
 SignalWindow::Type windowType=SignalWindow::Type::Hann;
 static int stageFor(double hz,double rate){int stage=0;while(stage<levels-1&&rate/(size*(1<<stage))>hz*(std::pow(2.,1./pointsPerOctave)-1.))++stage;return stage;}
public:
 void reset(){for(auto& level:stages)level.reset();decimators={};sampleRate=0;}
 void clearAveragePreservingHistory(){for(size_t i=0;i<stages.size();++i){auto& level=stages[i];level.resetAverage();if(level.count>=size)level.calculate(sampleRate/(1<<(i+1)),delayMs,averageDepth,windowType);}}
 void setWindowType(SignalWindow::Type type){if(windowType!=type){windowType=type;reset();}}
 void setAverageDepth(int depth){if(depth>0&&depth!=averageDepth)averageDepth=depth;}
 void setRtaTemporalMode(TemporalMode mode,int depth=1){for(auto& stage:stages)stage.setTemporalMode(mode,depth);}
 void setDelayMs(double value){if(std::isfinite(value)&&value>=0&&std::abs(value-delayMs)>1.e-9){delayMs=value;for(auto& stage:stages)stage.resetAverage();}}
 void feed(double m,double r,double rate){if(!std::isfinite(m)||!std::isfinite(r)||!std::isfinite(rate)||rate<=0)return;if(sampleRate>0&&std::abs(sampleRate-rate)>1.e-6)reset();sampleRate=rate;for(int stage=1;stage<levels;++stage){double nextM,nextR;if(!decimators[(size_t)(stage-1)].push(m,r,nextM,nextR))break;m=nextM;r=nextR;stages[(size_t)(stage-1)].push(m,r,rate/(1<<stage),delayMs,averageDepth,windowType);}}
 std::optional<double> rtaDbfs(double hz)const{if(hz<=0||sampleRate<=0)return {};int stage=stageFor(hz,sampleRate);while(stage>=1){const auto& data=stages[(size_t)(stage-1)];int bin=(int)std::llround(hz*size*(1<<stage)/sampleRate);if(data.revision&&bin>=1&&bin<=size/2)return 10.*std::log10(std::max(1.e-20,data.micPower[(size_t)bin]));--stage;}return {};}
 std::optional<double> rtaBandPower(double lower,double upper)const{if(lower<=0||upper<=lower||sampleRate<=0)return {};int stage=stageFor(std::sqrt(lower*upper),sampleRate);while(stage>=1){const auto& data=stages[(size_t)(stage-1)];if(data.revision){double binHz=sampleRate/(size*(1<<stage)),total=0;bool found=false;int first=std::max(1,(int)std::floor(lower/binHz-.5));int last=std::min(size/2,(int)std::ceil(upper/binHz+.5));for(int k=first;k<=last;++k){double overlap=std::max(0.,std::min(upper,(k+.5)*binHz)-std::max(lower,(k-.5)*binHz))/binHz;if(overlap>0){total+=data.micPower[(size_t)k]*overlap;found=true;}}if(found)return total*data.bandPowerCorrection;}--stage;}return {};}
 std::optional<Transfer> transferAt(double hz)const{if(hz<=0||sampleRate<=0)return {};int stage=stageFor(hz,sampleRate);while(stage>=1){const auto& data=stages[(size_t)(stage-1)];int bin=(int)std::llround(hz*size*(1<<stage)/sampleRate);if(data.revision&&bin>=1&&bin<=size/2){double rp=data.refPower[(size_t)bin],mp=data.micPower[(size_t)bin];if(rp<=1.e-12||mp<=1.e-12)return {};auto h=data.cross[(size_t)bin]*data.powerScale/rp;double binHz=bin*sampleRate/(size*(1<<stage));h*=std::polar(1.,-2.*3.14159265358979323846*binHz*delayMs/1000.);Transfer result;result.frequencyHz=binHz;result.magnitudeDb=20.*std::log10(std::max(1.e-12,std::abs(h)));result.phaseDegrees=std::arg(h)*180./3.14159265358979323846;result.coherence=std::clamp(std::norm(data.cross[(size_t)bin])*data.powerScale*data.powerScale/(rp*mp),0.,1.);return result;}--stage;}return {};}
 std::uint64_t revision()const{std::uint64_t total=0;for(const auto& stage:stages)total+=stage.revision;return total;}
};
