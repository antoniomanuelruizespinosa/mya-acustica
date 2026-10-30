#pragma once
#include "InputLevelCore.h"
#include <limits>
namespace RmsCalibration {
constexpr double binSeconds=.5, stabilityDb=.5, silencePower=1.e-10;
constexpr int binCount=10;
inline std::optional<double> digital(const InputLevels::Peak& p){if(!p.valid||p.samples==0||!std::isfinite(p.sumSquares)||p.sumSquares<0)return {};if(p.sumSquares<=0)return -std::numeric_limits<double>::infinity();return 10.*std::log10(p.sumSquares/(double)p.samples);}
struct Session {
 enum class State{Idle,Measuring,Ready,Invalid,Cancelled,Confirmed};State state=State::Idle;
 std::string device,error;int channel=0;double rate=0,reference=94,minimum=INFINITY,maximum=-INFINITY,sum=0,bucketSum=0,lastTime=0;std::uint64_t samples=0,bucketSamples=0,lastSequence=0,watermark=0;int bins=0;std::optional<double> measured,offset;
 void fail(std::string why){if(state==State::Measuring||state==State::Ready){state=State::Invalid;error=std::move(why);offset.reset();}}
 bool start(std::string source,int ch,double sampleRate,double ref,std::uint64_t cursor){*this={};device=std::move(source);channel=ch;rate=sampleRate;reference=ref;watermark=lastSequence=cursor;if(device.empty()||ch<1||ch>InputLevels::maxChannels||!std::isfinite(rate)||rate<1||!std::isfinite(ref)||ref<0||ref>180){state=State::Invalid;error="Fuente, frecuencia o referencia invalida (0..180 dB)";return false;}state=State::Measuring;return true;}
 void sourceChanged(const std::string& key,int ch){if(key!=device||ch!=channel)fail("Fuente cambiada durante captura");}
 void cancel(){state=State::Cancelled;offset.reset();error="Cancelada; offset anterior conservado";}
 double progress()const{return state==State::Ready?1.:std::min(1.,(bins+(rate>0?(double)bucketSamples/(rate*binSeconds):0))/binCount);}
 void feed(const std::string& key,const InputLevels::Frame& frame,double sampleRate){
  if((state!=State::Measuring&&state!=State::Ready)||key!=device||frame.sequence<=watermark)return;
  if(!std::isfinite(frame.timeSeconds)||frame.timeSeconds<=0||(lastTime>0&&(frame.timeSeconds<=lastTime||frame.timeSeconds-lastTime>.25))){fail("Discontinuidad temporal (>250 ms o tiempo invalido)");return;}lastTime=frame.timeSeconds;
  if(!std::isfinite(sampleRate)||std::abs(sampleRate-rate)>1.e-9){fail("Cambio de frecuencia");return;}if(frame.sequence!=lastSequence+1){fail("Discontinuidad de bloques");return;}lastSequence=frame.sequence;
  const auto& p=frame.channels[(size_t)channel-1];auto db=digital(p);if(!db||p.clipped){fail(p.clipped?"Clipping digital; resultado rechazado":"Muestras ausentes/no finitas");return;}
  if(*db<=-100){fail("Silencio/nivel RMS <= -100 dBFS");return;}
  bucketSum+=p.sumSquares;bucketSamples+=p.samples;if((double)bucketSamples<rate*binSeconds)return;
  double power=bucketSum/(double)bucketSamples;if(power<=silencePower){fail("Silencio/nivel RMS <= -100 dBFS");return;}double level=10.*std::log10(power);minimum=std::min(minimum,level);maximum=std::max(maximum,level);if(maximum-minimum>stabilityDb){fail("Inestable: diferencia entre tramos > 0.5 dB");return;}
  if(state==State::Measuring){sum+=bucketSum;samples+=bucketSamples;++bins;if(bins==binCount){measured=10.*std::log10(sum/(double)samples);offset=reference-*measured;state=State::Ready;}}
  bucketSum=0;bucketSamples=0;
 }
 bool validatePending(){if(!confirmable())return false;if((double)bucketSamples>=rate*.05){double level=10.*std::log10(bucketSum/(double)bucketSamples);if(!std::isfinite(level)||std::max(maximum,level)-std::min(minimum,level)>stabilityDb)fail("Tramo pendiente inestable antes de confirmar");}return confirmable();}
 void confirmed(){if(confirmable())state=State::Confirmed;}
 bool confirmable()const{return state==State::Ready&&offset&&std::isfinite(*offset)&&measured&&std::isfinite(*measured);}
};
// Rectangular RMS over the last two complete >=0.5s bins; no Fast/Slow/weighting/Leq.
struct Rolling {
 double rate=0,sum=0,previous=0,lastTime=0;std::uint64_t count=0,previousCount=0,lastSequence=0;std::optional<double> level;bool clipped=false,bucketClip=false,previousClip=false;unsigned revision=0;
 bool hasClip()const{return clipped||bucketClip||previousClip;}
 void reset(){auto next=revision+1;*this={};revision=next;}
 void feed(const InputLevels::Peak& p,std::uint64_t sequence,double sampleRate,double timeSeconds=0){
  if((timeSeconds>0&&lastTime>0&&(timeSeconds<=lastTime||timeSeconds-lastTime>.25))||(lastSequence&&sequence!=lastSequence+1)||(rate>0&&std::abs(rate-sampleRate)>1.e-9)){reset();}lastSequence=sequence;rate=sampleRate;lastTime=timeSeconds;
  if(!digital(p)||!std::isfinite(sampleRate)||sampleRate<=0||!std::isfinite(timeSeconds)||timeSeconds<0){reset();return;}sum+=p.sumSquares;count+=p.samples;bucketClip=bucketClip||p.clipped;
  if((double)count<rate*binSeconds)return;
  if(previousCount){double mean=(sum+previous)/(double)(count+previousCount);level=mean>0?10.*std::log10(mean):-INFINITY;clipped=bucketClip||previousClip;++revision;}
  previous=sum;previousCount=count;previousClip=bucketClip;sum=0;count=0;bucketClip=false;
 }
};
}
