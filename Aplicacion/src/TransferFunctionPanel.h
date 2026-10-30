#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <optional>
#include <vector>

class TransferFunctionPanel : public juce::Component, private juce::Timer {
public:
 struct Sample {
  double frequencyHz=0.0;
  double magnitudeDb=0.0;
  double phaseDegrees=0.0;
  double coherence=0.0;
  bool valid=false;
 };
 struct Trace {
  juce::String name;
  juce::Colour colour;
  bool selected=false;
  std::vector<Sample> samples;
 };
 using DataProvider=std::function<std::vector<Trace>()>;

 explicit TransferFunctionPanel(DataProvider provider);
 ~TransferFunctionPanel() override;
 void setActive(bool active);
 void visibilityChanged() override;
 void paint(juce::Graphics& graphics) override;

private:
 void timerCallback() override;
 void drawGrid(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawTraces(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawCoherence(juce::Graphics& graphics,juce::Rectangle<float> bounds) const;
 static std::optional<float> frequencyPosition(double frequencyHz,juce::Rectangle<float> bounds);

 DataProvider dataProvider;
 std::vector<Trace> traces;
 bool active=false;
};
