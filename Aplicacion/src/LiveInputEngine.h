#pragma once
#include "AudioRouting.h"
#include "InputLevelCore.h"
#include "RmsCalibrationCore.h"
#include "WeightedMeterCore.h"
#include "LoopPink.h"
#include "LoopSweep.h"
#include "LoopCalibrationCore.h"
#include "TransferFunctionCore.h"
#include <functional>
#include <memory>

// One retry is allowed only for a stream that started but never delivered audio.
struct FirstCallbackRetry {
 bool available=false;
 void arm(bool permitted){available=permitted;}
 void cancel(){available=false;}
 bool consume(const juce::String& error){bool retry=available&&error.startsWith("Sin primer callback >2000 ms");available=false;return retry;}
};

// Opens devices ONLY from explicit start(). The selected output can carry a bounded test signal.
class LiveInputEngine {
 using Clock=double(*)();
public:
 enum class Signal { None, Pink, Sweep };
private:
 double pinkLevelDb=-40.,sweepLevelDb=-40.,sweepStartHz=20.,sweepEndHz=20000.,sweepDuration=5.,referenceGainDb=0.;
 static double systemClock(){return juce::Time::getMillisecondCounterHiRes();}
 enum class Fault { None, UnexpectedRestart, BackendError };
 struct Session : juce::AudioIODeviceCallback {
  juce::OwnedArray<juce::AudioIODeviceType> types;
  std::unique_ptr<juce::AudioIODevice> device;
  InputLevels::Queue<> queue;
  TransferFunctionCore::Queue<> rawQueue;
  TransferFunctionCore::Queue<> loopQueue;
  std::atomic<bool> collectLoop{false};
  std::atomic<bool> loopOverflow{false};
  int captureMic=0,captureRef=0;
  std::atomic<std::uint64_t> rawSequence{0};
  std::uint64_t minimumRawSequence=0;
  std::atomic<unsigned> rawDropped{0};
  std::array<int,InputLevels::maxChannels> physical{};
  int count=0;std::uint64_t sequence=0;
  std::atomic<bool> stopped{true},stopNotified{false},aboutReady{false};
  std::atomic<Fault> fault{Fault::None};
  std::atomic<unsigned> dropped{0},starts{0};
  std::atomic<std::uint64_t> produced{0};
  std::atomic<double> lastCallbackMs{0};
  std::atomic<int> lastInputs{0},lastOutputs{0},lastSamples{0};
  std::atomic<float> lastOutputPeak{0.f};
  std::atomic<bool> sweepFinished{false};
  // One-time error publication: the UI reads bytes only after release/acquire Ready.
  std::atomic<int> errorPublication{0};std::array<char,512> backendError{};
  Clock clock=systemClock;double sampleRate=0;
  std::array<RmsCalibration::Rolling,InputLevels::maxChannels> rms;
  std::array<WeightedMeter::Channel,InputLevels::maxChannels> weighted;
  InputLevels::Window latest;
  double receivedAt=0,openedAt=0;unsigned seenDrops=0;
  juce::String key;
  Signal signal=Signal::None;
  bool virtualReference=false;
  std::atomic<float> pinkScale{1.f};
  std::atomic<float> referenceScale{1.f};
  LoopPink pink;
  LoopSweep sweep;
  void audioDeviceAboutToStart(juce::AudioIODevice* d)override{
   if(starts.fetch_add(1)!=0){fault.store(Fault::UnexpectedRestart);return;}
   sampleRate=d->getCurrentSampleRate();count=0;auto active=d->getActiveInputChannels();
   for(int ch=0;ch<InputLevels::maxChannels;++ch)if(active[ch]){physical[(size_t)count++]=ch;weighted[(size_t)ch].configure(sampleRate);}
   aboutReady.store(true,std::memory_order_release);stopped.store(false);
  }
  void audioDeviceStopped()override{stopNotified.store(true);stopped.store(true);}
  void audioDeviceError(const juce::String& text)override{
   int expected=0;
   if(errorPublication.compare_exchange_strong(expected,1)){
    text.copyToUTF8(backendError.data(),backendError.size());errorPublication.store(2,std::memory_order_release);
   }
   fault.store(Fault::BackendError,std::memory_order_release);
  }
  void audioDeviceIOCallbackWithContext(const float* const* input,int inputs,float* const* output,int outputs,int samples,const juce::AudioIODeviceCallbackContext&)override{
   for(int ch=0;ch<outputs;++ch)if(output&&output[ch])std::fill_n(output[ch],samples,0.f);
   float outputPeak=0.f;
   // The open mask enables one physical output; JUCE packs that channel at index zero.
   if(signal!=Signal::None&&outputs>=1&&output&&output[0]){float scale=pinkScale.load(std::memory_order_relaxed);for(int i=0;i<samples;++i){float value=(signal==Signal::Pink?pink.next():sweep.next())*scale;output[0][i]=value;outputPeak=std::max(outputPeak,std::abs(value));}if(signal==Signal::Sweep&&sweep.complete())sweepFinished.store(true,std::memory_order_release);}
   lastOutputs.store(outputs,std::memory_order_relaxed);lastOutputPeak.store(outputPeak,std::memory_order_release);
   const double now=clock();lastCallbackMs.store(now,std::memory_order_release);
   lastInputs.store(inputs,std::memory_order_relaxed);lastSamples.store(samples,std::memory_order_relaxed);
   auto frame=InputLevels::capture(input,fault.load(std::memory_order_relaxed)!=Fault::None?-1:inputs,samples,physical,count,++sequence);
   if(input&&inputs==count)for(int i=0;i<count;++i){int ch=physical[(size_t)i];weighted[(size_t)ch].process(input[i],samples,frame.channels[(size_t)ch]);}
   frame.timeSeconds=now*.001;
   if(!queue.push(frame))dropped.fetch_add(1,std::memory_order_relaxed);
   if(captureMic>0&&(captureRef>0||virtualReference)&&input&&inputs==count&&samples>0){
    int mi=-1,ri=-1;
    for(int ch=0;ch<count;++ch){if(physical[(size_t)ch]==captureMic-1)mi=ch;if(physical[(size_t)ch]==captureRef-1)ri=ch;}
    if(mi>=0&&(virtualReference||(ri>=0&&input[ri]))&&input[mi]&&(!virtualReference||(signal!=Signal::None&&outputs>=1&&output&&output[0])))for(int offset=0;offset<samples;){
     TransferFunctionCore::RawBlock block;block.sequence=rawSequence.fetch_add(1,std::memory_order_acq_rel)+1;block.sampleRate=sampleRate;
     block.count=std::min(TransferFunctionCore::blockCapacity,samples-offset);
     std::copy_n(input[mi]+offset,block.count,block.mic.begin());
     std::copy_n((virtualReference?output[0]:input[ri])+offset,block.count,block.ref.begin());
     float trim=referenceScale.load(std::memory_order_relaxed);if(trim!=1.f)for(int j=0;j<block.count;++j)block.ref[(size_t)j]*=trim;
     if(!rawQueue.push(block))rawDropped.fetch_add(1,std::memory_order_relaxed);
     offset+=block.count;
    }
   }
   if(collectLoop.load(std::memory_order_relaxed)&&signal==Signal::Pink&&output&&outputs>0&&output[0]&&input&&inputs==count){int ri=-1;for(int ch=0;ch<count;++ch)if(physical[(size_t)ch]==captureRef-1)ri=ch;if(ri>=0&&input[ri])for(int offset=0;offset<samples;){TransferFunctionCore::RawBlock block;block.sampleRate=sampleRate;block.count=std::min(TransferFunctionCore::blockCapacity,samples-offset);std::copy_n(input[ri]+offset,block.count,block.mic.begin());std::copy_n(output[0]+offset,block.count,block.ref.begin());if(!loopQueue.push(block)){loopOverflow.store(true,std::memory_order_release);collectLoop.store(false,std::memory_order_relaxed);}offset+=block.count;}}
   produced.store(frame.sequence,std::memory_order_release);
  }
  ~Session()override{if(device){device->stop();device->close();}}
 };
 std::vector<std::unique_ptr<Session>> sessions;
 InputLevels::Plan activePlan;juce::String description,error;
 bool runningFlag=false;std::uint64_t generationValue=0;Clock clock=systemClock;
 juce::String diagnostic(const Session& s,double now)const{
  const bool ready=s.aboutReady.load(std::memory_order_acquire);const auto n=s.produced.load(std::memory_order_acquire);const auto last=s.lastCallbackMs.load(std::memory_order_acquire);
  juce::String result="source="+s.key+"; starts="+juce::String(s.starts.load())+"; stopped="+juce::String((int)s.stopped.load())
   +"; stopNotified="+juce::String((int)s.stopNotified.load())+"; aboutReady="+juce::String((int)ready)+"; callbacks="+juce::String((juce::int64)n)+"; processed="+juce::String((juce::int64)s.latest.lastSequence)
   +"; dropped="+juce::String(s.dropped.load())+"; openedMs="+juce::String(s.openedAt,3)
   +"; nowMs="+juce::String(now,3)+"; callbackMs="+juce::String(last,3)
   +"; callbackAgeMs="+(n?juce::String(now-last,3):juce::String("none"))
   +"; processedMs="+juce::String(s.receivedAt,3)+"; rate="+juce::String(ready?s.sampleRate:0.,3)
   +"; enabledInputs="+juce::String(ready?s.count:0)+"; callbackInputs="+juce::String(s.lastInputs.load())
   +"; callbackOutputs="+juce::String(s.lastOutputs.load())+"; outputPeak="+juce::String(s.lastOutputPeak.load(),6)
   +"; callbackSamples="+juce::String(s.lastSamples.load());
  if(s.errorPublication.load(std::memory_order_acquire)==2)result+="; backendError="+juce::String::fromUTF8(s.backendError.data());
  return result;
 }
 bool fail(const Session& s,const juce::String& reason,double now){
  error=reason+" ["+s.key+"]. Pulsa Play para reabrir.";
  description=error+"\n"+diagnostic(s,now);stop();return false;
 }
public:
 explicit LiveInputEngine(Clock time=systemClock):clock(time?time:systemClock){}
 ~LiveInputEngine(){stop();}
 bool running()const{return runningFlag;}
 const juce::String& lastError()const{return error;}
 const juce::String& summary()const{return runningFlag?description:error;}
 juce::String details()const{auto text=description;if(runningFlag)for(const auto& s:sessions)text+="\n"+diagnostic(*s,clock());return text;}
 unsigned drops()const{unsigned n=0;for(const auto& s:sessions)n+=s->dropped.load(std::memory_order_relaxed);return n;}
 void setPinkLevelDb(double db){db=juce::jlimit(-60.,-20.,db);pinkLevelDb=db;float scale=(float)std::pow(10.,(db+40.)/20.);for(auto& s:sessions)s->pinkScale.store(scale,std::memory_order_release);}
 double pinkLevel()const{return pinkLevelDb;}
 void setSweepLevelDb(double db){db=juce::jlimit(-60.,-20.,db);sweepLevelDb=db;for(auto& s:sessions)if(s->signal==Signal::Sweep)s->pinkScale.store((float)std::pow(10.,(db+40.)/20.),std::memory_order_release);}
 double sweepLevel()const{return sweepLevelDb;}
 void setSweep(double startHz,double endHz,double duration){sweepStartHz=juce::jlimit(10.,20000.,startHz);sweepEndHz=juce::jlimit(sweepStartHz+1.,24000.,endHz);sweepDuration=juce::jlimit(.1,60.,duration);}
 double sweepStart()const{return sweepStartHz;} double sweepEnd()const{return sweepEndHz;} double sweepSeconds()const{return sweepDuration;}
 void setReferenceGainDb(double db){db=juce::jlimit(-24.,24.,db);referenceGainDb=db;float scale=(float)std::pow(10.,db/20.);for(auto& s:sessions)if(!s->virtualReference)s->referenceScale.store(scale,std::memory_order_release);}
 double referenceGain()const{return referenceGainDb;}
 bool startLoopCalibration(const std::string& key){for(auto& s:sessions)if(s->key.toStdString()==key&&s->signal==Signal::Pink&&!s->virtualReference&&s->captureRef>0){bool hasReference=false;for(int i=0;i<s->count;++i)hasReference=hasReference||s->physical[(size_t)i]==s->captureRef-1;if(!hasReference)return false;TransferFunctionCore::RawBlock discarded;while(s->loopQueue.pop(discarded)){}s->loopOverflow.store(false);s->collectLoop.store(true,std::memory_order_release);return true;}return false;}
 void cancelLoopCalibration(){for(auto& s:sessions)s->collectLoop.store(false,std::memory_order_release);}
 bool loopCalibrationActive(const std::string& key)const{for(const auto& s:sessions)if(s->key.toStdString()==key)return s->collectLoop.load(std::memory_order_acquire);return false;}
 bool loopCalibrationOverflow(const std::string& key)const{for(const auto& s:sessions)if(s->key.toStdString()==key)return s->loopOverflow.load(std::memory_order_acquire);return false;}
 void drainLoopCalibration(const std::string& key,const std::function<void(const TransferFunctionCore::RawBlock&)>& observe){for(auto& s:sessions)if(s->key.toStdString()==key){TransferFunctionCore::RawBlock block;while(s->loopQueue.pop(block))if(observe)observe(block);}}
 std::uint64_t generation()const{return generationValue;}
 static std::vector<InputLevels::Device> catalog(const std::vector<AudioEndpoint>& endpoints){std::vector<InputLevels::Device> result;for(const auto& d:endpoints)result.push_back({d.key.toStdString(),d.inputs.size()});return result;}
 bool start(const InputLevels::Plan& plan,const std::vector<AudioEndpoint>& endpoints,Signal outputSignal=Signal::None,const juce::String& outputDeviceKey={},const juce::String& tfDeviceKey={},int tfMic=0,int tfRef=0,bool internalReference=false,int outputChannel=0){
  stop();error.clear();description.clear();if(!plan.valid()){error=juce::String::fromUTF8(plan.error.c_str());return false;}
  if(outputSignal!=Signal::None){
   if(plan.groups.size()!=1||juce::String::fromUTF8(plan.groups.front().key.c_str())!=outputDeviceKey){error="El loop requiere la misma tarjeta para entrada y salida";return false;}
   auto* outputEndpoint=findEndpoint(endpoints,outputDeviceKey);
   if(!outputEndpoint||outputChannel<1||outputChannel>outputEndpoint->outputs.size()){error="Canal de salida no disponible";return false;}
  }
  for(const auto& group:plan.groups){
   auto* endpoint=findEndpoint(endpoints,juce::String::fromUTF8(group.key.c_str()));
   if(!endpoint){error="Dispositivo no disponible";stop();return false;}
   auto session=std::make_unique<Session>();session->key=endpoint->key;session->clock=clock;session->signal=outputSignal;session->pinkScale.store((float)std::pow(10.,((outputSignal==Signal::Sweep?sweepLevelDb:pinkLevelDb)+40.)/20.));session->referenceScale.store((float)std::pow(10.,referenceGainDb/20.));
   if(endpoint->key==tfDeviceKey&&tfMic>0&&(tfRef>0||internalReference)){session->captureMic=tfMic;session->captureRef=tfRef;session->virtualReference=internalReference;if(internalReference)session->referenceScale.store(1.f);}
   juce::AudioDeviceManager manager;manager.createAudioDeviceTypes(session->types);juce::AudioIODeviceType* type=nullptr;
   for(auto* t:session->types)if(t->getTypeName()==endpoint->backend){type=t;break;}
   if(!type){error="Backend no disponible: "+endpoint->backend;stop();return false;}
   type->scanForDevices();juce::BigInteger mask;for(int ch=0;ch<InputLevels::maxChannels;++ch)if(group.enabled[(size_t)ch])mask.setBit(ch);session->device.reset(type->createDevice(outputSignal!=Signal::None?endpoint->name:juce::String{},mask.isZero()?juce::String{}:endpoint->name));
   if(!session->device){error="No se pudo crear entrada: "+endpoint->name;stop();return false;}
   auto rates=session->device->getAvailableSampleRates();double rate=session->device->getCurrentSampleRate();
   if(rate<=0||!rates.contains(rate))rate=rates.isEmpty()?0:rates[0];int size=session->device->getDefaultBufferSize();
   if(rate<=0||size<=0){error="Formato de entrada no disponible";stop();return false;}
   juce::BigInteger outputMask;if(outputSignal!=Signal::None)outputMask.setBit(outputChannel-1);
   auto failure=session->device->open(mask,outputMask,rate,size);
   if(failure.isNotEmpty()){error=endpoint->name+": "+failure;stop();return false;}
   if(session->device->getActiveInputChannels()!=mask||session->device->getActiveOutputChannels()!=outputMask){error="Backend no respeto los canales de entrada/salida solicitados";stop();return false;}
   session->openedAt=clock();session->device->start(session.get());
   if(session->fault.load()==Fault::BackendError)return fail(*session,"Error notificado por backend al iniciar",clock());
   if(session->fault.load()==Fault::UnexpectedRestart)return fail(*session,"Backend reinicio callback al iniciar",clock());
   if(!session->device->isPlaying())return fail(*session,"Dispositivo no entro en reproduccion de entrada",clock());
   session->sweep.configure(rate,sweepStartHz,sweepEndHz,sweepDuration);
   description+=endpoint->name+" "+juce::String(session->device->getCurrentSampleRate(),0)+" Hz / "+juce::String(session->device->getCurrentBufferSizeSamples())+(outputSignal!=Signal::None?" / "+juce::String(outputSignal==Signal::Pink?"rosa":"sweep")+" Output "+juce::String(outputChannel)+" "+juce::String(outputSignal==Signal::Pink?pinkLevelDb:sweepLevelDb,0)+" dBFS; ":"; ");
   sessions.push_back(std::move(session));
  }
  activePlan=plan;runningFlag=true;return true;
 }
 void stop(){++generationValue;runningFlag=false;sessions.clear();activePlan={};}
 // Called by the UI thread. Blocks already issued by the callback are excluded,
 // including one still being copied while the queue is drained.
 void discardPendingTF(){for(auto& s:sessions)if(s->captureMic>0&&(s->captureRef>0||s->virtualReference)){s->minimumRawSequence=s->rawSequence.load(std::memory_order_acquire);TransferFunctionCore::RawBlock block;while(s->rawQueue.pop(block)){};}}
 bool reconcile(const std::vector<AudioEndpoint>& endpoints){if(runningFlag&&!InputLevels::available(activePlan,catalog(endpoints))){error="Entrada desconectada o canales modificados. Reconecta y pulsa Play.";description=error;stop();return false;}return true;}
 // Heartbeat belongs to the producer, not the time of the UI's previous poll.
 // Drain before the timeout check. Backlogged samples still retain their OLD timestamps.
 bool poll(const std::function<void(const std::string&,const InputLevels::Frame&,double)>& observe={},const std::function<void(const std::string&,const TransferFunctionCore::RawBlock&)>& observeRaw={}){
  if(!runningFlag)return false;
  for(auto& s:sessions){
   double now=clock();auto fault=s->fault.load(std::memory_order_acquire);
   if(fault==Fault::BackendError)return fail(*s,"Error notificado por backend",now);
   if(fault==Fault::UnexpectedRestart)return fail(*s,"Reinicio inesperado del callback",now);
   if(s->stopNotified.load())return fail(*s,"Backend notifico audioDeviceStopped",now);
   if(s->stopped.load()){
    if(now-s->openedAt<=2000)continue; // AboutToStart may still be pending; no real Stop was notified.
    return fail(*s,s->starts.load()==0?"No se recibio AboutToStart >2000 ms":"AboutToStart no completado >2000 ms",now);
   }
   InputLevels::Window window;InputLevels::Frame frame;double lastProcessedMs=0;
   for(int drained=0;drained<64&&s->queue.pop(frame);++drained){
    if(!std::isfinite(frame.timeSeconds)||frame.timeSeconds<=0)return fail(*s,"Timestamp de callback invalido",clock());
    lastProcessedMs=frame.timeSeconds*1000.;window.merge(frame);
    for(int i=0;i<s->count;++i){auto ch=(size_t)s->physical[(size_t)i];s->rms[ch].feed(frame.channels[ch],frame.sequence,s->sampleRate,frame.timeSeconds);}
    if(observe)observe(s->key.toStdString(),frame,s->sampleRate);
   }
   if(observeRaw){TransferFunctionCore::RawBlock raw;for(int drained=0;drained<64&&s->rawQueue.pop(raw);++drained)if(raw.sequence>s->minimumRawSequence)observeRaw(s->key.toStdString(),raw);}
   unsigned count=s->dropped.load();bool gap=count!=s->seenDrops;s->seenDrops=count;
   if(gap){for(auto& rms:s->rms)rms.reset();for(auto& weighted:s->weighted)weighted.configure(s->sampleRate);}
   if(window.any){window.gap=window.gap||gap;s->latest=window;s->receivedAt=lastProcessedMs;}else if(gap)s->latest={};
   now=clock();fault=s->fault.load(std::memory_order_acquire);
   if(fault==Fault::BackendError)return fail(*s,"Error notificado por backend",now);
   if(fault==Fault::UnexpectedRestart)return fail(*s,"Reinicio inesperado del callback",now);
   if(s->stopNotified.load())return fail(*s,"Backend notifico audioDeviceStopped",now);
   const auto n=s->produced.load(std::memory_order_acquire);const auto last=s->lastCallbackMs.load(std::memory_order_acquire);now=clock();
   if((n||last>0)&&(!std::isfinite(last)||last<=0||last>now+.001))return fail(*s,"Reloj de callback invalido",now);
   if(now-(last>0?last:s->openedAt)>2000)return fail(*s,last>0?"Callbacks interrumpidos >2000 ms":"Sin primer callback >2000 ms",now);
  }
  return true;
 }
 bool caughtUp(const std::string& key)const{for(const auto& s:sessions)if(s->key.toStdString()==key)return s->latest.any&&s->latest.lastSequence==s->produced.load(std::memory_order_acquire);return false;}
 double rate(const std::string& key)const{for(const auto& s:sessions)if(s->key.toStdString()==key)return s->aboutReady.load(std::memory_order_acquire)?s->sampleRate:0.;return 0;}
 int bufferSamples(const std::string& key)const{for(const auto& s:sessions)if(s->key.toStdString()==key)return s->device?s->device->getCurrentBufferSizeSamples():0;return 0;}
 std::optional<double> outputDbfs(const std::string& key)const{if(!runningFlag)return {};for(const auto& s:sessions)if(s->key.toStdString()==key){if(s->lastOutputs.load(std::memory_order_acquire)<1)return {};float peak=s->lastOutputPeak.load(std::memory_order_acquire);if(peak<=0)return {};return 20.*std::log10((double)peak);}return {};}
 bool sweepComplete()const{for(const auto& s:sessions)if(s->signal==Signal::Sweep&&s->sweepFinished.load(std::memory_order_acquire))return true;return false;}
 std::uint64_t cursor(const std::string& key)const{for(const auto& s:sessions)if(s->key.toStdString()==key)return s->produced.load(std::memory_order_acquire);return 0;}
 std::optional<double> rms(const std::string& key,int ch)const{if(!channel(key,ch).valid)return {};for(const auto& s:sessions)if(s->key.toStdString()==key)return s->rms[(size_t)ch-1].level;return {};}
 bool rmsClip(const std::string& key,int ch)const{if(ch<1||ch>InputLevels::maxChannels)return false;for(const auto& s:sessions)if(s->key.toStdString()==key)return s->rms[(size_t)ch-1].hasClip();return false;}
 InputLevels::Peak channel(const std::string& key,int channel)const{if(!runningFlag||channel<1||channel>InputLevels::maxChannels)return {};double now=clock();for(const auto& s:sessions)if(s->key.toStdString()==key){if(!s->latest.any||s->latest.gap||now-s->receivedAt>250)return {};return s->latest.channels[(size_t)channel-1];}return {};}
 #if defined(MYA_INPUT_ENGINE_TEST)
 // Drive the ACTUAL callback/queue/poll path with a clock, no device creation/open.
 void fixtureStart(juce::String key,double rate,const std::vector<int>& channels,bool pinkOutput=false,int tfMic=0,int tfRef=0,bool internalReference=false){fixtureSignal(key,rate,channels,pinkOutput?Signal::Pink:Signal::None,tfMic,tfRef,internalReference);}
 void fixtureSignal(juce::String key,double rate,const std::vector<int>& channels,Signal signal,int tfMic=0,int tfRef=0,bool internalReference=false){stop();error.clear();description.clear();auto s=std::make_unique<Session>();s->key=key;s->clock=clock;s->sampleRate=rate;s->openedAt=clock();s->starts=1;s->aboutReady=true;s->stopped=false;s->signal=signal;s->captureMic=tfMic;s->captureRef=tfRef;s->virtualReference=internalReference;s->pinkScale.store((float)std::pow(10.,(((signal==Signal::Sweep?sweepLevelDb:pinkLevelDb)+40.)/20.)));s->sweep.configure(rate,sweepStartHz,sweepEndHz,sweepDuration);for(int ch:channels){s->physical[(size_t)s->count++]=ch-1;s->weighted[(size_t)ch-1].configure(rate);}sessions.push_back(std::move(s));runningFlag=true;}
 void fixtureCallback(const float* const* input,int inputs,int samples,size_t session=0){sessions[session]->audioDeviceIOCallbackWithContext(input,inputs,nullptr,0,samples,{});}
 void fixtureOutputCallback(const float* const* input,int inputs,float* output,int samples){float* channels[]={output};sessions.front()->audioDeviceIOCallbackWithContext(input,inputs,channels,1,samples,{});}
 void fixtureStereoOutputCallback(const float* const* input,int inputs,float* left,float* right,int samples){float* channels[]={left,right};sessions.front()->audioDeviceIOCallbackWithContext(input,inputs,channels,2,samples,{});}
 void fixtureNoAboutToStart(){sessions.front()->starts=0;sessions.front()->aboutReady=false;sessions.front()->stopped=true;}
 void fixtureStarting(){fixtureNoAboutToStart();sessions.front()->starts=1;}
 void fixtureFinishStart(){sessions.front()->starts=1;sessions.front()->aboutReady=true;sessions.front()->stopped=false;}
 void fixtureStopped(){sessions.front()->audioDeviceStopped();}
 void fixtureBackendError(const juce::String& text){sessions.front()->audioDeviceError(text);}
 void fixtureRestart(){sessions.front()->audioDeviceAboutToStart(nullptr);}
 #endif
};
