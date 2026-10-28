#pragma once
#include "LoopCalibrationCore.h"
#include "InputLevelCore.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

struct LoopCalibrationRecord {
 juce::String deviceKey;
 double sampleRate=0;
 int bufferSamples=0;
 int outputChannel=0,inputChannel=0;
 LoopCalibrationCore::Result result;
 bool matches(const juce::String& key,double rate,int buffer,int output,int input)const{return deviceKey==key&&rate>0&&std::abs(sampleRate-rate)<.01&&bufferSamples==buffer&&buffer>0&&outputChannel==output&&inputChannel==input;}
 bool sameRoute(const juce::String& key,int output,int input)const{return deviceKey==key&&outputChannel==output&&inputChannel==input;}
};
class LoopCalibrationStore {
 juce::File file;
 std::vector<LoopCalibrationRecord> saved;
 static bool valid(const LoopCalibrationRecord& r){return r.deviceKey.isNotEmpty()&&std::isfinite(r.sampleRate)&&r.sampleRate>0&&r.bufferSamples>0&&r.outputChannel>=1&&r.outputChannel<=InputLevels::maxChannels&&r.inputChannel>=1&&r.inputChannel<=InputLevels::maxChannels&&std::isfinite(r.result.delayMs)&&r.result.delayMs>=0&&std::isfinite(r.result.gainDb)&&std::abs(r.result.gainDb)<=24&&std::isfinite(r.result.correlation)&&r.result.correlation>=.7&&r.result.correlation<=1&&std::isfinite(r.result.outputDbfs)&&std::isfinite(r.result.inputDbfs);}
 static std::optional<LoopCalibrationRecord> decode(const juce::var& v){
  if(!v.isObject())return {};
  LoopCalibrationRecord r;r.deviceKey=v["deviceKey"].toString();r.sampleRate=double(v["sampleRate"]);r.bufferSamples=int(v["bufferSamples"]);r.outputChannel=int(v["outputChannel"]);r.inputChannel=int(v["inputChannel"]);r.result.delayMs=double(v["delayMs"]);r.result.gainDb=double(v["gainDb"]);r.result.correlation=double(v["correlation"]);r.result.outputDbfs=double(v["outputDbfs"]);r.result.inputDbfs=double(v["inputDbfs"]);
  if(!valid(r))return {};return r;
 }
 static juce::var encode(const LoopCalibrationRecord& r){
  juce::DynamicObject::Ptr obj=new juce::DynamicObject();obj->setProperty("deviceKey",r.deviceKey);obj->setProperty("sampleRate",r.sampleRate);obj->setProperty("bufferSamples",r.bufferSamples);obj->setProperty("outputChannel",r.outputChannel);obj->setProperty("inputChannel",r.inputChannel);obj->setProperty("delayMs",r.result.delayMs);obj->setProperty("gainDb",r.result.gainDb);obj->setProperty("correlation",r.result.correlation);obj->setProperty("outputDbfs",r.result.outputDbfs);obj->setProperty("inputDbfs",r.result.inputDbfs);return juce::var(obj.get());
 }
 bool persist(){
  if(!file.getParentDirectory().createDirectory())return false;
  if(saved.empty())return file.existsAsFile()?file.deleteFile():true;
  juce::DynamicObject::Ptr root=new juce::DynamicObject();root->setProperty("schema",3);juce::Array<juce::var> records;for(const auto& r:saved)records.add(encode(r));root->setProperty("records",records);
  juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(juce::JSON::toString(juce::var(root.get()),true))&&tmp.overwriteTargetFileWithTemporary();
 }
public:
 explicit LoopCalibrationStore(juce::File location):file(std::move(location)){
  auto v=juce::JSON::parse(file);if(!v.isObject())return;
  if(int(v["schema"])==2){if(auto r=decode(v))saved.push_back(*r);return;}
  if(int(v["schema"])!=3||!v["records"].isArray())return;
  for(const auto& item:*v["records"].getArray())if(auto r=decode(item)){auto existing=find(r->deviceKey,r->sampleRate,r->bufferSamples,r->outputChannel,r->inputChannel);if(!existing)saved.push_back(*r);}
 }
 size_t size()const{return saved.size();}
 const LoopCalibrationRecord* find(const juce::String& key,double rate,int buffer,int output,int input)const{for(auto it=saved.rbegin();it!=saved.rend();++it)if(it->matches(key,rate,buffer,output,input))return &*it;return nullptr;}
 const LoopCalibrationRecord* latestForRoute(const juce::String& key,int output,int input)const{for(auto it=saved.rbegin();it!=saved.rend();++it)if(it->sameRoute(key,output,input))return &*it;return nullptr;}
 bool set(const LoopCalibrationRecord& r){if(!valid(r))return false;auto prior=saved;saved.erase(std::remove_if(saved.begin(),saved.end(),[&](const auto& item){return item.matches(r.deviceKey,r.sampleRate,r.bufferSamples,r.outputChannel,r.inputChannel);}),saved.end());saved.push_back(r);if(persist())return true;saved=std::move(prior);return false;}
 bool clear(LoopCalibrationRecord target){auto prior=saved;saved.erase(std::remove_if(saved.begin(),saved.end(),[&](const auto& r){return r.matches(target.deviceKey,target.sampleRate,target.bufferSamples,target.outputChannel,target.inputChannel);}),saved.end());if(saved.size()==prior.size())return true;if(persist())return true;saved=std::move(prior);return false;}
};
