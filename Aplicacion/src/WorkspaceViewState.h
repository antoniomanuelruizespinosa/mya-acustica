#pragma once
#include "TFMeterModel.h"
struct WorkspaceViewState {
 static constexpr int leftMin=148,leftMax=300,rightMin=216,rightMax=340;
 int leftWidth=148,rightWidth=216;bool leftVisible=true,rightVisible=true;TFMeterSettings meters;juce::String avgColour="ffffac24";
 void constrain(){leftWidth=std::clamp(leftWidth,leftMin,leftMax);rightWidth=std::clamp(rightWidth,rightMin,rightMax);}
 juce::var encode() const{auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("leftWidth",leftWidth);o->setProperty("rightWidth",rightWidth);o->setProperty("leftVisible",leftVisible);o->setProperty("rightVisible",rightVisible);o->setProperty("inputMeters",meters.encode());o->setProperty("avgColour",avgColour);return juce::var(o);}
 static std::optional<WorkspaceViewState> decode(const juce::var& v){if(!v.isObject()||int(v["schema"])!=1||!v["leftWidth"].isInt()||!v["rightWidth"].isInt()||!v["leftVisible"].isBool()||!v["rightVisible"].isBool())return {};auto m=TFMeterSettings::decode(v["inputMeters"]);if(!m)return {};WorkspaceViewState s{int(v["leftWidth"]),int(v["rightWidth"]),bool(v["leftVisible"]),bool(v["rightVisible"]),*m};if(v.hasProperty("avgColour")){if(!v["avgColour"].isString())return {};auto c=v["avgColour"].toString();if(c.length()!=8||!c.containsOnly("0123456789abcdefABCDEF"))return {};s.avgColour=c;}s.constrain();return s;}
 bool save(const juce::File& file) const{juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(juce::JSON::toString(encode()))&&tmp.overwriteTargetFileWithTemporary();}
 static std::optional<WorkspaceViewState> read(const juce::File& file){return decode(juce::JSON::parse(file));}
};
