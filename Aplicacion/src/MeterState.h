#pragma once
#include "InputLevelCore.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <optional>

// Configuration only. No level estimator, audio routing changes or calibration claims.
class MeterState {
public:
 struct Source {juce::String deviceKey,label;int channel=0;};
 struct Card {juce::String id,name,deviceKey;int channel=0,metric=1,weight=1,alarmMetric=0,alarmWeight=0;double warning=105.,alarm=110.,hysteresis=1.;bool alarmEnabled=true,animate=true;double leqSeconds=10.;juce::String textColour="ffe5edf4",backgroundColour="ff080a0c",warningColour="ffff9500",alarmColour="ffe83232";};
 struct State {int rows=2,columns=2,x=100,y=100,width=960,height=660;bool sidebarMeters=false,mainVisible=true,clock=false,clock24=true,alwaysOnTop=false;juce::String primaryId;std::vector<Card> cards;};
 struct Runtime {std::optional<double> current,maximum,leqFill;bool acquisitionValid=false,calibrated=false,overload=false,leqComplete=false;unsigned maxResetCount=0,leqResetCount=0;double leqPower=0;std::uint64_t leqSamples=0,lastSequence=0;void resetMax(){maximum.reset();++maxResetCount;}void resetLeq(){leqFill.reset();leqPower=0;leqSamples=0;lastSequence=0;leqComplete=false;current.reset();++leqResetCount;}};
private:
 juce::File file;State data;juce::String error;bool usable=true;unsigned revision=0;juce::String defaultDevice;
 int nextListener=0;std::map<int,std::function<void()>> listeners;std::vector<Source> inputs;std::map<juce::String,Runtime> readings;
 static Card card(int n){Card c;c.channel=1;c.id=juce::Uuid().toString();c.name="Medidor "+juce::String(n);return c;}
 static bool colourValid(const juce::String& c){return c.length()==8&&c.containsOnly("0123456789abcdefABCDEF");}
 static bool valid(const State& s){
  if(s.rows<1||s.rows>1000||s.columns<1||s.columns>1000||s.cards.size()>10000||s.width<320||s.height<260||s.width>10000||s.height>10000)return false;
  std::set<juce::String> ids;for(const auto& c:s.cards)if(!std::isfinite(c.warning)||!std::isfinite(c.alarm)||!std::isfinite(c.hysteresis)||c.warning>=c.alarm||c.warning< -200||c.alarm>200||c.hysteresis<0||c.hysteresis>20||(c.alarmMetric==1&&c.alarm>0)||c.alarmMetric<0||c.alarmMetric>6||c.alarmWeight<0||c.alarmWeight>3||c.id.isEmpty()||!ids.insert(c.id).second||c.name.trim().isEmpty()||!std::isfinite(c.leqSeconds)||c.leqSeconds<1||c.leqSeconds>86400||!colourValid(c.textColour)||!colourValid(c.backgroundColour)||!colourValid(c.warningColour)||!colourValid(c.alarmColour)||c.channel<0||c.metric<1||c.metric>6||c.weight<1||c.weight>3||((c.metric==1||c.metric==6)&&c.weight!=1))return false;
  return true;
 }
 static juce::var encode(const State& s){
  auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("rows",s.rows);o->setProperty("columns",s.columns);o->setProperty("x",s.x);o->setProperty("y",s.y);o->setProperty("width",s.width);o->setProperty("height",s.height);o->setProperty("sidebarMeters",s.sidebarMeters);o->setProperty("mainVisible",s.mainVisible);o->setProperty("clock",s.clock);o->setProperty("clock24",s.clock24);o->setProperty("alwaysOnTop",s.alwaysOnTop);o->setProperty("primaryId",s.primaryId);juce::Array<juce::var> cards;
  for(const auto& c:s.cards){auto* a=new juce::DynamicObject();a->setProperty("id",c.id);a->setProperty("name",c.name);a->setProperty("deviceKey",c.deviceKey);a->setProperty("inputChannel",c.channel);a->setProperty("metric",c.metric);a->setProperty("weight",c.weight);a->setProperty("warning",c.warning);a->setProperty("alarm",c.alarm);a->setProperty("hysteresis",c.hysteresis);a->setProperty("alarmEnabled",c.alarmEnabled);a->setProperty("animate",c.animate);a->setProperty("alarmMetric",c.alarmMetric);a->setProperty("alarmWeight",c.alarmWeight);a->setProperty("leqSeconds",c.leqSeconds);a->setProperty("textColour",c.textColour);a->setProperty("backgroundColour",c.backgroundColour);a->setProperty("warningColour",c.warningColour);a->setProperty("alarmColour",c.alarmColour);cards.add(juce::var(a));}o->setProperty("cards",cards);return juce::var(o);
 }
 static bool decode(const juce::var& v,State& s){
  if(!v.isObject()||int(v["schema"])!=1||!v["cards"].isArray())return false;for(auto key:{"mainVisible","clock","clock24","alwaysOnTop"})if(v.hasProperty(key)&&!v[key].isBool())return false;
  s={};s.rows=int(v["rows"]);s.columns=int(v["columns"]);s.x=int(v["x"]);s.y=int(v["y"]);s.width=int(v["width"]);s.height=int(v["height"]);s.sidebarMeters=bool(v["sidebarMeters"]);if(v.hasProperty("mainVisible"))s.mainVisible=bool(v["mainVisible"]);if(v.hasProperty("clock"))s.clock=bool(v["clock"]);if(v.hasProperty("clock24"))s.clock24=bool(v["clock24"]);if(v.hasProperty("alwaysOnTop"))s.alwaysOnTop=bool(v["alwaysOnTop"]);s.primaryId=v["primaryId"].toString();
  for(const auto& a:*v["cards"].getArray()){if(!a.isObject())return false;Card c;c.id=a["id"].toString();c.name=a["name"].toString();c.deviceKey=a["deviceKey"].toString();c.channel=int(a["inputChannel"]);c.metric=int(a["metric"]);c.weight=int(a["weight"]);if(a.hasProperty("warning"))c.warning=double(a["warning"]);if(a.hasProperty("alarm"))c.alarm=double(a["alarm"]);if(a.hasProperty("hysteresis"))c.hysteresis=double(a["hysteresis"]);if(a.hasProperty("alarmEnabled"))c.alarmEnabled=bool(a["alarmEnabled"]);if(a.hasProperty("animate"))c.animate=bool(a["animate"]);if(a.hasProperty("alarmMetric"))c.alarmMetric=int(a["alarmMetric"]);if(a.hasProperty("alarmWeight"))c.alarmWeight=int(a["alarmWeight"]);if(a.hasProperty("leqSeconds"))c.leqSeconds=double(a["leqSeconds"]);if(a.hasProperty("textColour"))c.textColour=a["textColour"].toString();if(a.hasProperty("backgroundColour"))c.backgroundColour=a["backgroundColour"].toString();if(a.hasProperty("warningColour"))c.warningColour=a["warningColour"].toString();if(a.hasProperty("alarmColour"))c.alarmColour=a["alarmColour"].toString();s.cards.push_back(c);}return valid(s);
 }
 void notify(){auto copy=listeners;for(const auto& l:copy)if(listeners.count(l.first)&&l.second)l.second();}
 bool commit(State s,bool announce=true){
  if(!usable)return false;if(!valid(s)){error="Invalid meter configuration";return false;}
  juce::TemporaryFile temp(file);if(!temp.getFile().replaceWithText(juce::JSON::toString(encode(s)))||!temp.overwriteTargetFileWithTemporary()){error="No se pudieron guardar los medidores";return false;}
  data=std::move(s);++revision;error.clear();if(announce)notify();return true;
 }
public:
 explicit MeterState(juce::File preferences,const MeterState* seed=nullptr):file(std::move(preferences)){
  if(!file.existsAsFile()&&seed){data=seed->state();usable=seed->ready();if(!usable){error="Preferencias originales invalidas; archivo conservado";return;}auto oldPrimary=data.primaryId;for(auto& c:data.cards){auto old=c.id;c.id=juce::Uuid().toString();if(old==oldPrimary)data.primaryId=c.id;}if(!commit(data,false))usable=false;return;}
  if(file.existsAsFile()){State loaded;if(decode(juce::JSON::parse(file),loaded))data=std::move(loaded);else{usable=false;error=juce::String::fromUTF8("Configuración de medidores inválida; archivo conservado");}}
  if(!file.existsAsFile()||!usable)for(int i=1;i<=4;++i)data.cards.push_back(card(i));
 }
 const State& state() const{return data;}
 void setDefaultDevice(const juce::String& key){defaultDevice=key;}
 unsigned configRevision()const{return revision;}
 static juce::var parseNumber(const juce::String& text){auto list=juce::JSON::parse("["+text.trim().replaceCharacter(',','.')+"]");if(!list.isArray()||list.size()!=1)return {};auto v=list[0];return (v.isDouble()||v.isInt()||v.isInt64())&&std::isfinite(double(v))?v:juce::var{};}
 static juce::String valueText(std::optional<double> value){return !value?juce::String("--"):std::isinf(*value)&&*value<0?juce::String::fromUTF8("−∞"):juce::String(*value,1);}
 void clearReadings(){readings.clear();notify();}
 // UI-thread publication. Only a matching unweighted digital peak can enter runtime.
 void publishFs(const juce::String& id,const juce::String& device,int channel,std::optional<double> db,bool overload){auto* c=find(id);if(!c)return;auto& r=readings[id];r.acquisitionValid=c->metric==1&&c->weight==1&&c->deviceKey==device&&c->channel==channel&&db&&!std::isnan(*db)&&(!std::isinf(*db)||*db<0);r.calibrated=false;r.current=r.acquisitionValid?db:std::nullopt;r.overload=r.acquisitionValid&&overload;if(r.current&&(!r.maximum||*r.current>*r.maximum))r.maximum=r.current;}
 void publishRmsSpl(const juce::String& id,const juce::String& device,int channel,std::optional<double> spl,bool acquired,bool calibrated,bool overload){auto* c=find(id);if(!c)return;bool match=c->metric==6&&c->weight==1&&c->deviceKey==device&&c->channel==channel;auto& r=readings[id];r.acquisitionValid=match&&acquired;r.calibrated=match&&calibrated;r.overload=match&&overload;r.current=r.acquisitionValid&&r.calibrated&&!overload&&spl&&std::isfinite(*spl)?spl:std::nullopt;if(r.current&&(!r.maximum||*r.current>*r.maximum))r.maximum=r.current;}
 void feedWeighted(const juce::String& id,const juce::String& device,int channel,const InputLevels::Peak& peak,std::uint64_t sequence,double sampleRate,std::optional<double> calibrationOffset){for(const auto& c:data.cards){if(c.id!=id||c.deviceKey!=device||c.channel!=channel||c.metric<2||c.metric>5)continue;auto& r=readings[c.id];if(r.lastSequence&&sequence!=r.lastSequence+1)r.resetLeq();r.lastSequence=sequence;r.acquisitionValid=peak.valid&&peak.weightedValid&&sampleRate>=32000&&sampleRate<=384000;r.calibrated=calibrationOffset.has_value();r.overload=peak.clipped;if(!r.acquisitionValid||!r.calibrated||r.overload){r.current.reset();r.resetLeq();continue;}int w=c.weight-1;double power=c.metric==2?peak.fastPower[(size_t)w]:c.metric==3?peak.slowPower[(size_t)w]:peak.weightedPeak[(size_t)w]*peak.weightedPeak[(size_t)w];if(c.metric==5){auto target=(std::uint64_t)std::ceil(c.leqSeconds*sampleRate);if(!r.leqComplete){auto take=std::min<std::uint64_t>(peak.samples,target-r.leqSamples);r.leqPower+=peak.weightedSquares[(size_t)w]*(double)take/(double)peak.samples;r.leqSamples+=take;r.leqComplete=r.leqSamples>=target;}r.leqFill=std::min(1.,(double)r.leqSamples/(double)target);if(!r.leqComplete){r.current.reset();continue;}power=r.leqPower/(double)r.leqSamples;}else r.leqFill.reset();if(std::isfinite(power)&&power>=0)r.current=(power>0?10.*std::log10(power):-std::numeric_limits<double>::infinity())+*calibrationOffset;else r.current.reset();if(r.current&&(!r.maximum||*r.current>*r.maximum))r.maximum=r.current;}}
 void validateWeighted(const juce::String& id,bool live,bool calibrated,bool overload){auto* c=find(id);if(!c||c->metric<2||c->metric>5)return;auto& r=readings[id];if(!live||!calibrated||overload){r.current.reset();r.acquisitionValid=live&&r.acquisitionValid;r.calibrated=calibrated;r.overload=overload;if((!live||overload)&&r.leqSamples)r.resetLeq();}}
 void invalidateRms(const juce::String& key,int channel){for(const auto& c:data.cards)if(c.metric>=2&&c.deviceKey==key&&c.channel==channel)readings.erase(c.id);notify();}
 void readingsChanged(){notify();}
 const Card* primaryCard()const{auto* card=find(data.primaryId);return card?card:data.cards.empty()?nullptr:&data.cards.front();}
 const Runtime& runtime(const juce::String& id)const{auto found=readings.find(id);if(found!=readings.end())return found->second;static const Runtime empty;return empty;}
 bool resetMax(const juce::String& id){if(!find(id))return false;readings[id].resetMax();notify();return true;}
 bool resetLeq(const juce::String& id){if(!find(id))return false;readings[id].resetLeq();notify();return true;}
 bool primary(const juce::String& id){if(!find(id))return false;auto s=data;s.primaryId=id;return commit(s);}
 bool mainVisibility(bool shown){auto s=data;s.mainVisible=shown;return commit(s);}
 bool clockMode(bool clock){auto s=data;s.clock=clock;return commit(s);}
 bool clockFormat(bool h24){auto s=data;s.clock24=h24;return commit(s);}
 bool topmost(bool top){auto s=data;s.alwaysOnTop=top;return commit(s);}

