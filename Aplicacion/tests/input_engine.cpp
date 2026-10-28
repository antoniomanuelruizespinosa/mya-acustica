#include "../src/LiveInputEngine.h"
#include <cassert>
#include <iostream>
#include <thread>
static double nowMs=10000;
static double clockMs(){return nowMs;}
static const std::string key="CoreAudio / OCTA fixture";
static std::array<float,480> sine(){std::array<float,480> x;for(size_t i=0;i<x.size();++i)x[i]=(float)(.01*std::sin(juce::MathConstants<double>::twoPi*1000*i/48000));return x;}
int main(){
 auto x=sine();const float* input[]={x.data()};LiveInputEngine engine(clockMs);
 auto start=[&](std::vector<int> channels){nowMs+=10000;engine.fixtureStart(juce::String(key),48000,channels);};
 auto callback=[&]{nowMs+=10;engine.fixtureCallback(input,1,480);};
 // Single physical Input1: actual IO callback -> SPSC -> peak/RMS poll, no backend open.
 start({1});assert(engine.poll()&&!engine.rms(key,1));
 for(int i=0;i<100;++i){callback();assert(engine.poll());}
 assert(engine.channel(key,1).valid&&engine.rms(key,1));assert(std::abs(*engine.rms(key,1)+43.01029995664)<1.e-6);assert(engine.caughtUp(key));
 nowMs+=251;assert(!engine.channel(key,1).valid&&!engine.rms(key,1));
 // Exact B/R1 defect: UI does not poll for 3s, callbacks continue. Old poll checked
 // now-receivedAt before draining and falsely stopped. Heartbeat must keep it alive;
 // dropped/backlogged data must not become a fresh measurement.
 start({1});callback();assert(engine.poll());double previousUiPoll=nowMs;
 for(int i=0;i<300;++i)callback();assert(nowMs-previousUiPoll>2000);
 assert(engine.poll()&&engine.running()&&engine.drops()>0);std::cout<<"REGRESSION UI stall: old guard age="<<nowMs-previousUiPoll<<" ms (>2000), stream stays alive; "<<engine.details()<<"\n";assert(!engine.channel(key,1).valid&&!engine.rms(key,1));
 for(int i=0;i<100;++i){callback();assert(engine.poll());}assert(engine.rms(key,1)&&engine.channel(key,1).valid);
 // Same stall before the very first poll: active callbacks aren't a startup timeout.
 start({1});for(int i=0;i<300;++i)callback();assert(engine.poll()&&engine.running());assert(!engine.channel(key,1).valid);
 // But queued OLD frames with no recent callbacks cannot mask a real timeout.
 start({1});callback();nowMs+=2001;assert(!engine.poll()&&!engine.running());assert(engine.lastError().contains("Callbacks interrumpidos"));assert(engine.details().contains("callbacks=1")&&engine.details().contains("callbackAgeMs=2001"));
 start({1});nowMs+=1999;assert(engine.poll());nowMs+=2;assert(!engine.poll());assert(engine.lastError().contains("Sin primer callback"));assert(engine.details().contains("callbacks=0"));
 start({1});engine.fixtureStopped();assert(!engine.poll());assert(engine.lastError().contains("audioDeviceStopped")&&!engine.lastError().contains("Sin primer"));
 start({1});engine.fixtureBackendError("backend fixture disconnected 123");assert(!engine.poll());assert(engine.lastError().contains("Error notificado por backend"));assert(engine.details().contains("backend fixture disconnected 123"));
 start({1});engine.fixtureRestart();assert(!engine.poll());assert(engine.lastError().contains("Reinicio inesperado"));assert(engine.details().contains("starts=2"));
 start({1});engine.fixtureNoAboutToStart();assert(engine.poll()&&engine.running()&&engine.rate(key)==0.);nowMs+=2001;assert(!engine.poll());assert(engine.lastError().contains("No se recibio AboutToStart"));
 start({1});engine.fixtureNoAboutToStart();nowMs+=100;assert(engine.poll());engine.fixtureFinishStart();callback();assert(engine.poll()&&engine.channel(key,1).valid);
 start({1});engine.fixtureStarting();assert(engine.poll());nowMs+=2001;assert(!engine.poll()&&engine.lastError().contains("AboutToStart no completado"));
 // The observer can see a stop/error while draining; post-drain checks still reject.
 start({1});callback();assert(!engine.poll([&](const auto&,const auto&,double){engine.fixtureStopped();}));assert(engine.lastError().contains("audioDeviceStopped"));
 // Callback packing must map to physical Input5, not silently to Input1.
 start({5});for(int i=0;i<100;++i){callback();assert(engine.poll());}assert(engine.rms(key,5)&&!engine.rms(key,1));
 // R3 global M1..4/R5 acquisition mask still yields five distinct physical inputs.
 start({1,2,3,4,5});std::array<std::array<float,480>,5> channels;const float* five[5];
 for(size_t ch=0;ch<5;++ch){channels[ch]=x;for(auto& sample:channels[ch])sample*=(float)(ch+1);five[ch]=channels[ch].data();}
 for(int i=0;i<100;++i){nowMs+=10;engine.fixtureCallback(five,5,480);assert(engine.poll());}
 for(int ch=1;ch<=5;++ch){assert(engine.channel(key,ch).valid&&engine.rms(key,ch));assert(std::abs(*engine.rms(key,ch)-(-43.01029995664+20*std::log10(ch)))<1.e-6);}
 // Invalid callback input count isn't "no callbacks". It produces no readings.
 start({1});nowMs+=10;engine.fixtureCallback(nullptr,0,480);assert(engine.poll()&&engine.running());assert(!engine.channel(key,1).valid&&!engine.rms(key,1));assert(engine.details().contains("callbacks=1")&&engine.details().contains("callbackInputs=0"));
 // Real end-to-end calibration from callback frames, no physical audio, no persistence.
 start({1});for(int i=0;i<100;++i){callback();assert(engine.poll());}
 RmsCalibration::Session capture;assert(capture.start(key,1,48000,94,engine.cursor(key)));
 for(int i=0;i<500;++i){callback();assert(engine.poll([&](const auto& k,const auto& f,double rate){capture.feed(k,f,rate);}));}
 assert(capture.confirmable()&&std::abs(*capture.measured+43.01029995664)<1.e-6);
 // The reported format 48000/512, continuous sine phase and actual callback timing.
 start({1});std::array<float,512> block;const float* actualInput[]={block.data()};int index=0;
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
