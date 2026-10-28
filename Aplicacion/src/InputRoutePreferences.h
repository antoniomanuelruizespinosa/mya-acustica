#pragma once
#include "InputLevelCore.h"
#include <juce_core/juce_core.h>
struct InputRoutePreferences {
 std::array<InputLevels::Route,4> routes{};
 juce::String outputDevice;int outputChannel=0;bool outputStored=false;
 juce::var encode()const{auto* object=new juce::DynamicObject();object->setProperty("schema",1);juce::Array<juce::var> list;for(const auto& r:routes){auto* row=new juce::DynamicObject();row->setProperty("device",juce::String::fromUTF8(r.device.c_str()));row->setProperty("mic",r.mic);row->setProperty("ref",r.ref);list.add(juce::var(row));}object->setProperty("routes",list);object->setProperty("outputDevice",outputDevice);object->setProperty("outputChannel",outputChannel);return juce::var(object);}
 static std::optional<InputRoutePreferences> read(const juce::File& file){auto v=juce::JSON::parse(file);if(!v.isObject()||int(v["schema"])!=1||!v["routes"].isArray()||v["routes"].getArray()->size()!=4)return {};InputRoutePreferences result;for(int i=0;i<4;++i){auto row=v["routes"].getArray()->getReference(i);if(!row["device"].isString()||!row["mic"].isInt()||!row["ref"].isInt())return {};int m=int(row["mic"]),r=int(row["ref"]);if(m<0||r<0||m>InputLevels::maxChannels||r>InputLevels::maxChannels)return {};result.routes[(size_t)i]={row["device"].toString().toStdString(),m,r};}if(v.hasProperty("outputDevice")||v.hasProperty("outputChannel")){if(!v["outputDevice"].isString()||!v["outputChannel"].isInt()||int(v["outputChannel"])<0||int(v["outputChannel"])>InputLevels::maxChannels)return {};result.outputDevice=v["outputDevice"].toString();result.outputChannel=int(v["outputChannel"]);result.outputStored=true;}return result;}
 bool save(const juce::File& file)const{juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(juce::JSON::toString(encode()))&&tmp.overwriteTargetFileWithTemporary();}
};