 const std::vector<Source>& sources() const{return inputs;}
 bool ready() const{return usable;}
 const juce::String& lastError() const{return error;}
 int listen(std::function<void()> f){int id=++nextListener;listeners[id]=std::move(f);return id;}
 void unlisten(int id){listeners.erase(id);}
 void setSources(std::vector<Source> sources){inputs=std::move(sources);++revision;notify();}
 const Card* find(const juce::String& id) const{for(const auto& c:data.cards)if(c.id==id)return &c;return nullptr;}
 bool add(){if(data.cards.size()>=10000){error="Meter configuration capacity reached";return false;}auto s=data;auto added=card((int)s.cards.size()+1);if(auto* c=primaryCard())added.deviceKey=c->deviceKey;if(added.deviceKey.isEmpty())added.deviceKey=defaultDevice;s.cards.push_back(added);return commit(s);}
 bool remove(const juce::String& id){auto s=data;auto old=s.cards.size();s.cards.erase(std::remove_if(s.cards.begin(),s.cards.end(),[&](const Card& c){return c.id==id;}),s.cards.end());bool ok=old!=s.cards.size()&&commit(s);if(ok)readings.erase(id);return ok;}
 bool update(const Card& card){auto s=data;for(auto& c:s.cards)if(c.id==card.id){bool changed=c.deviceKey!=card.deviceKey||c.channel!=card.channel||c.metric!=card.metric||c.weight!=card.weight||c.leqSeconds!=card.leqSeconds;c=card;if(c.metric==1||c.metric==6)c.weight=1;bool ok=commit(s,false);if(ok){if(changed)readings.erase(card.id);notify();}return ok;}return false;}
 bool move(const juce::String& id,int step){auto s=data;for(size_t i=0;i<s.cards.size();++i)if(s.cards[i].id==id){int target=(int)i+step;if(target<0||target>=(int)s.cards.size())return false;std::swap(s.cards[i],s.cards[(size_t)target]);return commit(s);}return false;}
 bool setGrid(int rows,int columns){auto s=data;s.rows=rows;s.columns=columns;return commit(s);}
 bool sidebar(bool meters){auto s=data;s.sidebarMeters=meters;return commit(s);}
 bool window(int x,int y,int width,int height){if(x==data.x&&y==data.y&&width==data.width&&height==data.height)return true;auto s=data;s.x=x;s.y=y;s.width=width;s.height=height;return commit(s,false);}
 static juce::String unavailableInputLabel(const Card& c){if(c.channel<=0)return "Sin asignar";return "Input "+juce::String(c.channel)+(c.deviceKey.isEmpty()?" (sin dispositivo)":" [no disponible]");}
 static bool sameInput(const Card& c,const Source& s){return c.deviceKey==s.deviceKey&&c.channel==s.channel;}
};
