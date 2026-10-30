#pragma once
#include <algorithm>
#include <cmath>

class LoopSweep {
 double sampleRate=48000.,startHz=20.,endHz=20000.,durationSeconds=5.,position=0.;bool completed=false;
public:
 static constexpr float maximum=.01f;
 void configure(double rate,double start,double end,double duration){
  sampleRate=std::max(8000.,rate);startHz=std::clamp(start,1.,sampleRate*.45);endHz=std::clamp(end,startHz+1.,sampleRate*.45);durationSeconds=std::clamp(duration,.1,60.);position=0.;completed=false;
 }
 float next(){
  if(completed)return 0.f;
  const double total=std::max(1.,durationSeconds*sampleRate),u=position/total,ratio=endHz/startHz;
  const double phase=2.*3.14159265358979323846*startHz*durationSeconds*(std::pow(ratio,u)-1.)/std::log(ratio);
  const double fadeSamples=std::min(total*.05,sampleRate*.02),edge=std::min(position,total-position);
  const double fade=edge<fadeSamples?.5-.5*std::cos(3.14159265358979323846*std::max(0.,edge)/fadeSamples):1.;
  if(++position>=total)completed=true;
  return (float)(maximum*fade*std::sin(phase));
 }
 bool complete()const{return completed;}
};
