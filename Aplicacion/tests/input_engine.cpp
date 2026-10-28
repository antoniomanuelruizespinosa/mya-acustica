#include "../src/LiveInputEngine.h"
#include "../src/LoopCalibrationStore.h"
#include <cassert>
#include <iostream>
#include <thread>
static double nowMs=10000;
static double clockMs(){return nowMs;}
static const std::string key="CoreAudio / OCTA fixture";
static std::array<float,480> sine(){std::array<float,480> x;for(size_t i=0;i<x.size();++i)x[i]=(float)(.01*std::sin(juce::MathConstants<double>::twoPi*1000*i/48000));return x;}
int main(int argc,char** argv){
 assert(argc==2);
 FirstCallbackRetry retry;retry.arm(true);assert(!retry.consume("Callbacks interrumpidos >2000 ms")&&!retry.available);
 retry.arm(true);assert(retry.consume("Sin primer callback >2000 ms [CoreAudio / OCTA-CAPTURE]")&&!retry.consume("Sin primer callback >2000 ms"));
 retry.arm(true);retry.cancel();assert(!retry.consume("Sin primer callback >2000 ms"));
 auto x=sine();const float* input[]={x.data()};LiveInputEngine engine(clockMs);
 auto startChannels=[&](const std::vector<int>& channels){nowMs+=10000;engine.fixtureStart(juce::String(key),48000,channels);};
 auto start=[&]{startChannels({1});};
 auto callback=[&]{nowMs+=10;engine.fixtureCallback(input,1,480);};
 // Duplex callback: output is silent unless the explicit low-level pink test is armed.
 {
  std::array<float,480> out;out.fill(1.f);engine.fixtureStart(juce::String(key),48000,{1});engine.fixtureOutputCallback(input,1,out.data(),480);
  for(float sample:out)assert(sample==0.f);assert(!engine.outputDbfs(key));
  engine.fixtureStart(juce::String(key),48000,{1},true);engine.fixtureOutputCallback(input,1,out.data(),480);
  bool nonzero=false;for(float sample:out){assert(std::isfinite(sample)&&std::abs(sample)<=LoopPink::maximum);nonzero=nonzero||sample!=0.f;}assert(nonzero);assert(engine.outputDbfs(key)&&*engine.outputDbfs(key)<=-40.);
  std::array<float,480> right;right.fill(1.f);out.fill(1.f);engine.fixtureStereoOutputCallback(input,1,out.data(),right.data(),480);
  nonzero=false;for(float sample:out){assert(std::isfinite(sample)&&std::abs(sample)<=LoopPink::maximum);nonzero=nonzero||sample!=0.f;}assert(nonzero);
  for(float sample:right)assert(sample==0.f);
  engine.stop();assert(!engine.running()&&!engine.outputDbfs(key));
  engine.setSweep(20,20000,1.);engine.setSweepLevelDb(-40);engine.fixtureSignal(juce::String(key),48000,{1},LiveInputEngine::Signal::Sweep);out.fill(1.f);engine.fixtureOutputCallback(input,1,out.data(),480);nonzero=false;for(float sample:out){assert(std::isfinite(sample)&&std::abs(sample)<=LoopSweep::maximum);nonzero=nonzero||sample!=0.f;}assert(nonzero&&engine.outputDbfs(key));for(int block=1;block<100;++block)engine.fixtureOutputCallback(input,1,out.data(),480);assert(engine.sweepComplete());out.fill(1.f);engine.fixtureOutputCallback(input,1,out.data(),480);for(float sample:out)assert(sample==0.f);engine.stop();
 }
 // Single physical Input1: actual IO callback -> SPSC -> peak/RMS poll, no backend open.
 start();assert(engine.poll()&&!engine.rms(key,1));
 for(int i=0;i<100;++i){callback();assert(engine.poll());}
 assert(engine.channel(key,1).valid&&engine.rms(key,1));assert(std::abs(*engine.rms(key,1)+43.01029995664)<1.e-6);assert(engine.caughtUp(key));
 nowMs+=251;assert(!engine.channel(key,1).valid&&!engine.rms(key,1));
 // Exact B/R1 defect: UI does not poll for 3s, callbacks continue. Old poll checked
 // now-receivedAt before draining and falsely stopped. Heartbeat must keep it alive;
 // dropped/backlogged data must not become a fresh measurement.
 start();callback();assert(engine.poll());double previousUiPoll=nowMs;
 for(int i=0;i<300;++i)callback();assert(nowMs-previousUiPoll>2000);
 assert(engine.poll()&&engine.running()&&engine.drops()>0);std::cout<<"REGRESSION UI stall: old guard age="<<nowMs-previousUiPoll<<" ms (>2000), stream stays alive; "<<engine.details()<<"\n";assert(!engine.channel(key,1).valid&&!engine.rms(key,1));
 for(int i=0;i<100;++i){callback();assert(engine.poll());}assert(engine.rms(key,1)&&engine.channel(key,1).valid);
 // Same stall before the very first poll: active callbacks aren't a startup timeout.
 start();for(int i=0;i<300;++i)callback();assert(engine.poll()&&engine.running());assert(!engine.channel(key,1).valid);
 // But queued OLD frames with no recent callbacks cannot mask a real timeout.
 start();callback();nowMs+=2001;assert(!engine.poll()&&!engine.running());assert(engine.lastError().contains("Callbacks interrumpidos"));assert(engine.details().contains("callbacks=1")&&engine.details().contains("callbackAgeMs=2001"));
 start();nowMs+=1999;assert(engine.poll());nowMs+=2;assert(!engine.poll());assert(engine.lastError().contains("Sin primer callback"));assert(engine.details().contains("callbacks=0"));
 start();engine.fixtureStopped();assert(!engine.poll());assert(engine.lastError().contains("audioDeviceStopped")&&!engine.lastError().contains("Sin primer"));
 start();engine.fixtureBackendError("backend fixture disconnected 123");assert(!engine.poll());assert(engine.lastError().contains("Error notificado por backend"));assert(engine.details().contains("backend fixture disconnected 123"));
 start();engine.fixtureRestart();assert(!engine.poll());assert(engine.lastError().contains("Reinicio inesperado"));assert(engine.details().contains("starts=2"));
 start();engine.fixtureNoAboutToStart();assert(engine.poll()&&engine.running()&&engine.rate(key)==0.);nowMs+=2001;assert(!engine.poll());assert(engine.lastError().contains("No se recibio AboutToStart"));
 start();engine.fixtureNoAboutToStart();nowMs+=100;assert(engine.poll());engine.fixtureFinishStart();callback();assert(engine.poll()&&engine.channel(key,1).valid);
 start();engine.fixtureStarting();assert(engine.poll());nowMs+=2001;assert(!engine.poll()&&engine.lastError().contains("AboutToStart no completado"));
 // The observer can see a stop/error while draining; post-drain checks still reject.
 start();callback();assert(!engine.poll([&](const auto&,const auto&,double){engine.fixtureStopped();}));assert(engine.lastError().contains("audioDeviceStopped"));
 // Callback packing must map to physical Input5, not silently to Input1.
 startChannels({5});for(int i=0;i<100;++i){callback();assert(engine.poll());}assert(engine.rms(key,5)&&!engine.rms(key,1));
 // R3 global M1..4/R5 acquisition mask still yields five distinct physical inputs.
 startChannels({1,2,3,4,5});std::array<std::array<float,480>,5> channels;const float* five[5];
 for(size_t ch=0;ch<5;++ch){channels[ch]=x;for(auto& sample:channels[ch])sample*=(float)(ch+1);five[ch]=channels[ch].data();}
 for(int i=0;i<100;++i){nowMs+=10;engine.fixtureCallback(five,5,480);assert(engine.poll());}
 for(int ch=1;ch<=5;++ch){assert(engine.channel(key,ch).valid&&engine.rms(key,ch));assert(std::abs(*engine.rms(key,ch)-(-43.01029995664+20*std::log10(ch)))<1.e-6);}
 // M/R raw capture uses physical Input1 and Input5, not packed positions 1 and 5.
 engine.fixtureStart(juce::String(key),48000,{1,2,3,4,5},false,1,5);
 nowMs+=10;engine.fixtureCallback(five,5,480);
 bool captured=false;
 assert(engine.poll({},[&](const std::string& observed,const TransferFunctionCore::RawBlock& b){assert(observed==key&&b.count==480&&b.sampleRate==48000);assert(b.mic[0]==channels[0][0]&&b.ref[1]==channels[4][1]);captured=true;}));
 assert(captured);
 engine.setReferenceGainDb(6.);nowMs+=10;engine.fixtureCallback(five,5,480);double trim=std::pow(10.,6./20.);bool trimmed=false;
 assert(engine.poll({},[&](const std::string&,const TransferFunctionCore::RawBlock& b){assert(std::abs(b.ref[1]-channels[4][1]*trim)<1e-6);trimmed=true;}));assert(trimmed);engine.setReferenceGainDb(0.);
 // V discards queued raw samples while keeping the same live input session.
 nowMs+=10;engine.fixtureCallback(five,5,480);
 engine.discardPendingTF();assert(engine.running());
 int afterReset=0;assert(engine.poll({},[&](const std::string&,const TransferFunctionCore::RawBlock&){++afterReset;}));assert(afterReset==0);
 nowMs+=10;engine.fixtureCallback(five,5,480);
 assert(engine.poll({},[&](const std::string&,const TransferFunctionCore::RawBlock& b){assert(b.count==480);++afterReset;}));assert(afterReset==1);
 // Internal loop takes the exact generated samples as R, without a physical reference input.
 engine.fixtureStart(juce::String(key),48000,{1},true,1,0,true);
 std::array<float,480> generated{};nowMs+=10;engine.fixtureOutputCallback(input,1,generated.data(),480);
 bool virtualCaptured=false;assert(engine.poll({},[&](const std::string& observed,const TransferFunctionCore::RawBlock& b){assert(observed==key&&b.count==480);for(int n=0;n<b.count;++n){assert(b.mic[(size_t)n]==x[(size_t)n]);assert(b.ref[(size_t)n]==generated[(size_t)n]);}virtualCaptured=true;}));assert(virtualCaptured);
 engine.fixtureStart(juce::String(key),48000,{1},false,1,0,true);generated.fill(1.f);nowMs+=10;engine.fixtureOutputCallback(input,1,generated.data(),480);for(float sample:generated)assert(sample==0.f);assert(!engine.outputDbfs(key));int silentRaw=0;assert(engine.poll({},[&](const std::string&,const TransferFunctionCore::RawBlock&){++silentRaw;}));assert(silentRaw==0);
 // Physical calibration captures the configured output and Input 3 in paired duplex callbacks.
 engine.fixtureStart(juce::String(key),48000,{3},true,0,3);
 assert(engine.startLoopCalibration(key));
 std::array<float,480> returned{},prior{};const float* loopInput[]={returned.data()};
 nowMs+=10;engine.fixtureOutputCallback(loopInput,1,generated.data(),480);prior=generated;
 returned=prior;nowMs+=10;engine.fixtureOutputCallback(loopInput,1,generated.data(),480);
 int loopBlocks=0;engine.drainLoopCalibration(key,[&](const TransferFunctionCore::RawBlock& b){assert(b.count==480&&b.sampleRate==48000);if(++loopBlocks==2)for(int i=0;i<480;++i)assert(b.mic[(size_t)i]==prior[(size_t)i]&&b.ref[(size_t)i]==generated[(size_t)i]);});
 assert(loopBlocks==2&&!engine.loopCalibrationOverflow(key));engine.cancelLoopCalibration();assert(!engine.loopCalibrationActive(key));
 assert(engine.startLoopCalibration(key));for(int i=0;i<300&&!engine.loopCalibrationOverflow(key);++i){nowMs+=10;engine.fixtureOutputCallback(loopInput,1,generated.data(),480);}assert(engine.loopCalibrationOverflow(key)&&!engine.loopCalibrationActive(key));
 engine.fixtureStart(juce::String(key),48000,{3},true,0,5);assert(!engine.startLoopCalibration(key));
 engine.stop();
 // Persistence binds the electronic compensation to device, format and selected output/reference channels.
 auto temp=juce::File(juce::String::fromUTF8(argv[1])).getChildFile("mya-loop-calibration.json");
 LoopCalibrationRecord first;first.deviceKey=juce::String(key);first.sampleRate=48000;first.bufferSamples=512;first.outputChannel=2;first.inputChannel=3;first.result={6.6,4.2,.95,-40,-44.2,60000};
 LoopCalibrationRecord second=first;second.outputChannel=4;second.inputChannel=7;second.result.delayMs=12.3;second.result.gainDb=7.1;
 LoopCalibrationRecord alternateFormat=first;alternateFormat.sampleRate=44100;alternateFormat.bufferSamples=256;alternateFormat.result.delayMs=9.8;
 {LoopCalibrationStore store(temp);assert(store.size()==0);assert(store.set(first));assert(store.set(second));assert(store.set(alternateFormat));LoopCalibrationRecord recalibrated=first;recalibrated.result.gainDb=5.1;assert(store.set(recalibrated));assert(store.size()==3);}
 {LoopCalibrationStore store(temp);assert(store.size()==3);auto a=store.find(juce::String(key),48000,512,2,3),b=store.find(juce::String(key),48000,512,4,7),c=store.find(juce::String(key),44100,256,2,3);assert(a&&b&&c&&a->result.delayMs==6.6&&a->result.gainDb==5.1&&b->result.delayMs==12.3&&c->result.delayMs==9.8);assert(!store.find(juce::String(key),44100,512,2,3));assert(!store.find(juce::String(key),48000,256,2,3));assert(!store.find("other device",48000,512,2,3));assert(!store.find(juce::String(key),48000,512,1,3));assert(!store.find(juce::String(key),48000,512,2,4));assert(store.clear(*a));assert(store.size()==2&&temp.existsAsFile());}
 {LoopCalibrationStore store(temp);assert(!store.find(juce::String(key),48000,512,2,3));auto c=store.find(juce::String(key),44100,256,2,3);assert(c&&store.find(juce::String(key),48000,512,4,7));assert(store.clear(*c));assert(store.size()==1&&temp.existsAsFile());}
 {LoopCalibrationStore store(temp);auto b=store.find(juce::String(key),48000,512,4,7);assert(b&&store.clear(*b));assert(store.size()==0&&!temp.existsAsFile());}
 // A calibration written by the previous single-record format survives the upgrade.
 auto legacy=temp.getSiblingFile("mya-loop-calibration-legacy.json");
 assert(legacy.replaceWithText(R"({"schema":2,"deviceKey":"CoreAudio / OCTA fixture","sampleRate":48000,"bufferSamples":512,"outputChannel":2,"inputChannel":3,"delayMs":6.6,"gainDb":4.2,"correlation":0.95,"outputDbfs":-40,"inputDbfs":-44.2})"));
 {LoopCalibrationStore store(legacy);assert(store.size()==1&&store.find(juce::String(key),48000,512,2,3));assert(store.set(second));}
 {LoopCalibrationStore store(legacy);assert(store.size()==2);auto a=store.find(juce::String(key),48000,512,2,3),b=store.find(juce::String(key),48000,512,4,7);assert(a&&b);auto secondSaved=*b;assert(store.clear(*a));assert(store.clear(secondSaved));assert(!legacy.existsAsFile());}
 // Invalid callback input count isn't "no callbacks". It produces no readings.
 start();nowMs+=10;engine.fixtureCallback(nullptr,0,480);assert(engine.poll()&&engine.running());assert(!engine.channel(key,1).valid&&!engine.rms(key,1));assert(engine.details().contains("callbacks=1")&&engine.details().contains("callbackInputs=0"));
 // Real end-to-end calibration from callback frames, no physical audio, no persistence.
 start();for(int i=0;i<100;++i){callback();assert(engine.poll());}
 RmsCalibration::Session capture;assert(capture.start(key,1,48000,94,engine.cursor(key)));
 for(int i=0;i<500;++i){callback();assert(engine.poll([&](const auto& k,const auto& f,double rate){capture.feed(k,f,rate);}));}
 assert(capture.confirmable()&&std::abs(*capture.measured+43.01029995664)<1.e-6);
 // The reported format 48000/512, continuous sine phase and actual callback timing.
 start();std::array<float,512> block;const float* actualInput[]={block.data()};int index=0;
 auto actualCallback=[&]{for(auto& value:block){value=(float)(.01*std::sin(juce::MathConstants<double>::twoPi*1000*index/48000));++index;}nowMs+=512000./48000.;engine.fixtureCallback(actualInput,1,512);};
 for(int i=0;i<100;++i){actualCallback();assert(engine.poll());}assert(engine.rms(key,1));
 assert(capture.start(key,1,48000,94,engine.cursor(key)));
 for(int i=0;i<470;++i){actualCallback();assert(engine.poll([&](const auto& k,const auto& f,double rate){capture.feed(k,f,rate);}));}
 assert(capture.confirmable()&&capture.samples==240640&&std::abs(*capture.measured+43.01029995664)<.001);
 engine.stop();assert(!engine.running()&&!engine.rms(key,1));
 // Concurrent producer/UI polling with the real monotonic clock, still no device.
 {LiveInputEngine live;live.fixtureStart("Core / concurrent fixture",48000,{1});std::atomic<bool> done{false};std::thread producer([&]{for(int i=0;i<2000;++i)live.fixtureCallback(input,1,480);done.store(true,std::memory_order_release);});
  while(!done.load(std::memory_order_acquire)){assert(live.poll());std::this_thread::yield();}producer.join();assert(live.poll()&&live.running()&&live.lastError().isEmpty());live.fixtureCallback(input,1,480);assert(live.poll());live.fixtureCallback(input,1,480);assert(live.poll()&&live.channel("Core / concurrent fixture",1).valid);live.stop();}
 std::cout<<"PASS actual callback/queue/poll: single input peak+RMS, pre/post first-poll UI stalls with continuing callbacks, stale data rejected, real timeouts distinct, stop/error/restart/AboutToStart diagnostics, physical Input5 mapping, invalid input count, five-input M/R regression, 5s calibration via engine and 48000/512 continuous tone, concurrent callback producer/UI poll. No device created or opened.\n";
}
