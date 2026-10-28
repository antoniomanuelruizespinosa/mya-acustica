#pragma once
#include "DspMath.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <optional>
#include <vector>

namespace SpectrumCore {
enum class Average { None, Fifo, Exponential };
struct Settings {int fftSize=32768;MyaDSP::Window window=MyaDSP::Window::Hann;double overlap=.75;Average average=Average::Exponential;int depth=4;double framesPerSecond=20.;};
struct Frame {double sampleRate=0.;std::vector<double> db;};
class Analyzer {
 Settings settings;std::vector<double> pending;std::deque<std::vector<double>> fifo;Frame current;std::deque<Frame> history;int since=0;
 void transform(double rate){int n=settings.fftSize;std::vector<MyaDSP::Complex> x((size_t)n);double gain=0;for(int i=0;i<n;++i){double w=MyaDSP::window(settings.window,i,n);x[(size_t)i]=pending[(size_t)i]*w;gain+=w;}MyaDSP::fft(x);std::vector<double> power((size_t)n/2+1);for(size_t k=0;k<power.size();++k){double a=2.*std::abs(x[k])/std::max(gain,1e-12);power[k]=a*a;}
  if(settings.average==Average::Fifo){fifo.push_back(power);while((int)fifo.size()>settings.depth)fifo.pop_front();std::fill(power.begin(),power.end(),0.);for(const auto& f:fifo)for(size_t k=0;k<power.size();++k)power[k]+=f[k];for(auto& v:power)v/=fifo.size();}
  else if(settings.average==Average::Exponential&&!current.db.empty()){double alpha=2./(std::max(1,settings.depth)+1.);for(size_t k=0;k<power.size();++k){double old=std::pow(10.,current.db[k]/10.);power[k]=old*(1-alpha)+power[k]*alpha;}}
  current.sampleRate=rate;current.db.resize(power.size());for(size_t k=0;k<power.size();++k)current.db[k]=10.*std::log10(std::max(power[k],1e-20));history.push_back(current);while(history.size()>512)history.pop_front();
 }
public:
 void configure(Settings s){s.fftSize=std::clamp(s.fftSize,1024,131072);if(!MyaDSP::powerOfTwo(s.fftSize))s.fftSize=MyaDSP::nextPowerOfTwo(s.fftSize);s.overlap=std::clamp(s.overlap,0.,.9375);s.depth=std::clamp(s.depth,1,128);s.framesPerSecond=std::clamp(s.framesPerSecond,1.,60.);settings=s;reset();}
 const Settings& configuration()const{return settings;}
 void reset(){pending.clear();fifo.clear();current={};history.clear();since=0;}
 void feed(const float* data,int count,double rate){if(!data||count<=0||rate<=0)return;int hop=std::max(1,(int)std::llround(rate/settings.framesPerSecond));for(int i=0;i<count;++i){pending.push_back(data[i]);++since;if((int)pending.size()>=settings.fftSize&&since>=hop){transform(rate);int keep=(int)std::llround(settings.fftSize*settings.overlap);pending.erase(pending.begin(),pending.end()-std::clamp(keep,0,settings.fftSize-1));since=0;}if(pending.size()>(size_t)settings.fftSize*2)pending.erase(pending.begin(),pending.begin()+settings.fftSize);}}
 bool ready()const{return !current.db.empty();}const Frame& frame()const{return current;}const std::deque<Frame>& spectrogram()const{return history;}
 std::optional<double> at(double hz)const{if(!ready()||hz<0||current.sampleRate<=0)return {};int k=(int)std::llround(hz*settings.fftSize/current.sampleRate);if(k<0||(size_t)k>=current.db.size())return {};return current.db[(size_t)k];}
};
}
