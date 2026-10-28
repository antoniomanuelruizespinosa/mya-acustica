#pragma once
#include "RmsCalibrationCore.h"
#include <juce_core/juce_core.h>
#include <set>
class SplCalibrationStore {
public:
 struct Record {juce::String deviceKey,backend,device,microphone,gainDeclared,date;int channel=0;double reference=0,digital=0,offset=0,seconds=0,range=0;};
private:
 juce::File file;std::vector<Record> records;std::set<std::pair<juce::String,int>> acknowledged;juce::String error;bool usable=true;
 static bool valid(const Record& r){return r.deviceKey==r.backend+" / "+r.device&&r.backend.isNotEmpty()&&r.device.isNotEmpty()&&r.microphone.trim().isNotEmpty()&&r.gainDeclared.trim().isNotEmpty()&&r.date.isNotEmpty()&&r.channel>=1&&r.channel<=InputLevels::maxChannels&&std::isfinite(r.reference)&&r.reference>=0&&r.reference<=180&&std::isfinite(r.digital)&&r.digital> -100&&r.digital<0&&std::isfinite(r.offset)&&std::abs(r.offset-(r.reference-r.digital))<1.e-8&&std::isfinite(r.seconds)&&r.seconds>=5&&std::isfinite(r.range)&&r.range>=0&&r.range<=RmsCalibration::stabilityDb;}
 static juce::var encode(const std::vector<Record>& list){auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("convention","rms-sample-power-v1");juce::Array<juce::var> rows;for(const auto& r:list){auto* a=new juce::DynamicObject();a->setProperty("deviceKey",r.deviceKey);a->setProperty("backend",r.backend);a->setProperty("device",r.device);a->setProperty("channel",r.channel);a->setProperty("microphone",r.microphone);a->setProperty("gainDeclared",r.gainDeclared);a->setProperty("dateISO8601",r.date);a->setProperty("referenceDb",r.reference);a->setProperty("digitalRmsDbFS",r.digital);a->setProperty("offsetDb",r.offset);a->setProperty("durationSeconds",r.seconds);a->setProperty("rangeDb",r.range);a->setProperty("microphoneCurveApplied",false);a->setProperty("hardwareGainObserved",false);rows.add(juce::var(a));}o->setProperty("records",rows);return juce::var(o);}
 bool read(){auto v=juce::JSON::parse(file);if(!v.isObject()||!v["schema"].isInt()||int(v["schema"])!=1||v["convention"].toString()!="rms-sample-power-v1"||!v["records"].isArray())return false;std::vector<Record> list;std::set<std::pair<juce::String,int>> keys;for(const auto& a:*v["records"].getArray()){if(!a.isObject()||!a["channel"].isInt())return false;for(auto flag:{"microphoneCurveApplied","hardwareGainObserved"})if(a.hasProperty(flag)&&(!a[flag].isBool()||bool(a[flag])))return false;for(auto key:{"deviceKey","backend","device","microphone","gainDeclared","dateISO8601"})if(!a[key].isString())return false;for(auto key:{"referenceDb","digitalRmsDbFS","offsetDb","durationSeconds","rangeDb"})if(!a[key].isDouble()&&!a[key].isInt()&&!a[key].isInt64())return false;Record r{a["deviceKey"].toString(),a["backend"].toString(),a["device"].toString(),a["microphone"].toString(),a["gainDeclared"].toString(),a["dateISO8601"].toString(),int(a["channel"]),double(a["referenceDb"]),double(a["digitalRmsDbFS"]),double(a["offsetDb"]),double(a["durationSeconds"]),double(a["rangeDb"])};if(!valid(r)||!keys.insert({r.deviceKey,r.channel}).second)return false;list.push_back(r);}records=std::move(list);return true;}
public:
 explicit SplCalibrationStore(juce::File path):file(std::move(path)){if(file.existsAsFile()&&!read()){usable=false;error="Calibraciones invalidas; archivo conservado";}}
 const Record* find(const juce::String& key,int channel)const{for(const auto& r:records)if(r.deviceKey==key&&r.channel==channel)return &r;return nullptr;}
 bool ready()const{return usable;}const juce::String& lastError()const{return error;}
 bool isAcknowledged(const juce::String& key,int channel)const{return acknowledged.count({key,channel})!=0;}
 void acknowledge(const juce::String& key,int channel,bool yes){if(yes&&find(key,channel))acknowledged.insert({key,channel});else acknowledged.erase({key,channel});}
 void clearAcknowledgements(){acknowledged.clear();}
 std::optional<double> spl(const juce::String& key,int channel,std::optional<double> digital,bool clipped)const{auto* r=find(key,channel);if(!r||!isAcknowledged(key,channel)||!digital||!std::isfinite(*digital)||clipped)return {};return *digital+r->offset;}
 bool confirm(const Record& r,RmsCalibration::Session& capture){if(!capture.validatePending())return false;if(!usable||!capture.confirmable()||capture.device!=r.deviceKey.toStdString()||capture.channel!=r.channel||std::abs(capture.reference-r.reference)>1.e-8||std::abs(*capture.offset-r.offset)>1.e-8||std::abs(*capture.measured-r.digital)>1.e-8||std::abs((double)capture.samples/capture.rate-r.seconds)>1.e-8||std::abs((capture.maximum-capture.minimum)-r.range)>1.e-8||!valid(r)){error="Resultado no valido; offset anterior conservado";return false;}auto next=records;auto it=std::find_if(next.begin(),next.end(),[&](const Record& a){return a.deviceKey==r.deviceKey&&a.channel==r.channel;});if(it==next.end())next.push_back(r);else *it=r;juce::TemporaryFile temp(file);
  // Do not use File::replaceWithText: JUCE 8.0.12 ignores appendText failure inside
  // that helper and asserts when its nested temporary file does not exist.
  {
   juce::FileOutputStream output(temp.getFile());
   auto json=juce::JSON::toString(encode(next));
   if(!output.openedOk()||!output.writeText(json,false,false,"\n")){error="No se pudo escribir calibracion; offset anterior conservado";return false;}
   output.flush();
   if(output.getStatus().failed()){error="No se pudo completar escritura; offset anterior conservado";return false;}
  } // Close the stream before replacing the target, including on Windows.
  if(!temp.getFile().existsAsFile()||!temp.overwriteTargetFileWithTemporary()){error="No se pudo reemplazar calibracion; offset anterior conservado";return false;}records=std::move(next);acknowledge(r.deviceKey,r.channel,true);error.clear();return true;}
};
