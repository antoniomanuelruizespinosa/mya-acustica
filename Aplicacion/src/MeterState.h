#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <vector>

// Configuration only. No level estimator, audio routing changes or calibration claims.
class MeterState {
public:
 struct Source {juce::String deviceKey,label;int channel=0;};
 struct Card {juce::String id,name,deviceKey;int channel=0,metric=1,weight=1,alarmMetric=0,alarmWeight=0;double warning=105.,alarm=110.,hysteresis=1.;bool alarmEnabled=true,animate=true;};
 struct State {int rows=2,columns=2,x=100,y=100,width=960,height=660;bool sidebarMeters=false;std::vector<Card> cards;};
private:
 juce::File file;State data;juce::String error;bool usable=true;
 int nextListener=0;std::map<int,std::function<void()>> listeners;std::vector<Source> inputs;
 static Card card(int n){Card c;c.id=juce::Uuid().toString();c.name="Medidor "+juce::String(n);return c;}
 static bool valid(const State& s){
  if(s.rows<1||s.rows>1000||s.columns<1||s.columns>1000||s.cards.size()>10000||s.cards.size()>(size_t)(s.rows*s.columns)||s.width<320||s.height<260||s.width>10000||s.height>10000)return false;
  std::set<juce::String> ids;for(const auto& c:s.cards)if(!std::isfinite(c.warning)||!std::isfinite(c.alarm)||!std::isfinite(c.hysteresis)||c.warning>=c.alarm||c.warning< -200||c.alarm>200||c.hysteresis<0||c.hysteresis>20||(c.alarmMetric==1&&c.alarm>0)||c.alarmMetric<0||c.alarmMetric>5||c.alarmWeight<0||c.alarmWeight>3||c.id.isEmpty()||!ids.insert(c.id).second||c.name.trim().isEmpty()||c.channel<0||c.metric<1||c.metric>5||c.weight<1||c.weight>3||(c.metric==1&&c.weight!=1))return false;
  return true;
 }
 static juce::var encode(const State& s){
  auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("rows",s.rows);o->setProperty("columns",s.columns);o->setProperty("x",s.x);o->setProperty("y",s.y);o->setProperty("width",s.width);o->setProperty("height",s.height);o->setProperty("sidebarMeters",s.sidebarMeters);juce::Array<juce::var> cards;
  for(const auto& c:s.cards){auto* a=new juce::DynamicObject();a->setProperty("id",c.id);a->setProperty("name",c.name);a->setProperty("deviceKey",c.deviceKey);a->setProperty("inputChannel",c.channel);a->setProperty("metric",c.metric);a->setProperty("weight",c.weight);a->setProperty("warning",c.warning);a->setProperty("alarm",c.alarm);a->setProperty("hysteresis",c.hysteresis);a->setProperty("alarmEnabled",c.alarmEnabled);a->setProperty("animate",c.animate);a->setProperty("alarmMetric",c.alarmMetric);a->setProperty("alarmWeight",c.alarmWeight);cards.add(juce::var(a));}o->setProperty("cards",cards);return juce::var(o);
 }
 static bool decode(const juce::var& v,State& s){
  if(!v.isObject()||int(v["schema"])!=1||!v["cards"].isArray())return false;
  s={};s.rows=int(v["rows"]);s.columns=int(v["columns"]);s.x=int(v["x"]);s.y=int(v["y"]);s.width=int(v["width"]);s.height=int(v["height"]);s.sidebarMeters=bool(v["sidebarMeters"]);
  for(const auto& a:*v["cards"].getArray()){if(!a.isObject())return false;Card c;c.id=a["id"].toString();c.name=a["name"].toString();c.deviceKey=a["deviceKey"].toString();c.channel=int(a["inputChannel"]);c.metric=int(a["metric"]);c.weight=int(a["weight"]);if(a.hasProperty("warning"))c.warning=double(a["warning"]);if(a.hasProperty("alarm"))c.alarm=double(a["alarm"]);if(a.hasProperty("hysteresis"))c.hysteresis=double(a["hysteresis"]);if(a.hasProperty("alarmEnabled"))c.alarmEnabled=bool(a["alarmEnabled"]);if(a.hasProperty("animate"))c.animate=bool(a["animate"]);if(a.hasProperty("alarmMetric"))c.alarmMetric=int(a["alarmMetric"]);if(a.hasProperty("alarmWeight"))c.alarmWeight=int(a["alarmWeight"]);s.cards.push_back(c);}return valid(s);
 }
 void notify(){auto copy=listeners;for(const auto& l:copy)if(listeners.count(l.first)&&l.second)l.second();}
 bool commit(State s,bool announce=true){
  if(!usable)return false;if(!valid(s)){error="Invalid meter configuration";return false;}
  juce::TemporaryFile temp(file);if(!temp.getFile().replaceWithText(juce::JSON::toString(encode(s)))||!temp.overwriteTargetFileWithTemporary()){error="No se pudieron guardar los medidores";return false;}
  data=std::move(s);error.clear();if(announce)notify();return true;
 }
public:
 explicit MeterState(juce::File preferences):file(std::move(preferences)){
  if(file.existsAsFile()){State loaded;if(decode(juce::JSON::parse(file),loaded))data=std::move(loaded);else{usable=false;error=juce::String::fromUTF8("Configuración de medidores inválida; archivo conservado");}}
  if(!file.existsAsFile()||!usable)for(int i=1;i<=4;++i)data.cards.push_back(card(i));
 }
 const State& state() const{return data;}
 const std::vector<Source>& sources() const{return inputs;}
 bool ready() const{return usable;}
 const juce::String& lastError() const{return error;}
 int listen(std::function<void()> f){int id=++nextListener;listeners[id]=std::move(f);return id;}
 void unlisten(int id){listeners.erase(id);}
 void setSources(std::vector<Source> sources){inputs=std::move(sources);notify();}
 const Card* find(const juce::String& id) const{for(const auto& c:data.cards)if(c.id==id)return &c;return nullptr;}
 bool add(){if(data.cards.size()>=10000){error="Meter configuration capacity reached";return false;}auto s=data;s.cards.push_back(card((int)s.cards.size()+1));s.rows=std::max(s.rows,((int)s.cards.size()+s.columns-1)/s.columns);return commit(s);}
 bool remove(const juce::String& id){auto s=data;auto old=s.cards.size();s.cards.erase(std::remove_if(s.cards.begin(),s.cards.end(),[&](const Card& c){return c.id==id;}),s.cards.end());return old!=s.cards.size()&&commit(s);}
 bool update(const Card& card){auto s=data;for(auto& c:s.cards)if(c.id==card.id){c=card;if(c.metric==1)c.weight=1;return commit(s);}return false;}
 bool move(const juce::String& id,int step){auto s=data;for(size_t i=0;i<s.cards.size();++i)if(s.cards[i].id==id){int target=(int)i+step;if(target<0||target>=(int)s.cards.size())return false;std::swap(s.cards[i],s.cards[(size_t)target]);return commit(s);}return false;}
 bool setGrid(int rows,int columns){auto s=data;s.rows=rows;s.columns=columns;return commit(s);}
 bool sidebar(bool meters){auto s=data;s.sidebarMeters=meters;return commit(s);}
 bool window(int x,int y,int width,int height){if(x==data.x&&y==data.y&&width==data.width&&height==data.height)return true;auto s=data;s.x=x;s.y=y;s.width=width;s.height=height;return commit(s,false);}
 static bool sameInput(const Card& c,const Source& s){return c.deviceKey==s.deviceKey&&c.channel==s.channel;}
};
