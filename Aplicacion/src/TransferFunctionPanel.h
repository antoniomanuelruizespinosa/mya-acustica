#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <map>
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
  int id=-1;
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
 void resized() override;
 std::function<void(bool)> onResolutionChanged;
 int smoothingChoice() const { return smoothingMode; }
 bool multiWindow() const { return multiWindowMode; }
 void setDisplayOptions(bool multi, int smooth);

private:
 void timerCallback() override;
 void drawGrid(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawTraces(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawCoherence(juce::Graphics& graphics,juce::Rectangle<float> bounds) const;
 static std::optional<float> frequencyPosition(double frequencyHz,juce::Rectangle<float> bounds);

 DataProvider dataProvider;
 std::vector<Trace> traces;
 std::map<int,Trace> history;
 juce::ComboBox speed;
 bool multiWindowMode=true;
 int smoothingMode=2;
 double lastUpdateMs=0;
 bool active=false;
};
