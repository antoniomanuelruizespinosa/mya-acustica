#include "../src/TransferFunctionCore.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
 TransferFunctionCore::Estimator estimator;
 constexpr double rate=48000.,frequency=1000.,gain=.5,phase=.4;
 int sample=0;
 for(int block=0;block<180;++block){
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
 for(int block=0;block<180;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){b.ref[(size_t)i]=(float)(1.e-5*std::sin(2*3.14159265358979323846*frequency*sample/rate));b.mic[(size_t)i]=(float)(.1*std::sin(2*3.14159265358979323846*frequency*sample/rate));}estimator.feed(b);}
 assert(!estimator.ready()&&!estimator.at(frequency));
 estimator.reset();sample=0;
 for(int block=0;block<180;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){double angle=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(angle));b.mic[(size_t)i]=(float)(.5*gain*std::sin(angle+phase));}estimator.feed(b);}
 assert(estimator.ready());
 // V clears the visible estimate and only fresh blocks may form a new one.
 estimator.reset();assert(!estimator.ready()&&!estimator.at(frequency));
 sample=0;for(int block=0;block<180;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){double angle=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(angle));b.mic[(size_t)i]=(float)(.5*gain*std::sin(angle+phase));}estimator.feed(b);}assert(estimator.ready());
 TransferFunctionCore::RawBlock gap;gap.sequence=1000;gap.sampleRate=rate;gap.count=512;estimator.feed(gap);assert(!estimator.ready());
 TransferFunctionCore::Queue<1> queue;assert(queue.push(gap));assert(!queue.push(gap));TransferFunctionCore::RawBlock out;assert(queue.pop(out)&&out.sequence==1000);assert(!queue.pop(out));
 std::cout<<"PASS TF H1 gain/phase/coherence, gap reset, bounded queue\n";
}
