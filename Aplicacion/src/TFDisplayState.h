#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <optional>
#include <type_traits>
struct TFDisplayState {
 int frequencyGrid=1;bool showIR=true,linkPhaseMag=true,linkFrequencyZoom=false,allowNone=true,coherenceOn=true,coherenceSquared=true,coherenceFollowTrace=false;
 double foregroundWidth=2.,backgroundWidth=1.25,magMin=-18.,magMax=18.,magGrid=3.,magScroll=3.,magZoom=6.,phaseMin=-180.,phaseMax=180.,phaseGrid=30.,phaseScroll=30.,phaseZoom=60.,coherenceThreshold=0.,coherenceHeight=.25,coherenceWidth=1.;
 bool valid() const{
  for(double v:{foregroundWidth,backgroundWidth,magMin,magMax,magGrid,magScroll,magZoom,phaseMin,phaseMax,phaseGrid,phaseScroll,phaseZoom,coherenceThreshold,coherenceHeight,coherenceWidth})if(!std::isfinite(v))return false;
  return frequencyGrid>=1&&frequencyGrid<=4&&foregroundWidth>=.5&&foregroundWidth<=8&&backgroundWidth>=.5&&backgroundWidth<=8&&magMin>=-200&&magMax<=200&&magMax>magMin&&phaseMin>=-2000&&phaseMax<=2000&&phaseMax>phaseMin&&magGrid>=.5&&magGrid<=100&&phaseGrid>=5&&phaseGrid<=360&&magScroll>0&&magScroll<=100&&phaseScroll>0&&phaseScroll<=360&&magZoom>0&&magZoom<=100&&phaseZoom>0&&phaseZoom<=360&&coherenceThreshold>=0&&coherenceThreshold<=100&&(coherenceHeight==.25||coherenceHeight==.5)&&coherenceWidth>=.5&&coherenceWidth<=8;
 }
 bool ownCoherenceWidthEditable() const{return !coherenceFollowTrace;}
 double frequency(double u) const{return frequencyGrid==4?20.+u*19980.:20.*std::pow(1000.,u);}
 double coordinate(double f) const{return frequencyGrid==4?(f-20.)/19980.:std::log(f/20.)/std::log(1000.);}
 double minimum(int kind) const{return kind==1?phaseMin:magMin;}
 double maximum(int kind) const{return kind==1?phaseMax:magMax;}
 double span(int kind) const{return maximum(kind)-minimum(kind);}
 double normalize(double value,int kind) const{return (value-(maximum(kind)+minimum(kind))*.5)/span(kind);}
 double physical(double value,int kind) const{return value*span(kind)+(maximum(kind)+minimum(kind))*.5;}
 double coherence(double raw) const{return coherenceSquared?raw:std::sqrt(std::clamp(raw,0.,1.));}
 bool passes(std::optional<double> raw) const{return coherenceThreshold<=0||(raw&&std::isfinite(*raw)&&coherence(*raw)*100>=coherenceThreshold);}
 juce::var encode() const{
  auto* o=new juce::DynamicObject();o->setProperty("schema",1);
#define MYA_TF_WRITE(field) o->setProperty(#field,field)
  MYA_TF_WRITE(frequencyGrid);MYA_TF_WRITE(showIR);MYA_TF_WRITE(linkPhaseMag);MYA_TF_WRITE(linkFrequencyZoom);MYA_TF_WRITE(allowNone);MYA_TF_WRITE(coherenceOn);MYA_TF_WRITE(coherenceSquared);MYA_TF_WRITE(coherenceFollowTrace);
  MYA_TF_WRITE(foregroundWidth);MYA_TF_WRITE(backgroundWidth);MYA_TF_WRITE(magMin);MYA_TF_WRITE(magMax);MYA_TF_WRITE(magGrid);MYA_TF_WRITE(magScroll);MYA_TF_WRITE(magZoom);MYA_TF_WRITE(phaseMin);MYA_TF_WRITE(phaseMax);MYA_TF_WRITE(phaseGrid);MYA_TF_WRITE(phaseScroll);MYA_TF_WRITE(phaseZoom);MYA_TF_WRITE(coherenceThreshold);MYA_TF_WRITE(coherenceHeight);MYA_TF_WRITE(coherenceWidth);
#undef MYA_TF_WRITE
  return juce::var(o);
 }
 static std::optional<TFDisplayState> decode(const juce::var& v){
  if(!v.isObject()||int(v["schema"])!=1)return std::nullopt;TFDisplayState s;
#define MYA_TF_READ(field,type) if(v.hasProperty(#field)){if constexpr(std::is_same_v<type,bool>){if(!v[#field].isBool())return std::nullopt;}else{if(!v[#field].isDouble()&&!v[#field].isInt()&&!v[#field].isInt64())return std::nullopt;}s.field=(type)v[#field];}
  MYA_TF_READ(frequencyGrid,int);MYA_TF_READ(showIR,bool);MYA_TF_READ(linkPhaseMag,bool);MYA_TF_READ(linkFrequencyZoom,bool);MYA_TF_READ(allowNone,bool);MYA_TF_READ(coherenceOn,bool);MYA_TF_READ(coherenceSquared,bool);MYA_TF_READ(coherenceFollowTrace,bool);
  MYA_TF_READ(foregroundWidth,double);MYA_TF_READ(backgroundWidth,double);MYA_TF_READ(magMin,double);MYA_TF_READ(magMax,double);MYA_TF_READ(magGrid,double);MYA_TF_READ(magScroll,double);MYA_TF_READ(magZoom,double);MYA_TF_READ(phaseMin,double);MYA_TF_READ(phaseMax,double);MYA_TF_READ(phaseGrid,double);MYA_TF_READ(phaseScroll,double);MYA_TF_READ(phaseZoom,double);MYA_TF_READ(coherenceThreshold,double);MYA_TF_READ(coherenceHeight,double);MYA_TF_READ(coherenceWidth,double);
#undef MYA_TF_READ
  return s.valid()?std::optional<TFDisplayState>(s):std::nullopt;
 }
 bool save(const juce::File& file) const{if(!valid())return false;juce::TemporaryFile tmp(file);return tmp.getFile().replaceWithText(juce::JSON::toString(encode()))&&tmp.overwriteTargetFileWithTemporary();}
 static std::optional<TFDisplayState> read(const juce::File& file){return decode(juce::JSON::parse(file));}
};
