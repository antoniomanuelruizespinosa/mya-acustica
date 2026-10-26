#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <optional>
struct GraphViewState {
 std::array<int,2> types{{1,2}};int irScale=0;bool linkPhaseMag=true,linkSpecified=true;
 juce::var encode() const{auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("irScale",irScale);o->setProperty("linkPhaseMag",linkPhaseMag);juce::Array<juce::var> panels;for(int type:types)panels.add(type);o->setProperty("panelTypes",panels);return juce::var(o);}
 static std::optional<GraphViewState> decode(const juce::var& v){
  if(!v.isObject()||int(v["schema"])!=1||!v["panelTypes"].isArray()||v["panelTypes"].getArray()->size()!=2)return std::nullopt;
  GraphViewState s;s.linkSpecified=v.hasProperty("linkPhaseMag");if(s.linkSpecified&&!v["linkPhaseMag"].isBool())return std::nullopt;s.linkPhaseMag=s.linkSpecified?bool(v["linkPhaseMag"]):true;s.irScale=int(v["irScale"]);if(s.irScale<0||s.irScale>2)return std::nullopt;
  for(int i=0;i<2;++i){int type=int(v["panelTypes"].getArray()->getReference(i));if(type!=1&&type!=2)return std::nullopt;s.types[(size_t)i]=type;}return s;
 }
 bool save(const juce::File& file) const{juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(juce::JSON::toString(encode()))&&tmp.overwriteTargetFileWithTemporary();}
 static std::optional<GraphViewState> read(const juce::File& file){return decode(juce::JSON::parse(file));}
};
