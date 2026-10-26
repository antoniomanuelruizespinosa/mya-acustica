#pragma once
#include "CaptureLibrary.h"
#include "FractionalSmoothing.h"

// Reconstruct display from saved complex samples, never from the changing demo generator.
class CapturedTraceDisplay {
 struct Entry {CaptureLibrary::Trace trace;std::optional<FractionalSmoothing> smooth;};
 std::vector<Entry> traces;
public:
 void reload(const CaptureLibrary& library){
  traces.clear();if(library.state().view!=CaptureLibrary::Kind::SnapshotTF)return;
  for(const auto& n:library.state().nodes)if(n.kind==CaptureLibrary::Kind::SnapshotTF){auto trace=library.readTrace(n.id);if(trace)traces.push_back({std::move(*trace),std::nullopt});}
 }
 const CaptureLibrary::Node* find(const juce::String& id) const{for(const auto& e:traces)if(e.trace.node.id==id)return &e.trace.node;return nullptr;}
 std::vector<juce::String> visible() const{std::vector<juce::String> ids;for(const auto& e:traces)if(e.trace.node.visible)ids.push_back(e.trace.node.id);return ids;}
 std::optional<double> coherence(const juce::String& id,double frequency) const{for(const auto& e:traces)if(e.trace.node.id==id&&e.trace.node.visible){auto p=CaptureLibrary::interpolate(e.trace,frequency);return p?p->coherence:std::nullopt;}return std::nullopt;}
 std::optional<double> value(const juce::String& id,double frequency,int type,int smoothing,double threshold){
  if(type<1||type>2)return std::nullopt;
  for(auto& e:traces){if(e.trace.node.id==id&&e.trace.node.visible){
   auto raw=CaptureLibrary::interpolate(e.trace,frequency);
   if(!raw||(threshold>0&&(!raw->coherence||*raw->coherence*100<threshold)))return std::nullopt;
   auto h=raw->value;
   if(smoothing>1){
    if(!e.smooth){e.smooth.emplace();e.smooth->rebuild([&](double f){auto p=CaptureLibrary::interpolate(e.trace,f);return p?p->value:std::complex<double>(std::numeric_limits<double>::quiet_NaN(),0.);});}
    const double widths[]={48,24,12,6,3,1};h=e.smooth->at(frequency,widths[std::clamp(smoothing-2,0,5)],type==1);
   }
   if(!std::isfinite(h.real())||!std::isfinite(h.imag())||std::abs(h)<=1.e-15)return std::nullopt;
   return type==1?std::arg(h)/(2*std::acos(-1.)):20*std::log10(std::abs(h))/36.;
  }}
  return std::nullopt;
 }
};
