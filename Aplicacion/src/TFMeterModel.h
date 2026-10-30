#pragma once
#include <juce_core/juce_core.h>
#include <optional>
#include <cmath>
#include <algorithm>

// Input meters use dBFS exclusively. No SPL calibration or alarm values enter here.
struct TFMeterSettings {
 bool peakOn=true,gloss=true;double holdSeconds=2.,orangeDbFS=-12.,redDbFS=-6.;
 bool valid() const{return std::isfinite(holdSeconds)&&holdSeconds>=.5&&holdSeconds<=60&&std::isfinite(orangeDbFS)&&std::isfinite(redDbFS)&&orangeDbFS>=-60&&orangeDbFS<redDbFS&&redDbFS<=0;}
 juce::var encode() const{auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("peakOn",peakOn);o->setProperty("gloss",gloss);o->setProperty("holdSeconds",holdSeconds);o->setProperty("orangeDbFS",orangeDbFS);o->setProperty("redDbFS",redDbFS);return juce::var(o);}
 static std::optional<TFMeterSettings> decode(const juce::var& v){
  auto numeric=[](const juce::var& value){return value.isDouble()||value.isInt()||value.isInt64();};
  if(!v.isObject()||int(v["schema"])!=1||!v["peakOn"].isBool()||!v["gloss"].isBool()||!numeric(v["holdSeconds"])||!numeric(v["orangeDbFS"])||!numeric(v["redDbFS"]))return {};
  TFMeterSettings s{bool(v["peakOn"]),bool(v["gloss"]),double(v["holdSeconds"]),double(v["orangeDbFS"]),double(v["redDbFS"])};return s.valid()?std::optional<TFMeterSettings>(s):std::nullopt;
 }

};
struct TFMeterChannel {
 static constexpr double floorDbFS=-60.;
 std::optional<double> level,peak;double peakAt=0;
 static double fraction(double db){return std::clamp((db-floorDbFS)/-floorDbFS,0.,1.);}
 static double position(double db,double left,double width){return left+width*fraction(db);}
 void reset(){level.reset();peak.reset();peakAt=0;}
 void update(std::optional<double> db,double now,const TFMeterSettings& s){
  if(!db||std::isnan(*db)||(std::isinf(*db)&&*db>0)){reset();return;}level=*db;
  if(!s.peakOn){peak.reset();return;}
  if(!peak||*db>=*peak||now-peakAt>=s.holdSeconds){peak=*db;peakAt=now;}
 }
 bool clipped()const{return level&&*level>=0.;}
};
