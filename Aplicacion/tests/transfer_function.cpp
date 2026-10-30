#include "../src/TransferFunctionCore.h"
#include "../src/RTAEstimator.h"
#include "../src/SweepMeasurementCore.h"
#include "../src/LoopSweep.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
 TransferFunctionCore::Estimator estimator;
 constexpr double rate=48000.,frequency=1000.,gain=.5,phase=.4;
 int sample=0;
 for(int block=0;block<320;++block){
  TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;
  for(int i=0;i<b.count;++i,++sample){double angle=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(angle));b.mic[(size_t)i]=(float)(.5*gain*std::sin(angle+phase));}
  estimator.feed(b);
 }
 assert(estimator.ready());auto point=estimator.at(frequency);assert(point);
 assert(std::abs(point->magnitudeDb-20*std::log10(gain))<.2);
 assert(std::abs(point->phaseDegrees-phase*180/3.14159265358979323846)<2.);
 assert(point->coherence>.99);
 // Input noise without a usable reference must not appear as a valid TF.
 estimator.reset();sample=0;
 for(int block=0;block<320;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){b.ref[(size_t)i]=(float)(1.e-5*std::sin(2*3.14159265358979323846*frequency*sample/rate));b.mic[(size_t)i]=(float)(.1*std::sin(2*3.14159265358979323846*frequency*sample/rate));}estimator.feed(b);}
 assert(!estimator.ready()&&!estimator.at(frequency));
 estimator.reset();sample=0;
 for(int block=0;block<320;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){double angle=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(angle));b.mic[(size_t)i]=(float)(.5*gain*std::sin(angle+phase));}estimator.feed(b);}
 assert(estimator.ready());
 // V clears the visible estimate and only fresh blocks may form a new one.
 estimator.reset();assert(!estimator.ready()&&!estimator.at(frequency));
 sample=0;for(int block=0;block<320;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){double angle=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(angle));b.mic[(size_t)i]=(float)(.5*gain*std::sin(angle+phase));}estimator.feed(b);}assert(estimator.ready());
 // A delayed broadband response yields the same delay in Find and a centered measured impulse.
 estimator.reset();constexpr int delayedSamples=5472;std::array<float,delayedSamples> history{};int position=0;std::uint32_t seed=1;
 for(int block=0;block<480;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i){seed=seed*1664525u+1013904223u;float reference=(float)((int)(seed>>8)%65536-32768)/131072.f;b.ref[(size_t)i]=reference;b.mic[(size_t)i]=.5f*history[(size_t)position];history[(size_t)position]=reference;position=(position+1)%delayedSamples;}estimator.feed(b);}
 assert(estimator.ready());auto found=estimator.findDelayMs();assert(found&&std::abs(*found-114.)<.1);estimator.setDelayMs(*found);auto impulse=estimator.impulseAt(0.);assert(impulse&&*impulse>.2);auto side=estimator.impulseAt(10.);assert(side&&std::abs(*side)<.2);
 for(int block=480;block<960;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i){seed=seed*1664525u+1013904223u;float reference=(float)((int)(seed>>8)%65536-32768)/131072.f;b.ref[(size_t)i]=reference;b.mic[(size_t)i]=.5f*history[(size_t)position];history[(size_t)position]=reference;position=(position+1)%delayedSamples;}estimator.feed(b);}auto aligned=estimator.at(1000.);assert(aligned&&aligned->coherence>.8&&std::abs(aligned->magnitudeDb+6.02)<1.);
 TransferFunctionCore::RawBlock gap;gap.sequence=1000;gap.sampleRate=rate;gap.count=512;estimator.feed(gap);assert(!estimator.ready());
 TransferFunctionCore::Queue<1> queue;assert(queue.push(gap));assert(!queue.push(gap));TransferFunctionCore::RawBlock out;assert(queue.pop(out)&&out.sequence==1000);assert(!queue.pop(out));
 // The selected temporal averaging depth changes when a TF becomes display-ready.
 estimator.setAverageDepth(2);assert(estimator.averaging()==2&&!estimator.ready());sample=0;for(int block=0;block<64;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){double a=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(a));b.mic[(size_t)i]=(float)(.25*std::sin(a+phase));}estimator.feed(b);}assert(estimator.ready());auto smooth=estimator.smoothedAt(frequency,6.,false);assert(smooth&&estimator.at(frequency)&&smooth->magnitudeDb<estimator.at(frequency)->magnitudeDb-10.);
 // An RTA is a single-channel spectrum; a full-scale-normalized 0.5 sine reads -6 dBFS.
 RTAEstimator rta;sample=0;for(int block=0;block<16;++block){TransferFunctionCore::RawBlock b;b.count=512;b.sampleRate=rate;for(int i=0;i<b.count;++i,++sample)b.mic[(size_t)i]=(float)(.5*std::sin(2*3.14159265358979323846*frequency*sample/rate));rta.feed(b);}auto rtaLevel=rta.dbfs(frequency);assert(rtaLevel&&std::abs(*rtaLevel+6.02)<1.);
 // A finite sweep with a delayed copy yields measured magnitude and impulse timing.
 LoopSweep sweep;sweep.configure(rate,20.,20000.,5.);std::vector<float> sweepRef(5*48000+4800),sweepMic(sweepRef.size());for(size_t i=0;i<sweepRef.size();++i){sweepRef[i]=sweep.next();if(i>=100)sweepMic[i]=.5f*sweepRef[i-100];}auto sweepResult=SweepMeasurementCore::analyze(sweepMic,sweepRef,rate,20.,20000.);assert(sweepResult);estimator.loadSweepSpectrum(sweepResult->transfer,sweepResult->valid,rate);assert(estimator.ready()&&estimator.isSweep());auto sweepPoint=estimator.at(frequency);assert(sweepPoint&&std::abs(sweepPoint->magnitudeDb+6.02)<1.);auto sweepDelay=estimator.findDelayMs();assert(sweepDelay&&std::abs(*sweepDelay-1000.*100./rate)<.2);estimator.setDelayMs(*sweepDelay);auto sweepImpulse=estimator.impulseAt(0.);assert(sweepImpulse&&*sweepImpulse>.2);
 std::cout<<"PASS TF H1 gain/phase/coherence, gap reset, bounded queue\n";
}
