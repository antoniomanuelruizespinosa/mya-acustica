#include "../src/TransferFunctionCore.h"
#include "../src/RTAEstimator.h"
#include "../src/SweepMeasurementCore.h"
#include "../src/LoopSweep.h"
#include "../src/CoherenceBlanking.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
 assert(CoherenceBlanking::passes(.49,0.,true));
 assert(!CoherenceBlanking::passes(.49,50.,true));
 assert(CoherenceBlanking::passes(.49,70.,false));
 assert(!CoherenceBlanking::passes(.49,70.1,false));
 assert(CoherenceBlanking::visuallyQuiet(.08,false)&&!CoherenceBlanking::visuallyQuiet(.25,false));
 assert(CoherenceBlanking::visuallyQuiet(.25,true)&&!CoherenceBlanking::visuallyQuiet(.5,true));
 assert(!CoherenceBlanking::passesVisual(.08,0.,false));
 assert(CoherenceBlanking::passesVisual(.25,0.,false));
 assert(!CoherenceBlanking::passesVisual(.25,60.,false));
 auto estimatorStorage=std::make_unique<TransferFunctionCore::Estimator>();auto& estimator=*estimatorStorage;
 auto retainedRtaStorage=std::make_unique<RTAEstimator>();auto& retainedRta=*retainedRtaStorage;
 constexpr double rate=48000.,frequency=1000.,gain=.5,phase=.4;
 int sample=0;
 for(int block=0;block<320;++block){
  TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;
  for(int i=0;i<b.count;++i,++sample){double angle=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(angle));b.mic[(size_t)i]=(float)(.5*gain*std::sin(angle+phase));}
  estimator.feed(b);retainedRta.feed(b);
 }
 assert(estimator.ready());auto point=estimator.at(frequency);assert(point);
 assert(std::abs(point->magnitudeDb-20*std::log10(gain))<.2);
 estimator.clearAveragePreservingHistory();assert(estimator.ready());auto refreshed=estimator.at(frequency);assert(refreshed&&std::abs(refreshed->magnitudeDb-20*std::log10(gain))<.2);
 assert(retainedRta.ready()&&retainedRta.dbfs(frequency));retainedRta.clearAveragePreservingHistory();assert(retainedRta.ready()&&retainedRta.dbfs(frequency));
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
 assert(estimator.ready());auto found=estimator.findDelayMs();assert(found&&std::abs(*found-114.)<.1);auto trackCopy=estimator.delaySnapshot();assert(trackCopy&&trackCopy->fftSize==estimator.fftSizeSamples());auto tracked=TransferFunctionCore::Estimator::findDelayMs(*trackCopy);assert(tracked&&std::abs(*tracked-*found)<1.e-9);estimator.setDelayMs(*found);auto impulse=estimator.impulseAt(0.);assert(impulse&&*impulse>.2);auto side=estimator.impulseAt(10.);assert(side&&std::abs(*side)<.2);
 estimator.setDelayMs(0.);assert(estimator.delay()==0.);auto unaligned=estimator.impulseAt(0.);assert(unaligned&&std::abs(*unaligned)<.2);estimator.setDelayMs(*found);assert(std::abs(estimator.delay()-*found)<1.e-9);
 for(int block=480;block<960;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i){seed=seed*1664525u+1013904223u;float reference=(float)((int)(seed>>8)%65536-32768)/131072.f;b.ref[(size_t)i]=reference;b.mic[(size_t)i]=.5f*history[(size_t)position];history[(size_t)position]=reference;position=(position+1)%delayedSamples;}estimator.feed(b);}auto aligned=estimator.at(1000.);assert(aligned&&aligned->coherence>.8&&std::abs(aligned->magnitudeDb+6.02)<1.);
 TransferFunctionCore::RawBlock gap;gap.sequence=1000;gap.sampleRate=rate;gap.count=512;estimator.feed(gap);assert(!estimator.ready());
 TransferFunctionCore::Queue<1> queue;assert(queue.push(gap));assert(!queue.push(gap));TransferFunctionCore::RawBlock out;assert(queue.pop(out)&&out.sequence==1000);assert(!queue.pop(out));
 // The selected temporal averaging depth changes when a TF becomes display-ready.
 estimator.setAverageDepth(2);assert(estimator.averaging()==2&&!estimator.ready());sample=0;for(int block=0;block<64;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.sampleRate=rate;b.count=512;for(int i=0;i<b.count;++i,++sample){double a=2*3.14159265358979323846*frequency*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(a));b.mic[(size_t)i]=(float)(.25*std::sin(a+phase));}estimator.feed(b);}assert(estimator.ready());auto smooth=estimator.smoothedAt(frequency,6.,false);assert(smooth&&estimator.at(frequency)&&smooth->magnitudeDb<estimator.at(frequency)->magnitudeDb-10.);
 // High-frequency 1/12-oct smoothing averages power with logarithmic bin weights.
 auto smoothingProbe=std::make_unique<TransferFunctionCore::Estimator>();std::vector<std::complex<double>> spectrum(TransferFunctionCore::bins,{1.,0.});std::vector<bool> valid(TransferFunctionCore::bins,true);
 constexpr double probeHz=10000.;const int peakBin=(int)std::round(probeHz*TransferFunctionCore::fftSize/rate);spectrum[(size_t)peakBin]={10.,0.};smoothingProbe->loadSweepSpectrum(spectrum,valid,rate);
 double bandRatio=std::pow(2.,.5/12.),expectedPower=0.,bandWeight=0.,binWidth=rate/TransferFunctionCore::fftSize,lower=probeHz/bandRatio,upper=probeHz*bandRatio;
 int firstBin=(int)std::floor(lower/binWidth+.5),lastBin=(int)std::ceil(upper/binWidth-.5);
 for(int k=firstBin;k<=lastBin;++k){double a=std::max(lower,(k-.5)*binWidth),b=std::min(upper,(k+.5)*binWidth);double weight=b>a?std::log(b/a):0.;expectedPower+=(k==peakBin?100.:1.)*weight;bandWeight+=weight;}
 auto smoothedPeak=smoothingProbe->smoothedAt(probeHz,12.,false);assert(smoothedPeak&&std::abs(smoothedPeak->magnitudeDb-10.*std::log10(expectedPower/bandWeight))<1.e-8);
 // Partial edge bins must move continuously as the 1/6-octave band is swept.
 auto atLeft=smoothingProbe->smoothedAt(probeHz-1.,6.,false),atRight=smoothingProbe->smoothedAt(probeHz+1.,6.,false);
 assert(atLeft&&atRight&&std::abs(atLeft->magnitudeDb-atRight->magnitudeDb)<.1);
 assert(smoothingProbe->impulseAt(80.).has_value()&&!smoothingProbe->impulseAt(86.));
 // An RTA is a single-channel spectrum; a full-scale-normalized 0.5 sine reads -6 dBFS.
 auto rtaStorage=std::make_unique<RTAEstimator>();auto& rta=*rtaStorage;sample=0;for(int block=0;block<32;++block){TransferFunctionCore::RawBlock b;b.count=512;b.sampleRate=rate;for(int i=0;i<b.count;++i,++sample)b.mic[(size_t)i]=(float)(.5*std::sin(2*3.14159265358979323846*frequency*sample/rate));rta.feed(b);}auto rtaLevel=rta.dbfs(frequency);assert(rtaLevel&&std::abs(*rtaLevel+6.02)<1.);
 // A 1/3-octave band sums linear FFT-bin power; a two-window average uses exactly two powers.
 auto bandRtaStorage=std::make_unique<RTAEstimator>();auto& bandRta=*bandRtaStorage;bandRta.setMultiWindowEnabled(false);bandRta.setFFTSize(512);bandRta.setTemporalAveraging(MultiWindowSpectrum::TemporalMode::Linear,2);
 for(int frame=0;frame<2;++frame){TransferFunctionCore::RawBlock b;b.count=512;b.sampleRate=rate;double amplitude=frame==0?.5:.25;for(int i=0;i<b.count;++i)b.mic[(size_t)i]=(float)(amplitude*std::sin(2.*3.14159265358979323846*1500.*(frame*512+i)/rate));bandRta.feed(b);}
 auto twoFrames=bandRta.dbfs(1500.);assert(twoFrames&&std::abs(*twoFrames-10.*std::log10((.25+.0625)/2.))<.25);
 auto bandLevel=bandRta.smoothedDbfs(1500.,3.);assert(bandLevel&&std::abs(*bandLevel-*twoFrames)<2.);
 auto longAverageStorage=std::make_unique<RTAEstimator>();auto& longAverage=*longAverageStorage;longAverage.setMultiWindowEnabled(false);longAverage.setFFTSize(512);longAverage.setTemporalAveraging(MultiWindowSpectrum::TemporalMode::Linear,64);for(int frame=0;frame<64;++frame){TransferFunctionCore::RawBlock b;b.count=512;b.sampleRate=rate;for(int i=0;i<b.count;++i)b.mic[(size_t)i]=frame==0?(float)(.5*std::sin(2.*3.14159265358979323846*1500.*i/rate)):0.f;longAverage.feed(b);}auto sixtyFour=longAverage.dbfs(1500.);assert(sixtyFour&&std::abs(*sixtyFour-(20.*std::log10(.5)-10.*std::log10(64.)))<.25);
 // Fixed FFT choices must alter the actual bin spacing and discard the old average.
 estimator.setMultiWindowEnabled(false);rta.setMultiWindowEnabled(false);estimator.setAverageDepth(2);
 for(int fftSize:{128,32768}){
  estimator.setFFTSize(fftSize);rta.setFFTSize(fftSize);
  assert(!estimator.ready()&&!rta.ready());
  sample=0;int blocks=fftSize==128?4:128;
  for(int block=0;block<blocks;++block){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)block+1;b.count=512;b.sampleRate=rate;for(int i=0;i<b.count;++i,++sample){double a=2*3.14159265358979323846*1500.*sample/rate;b.ref[(size_t)i]=(float)(.5*std::sin(a));b.mic[(size_t)i]=(float)(.25*std::sin(a));}estimator.feed(b);rta.feed(b);}
  assert(estimator.ready()&&rta.ready());auto tf=estimator.at(1500.);auto spectrum=rta.dbfs(1500.);assert(tf&&std::abs(tf->magnitudeDb+6.02)<.5);assert(spectrum&&std::abs(*spectrum+12.04)<1.);
  assert(estimator.fftSizeSamples()==fftSize&&rta.fftSize()==fftSize);
 }
 estimator.setFFTSize(16384);estimator.setMultiWindowEnabled(true);
 // TF modes must apply distinct complex cross-spectrum weights, including a running mean.
 auto averagedGain=[&](MultiWindowSpectrum::TfAverageMode mode){auto probe=std::make_unique<TransferFunctionCore::Estimator>();probe->setFFTSize(512);probe->setMultiWindowEnabled(false);probe->setAverageMode(mode);for(int frame=0;frame<2;++frame){TransferFunctionCore::RawBlock b;b.sequence=(std::uint64_t)frame+1;b.count=512;b.sampleRate=rate;for(int i=0;i<512;++i){double v=.5*std::sin(2.*3.14159265358979323846*1500.*(frame*512+i)/rate);b.ref[(size_t)i]=(float)v;b.mic[(size_t)i]=(float)(v*(frame==0?1.:.2));}probe->feed(b);}auto value=probe->at(1500.);assert(value&&probe->ready());return value->magnitudeDb;};
 double fastGain=averagedGain(MultiWindowSpectrum::TfAverageMode::Fast),slowGain=averagedGain(MultiWindowSpectrum::TfAverageMode::Slow),infiniteGain=averagedGain(MultiWindowSpectrum::TfAverageMode::Infinite);assert(slowGain>fastGain&&fastGain>infiniteGain);assert(std::abs(infiniteGain-20.*std::log10(.6))<.3);
 // The decimated low-frequency window resolves a 31.25 Hz tone and preserves M/R phase.
 auto low=std::make_unique<MultiWindowSpectrum>();constexpr double lowHz=31.25;
 for(int i=0;i<288000;++i){double a=2.*3.14159265358979323846*lowHz*i/rate;low->feed(.25*std::sin(a+phase),.5*std::sin(a),rate);}
 auto lowRta=low->rtaDbfs(lowHz);auto lowTF=low->transferAt(lowHz);
 assert(lowRta&&std::abs(*lowRta+12.04)<1.);
 assert(lowTF&&std::abs(lowTF->magnitudeDb-20.*std::log10(gain))<.5);
 assert(lowTF->coherence>.98&&std::abs(lowTF->phaseDegrees-phase*180./3.14159265358979323846)<3.);
 assert(MultiWindowSpectrum::gridCount()==639);
 // A finite sweep with a delayed copy yields measured magnitude and impulse timing.
 LoopSweep sweep;sweep.configure(rate,20.,20000.,5.);std::vector<float> sweepRef(5*48000+4800),sweepMic(sweepRef.size());for(size_t i=0;i<sweepRef.size();++i){sweepRef[i]=sweep.next();if(i>=100)sweepMic[i]=.5f*sweepRef[i-100];}auto sweepResult=SweepMeasurementCore::analyze(sweepMic,sweepRef,rate,20.,20000.);assert(sweepResult&&sweepResult->impulse.size()>=sweepMic.size());auto synced=SweepMeasurementCore::analyze(sweepMic,sweepRef,rate,20.,20000.,16384,true);assert(synced&&std::abs(synced->syncDelayMs-1000.*100./rate)<.2&&synced->impulse.size()==sweepResult->impulse.size());estimator.loadSweepSpectrum(sweepResult->transfer,sweepResult->valid,rate);assert(estimator.ready()&&estimator.isSweep());auto sweepPoint=estimator.at(frequency);assert(sweepPoint&&std::abs(sweepPoint->magnitudeDb+6.02)<1.);auto sweepDelay=estimator.findDelayMs();assert(sweepDelay&&std::abs(*sweepDelay-1000.*100./rate)<.2);estimator.setDelayMs(*sweepDelay);auto sweepImpulse=estimator.impulseAt(0.);assert(sweepImpulse&&*sweepImpulse>.2);
 auto sweepWide=SweepMeasurementCore::analyze(sweepMic,sweepRef,rate,20.,20000.,32768);assert(sweepWide&&sweepWide->transfer.size()==16385);estimator.setFFTSize(32768);estimator.loadSweepSpectrum(sweepWide->transfer,sweepWide->valid,rate);auto widePoint=estimator.at(frequency);assert(widePoint&&std::abs(widePoint->magnitudeDb+6.02)<1.);
 std::cout<<"PASS TF H1 gain/phase/coherence, gap reset, bounded queue\n";
}
