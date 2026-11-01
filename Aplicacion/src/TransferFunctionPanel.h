#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <array>
#include <memory>
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
  bool bypassBlanking=false;
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
 void clearTraces();
 void visibilityChanged() override;
 void paint(juce::Graphics& graphics) override;
 void resized() override;
 std::function<void(bool)> onResolutionChanged;
 int smoothingChoice() const { return smoothingMode; }
 bool multiWindow() const { return multiWindowMode; }
 void setDisplayOptions(bool multi, int smooth);
 void setPlotLayout(juce::Rectangle<int> phase, juce::Rectangle<int> magnitude);
 void setPlotTransforms(const std::array<double,3>& panX,const std::array<double,3>& panY,
                        const std::array<double,3>& zoomX,const std::array<double,3>& zoomY);
 void setSelectedPlot(int plot);
 void setCoherenceDisplay(double thresholdPercent,bool squared,bool visible,double height);
 std::function<void(double)> onCoherenceThresholdChanged;
 std::function<void()> onCoherenceThresholdCommitted;

private:
 class PlotPane;
 class CoherenceHandle;
 void timerCallback() override;
 void paintPane(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawGrid(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawTraces(juce::Graphics& graphics,juce::Rectangle<float> bounds,bool phase) const;
 void drawCoherence(juce::Graphics& graphics,juce::Rectangle<float> bounds) const;
 float coherenceLineY(juce::Rectangle<float> bounds) const;
 void dragCoherenceHandle(float panelY);
 void updateCoherenceHandle();
 std::optional<float> frequencyPosition(double frequencyHz,juce::Rectangle<float> bounds,bool phase) const;
 float valuePosition(double value,juce::Rectangle<float> bounds,bool phase) const;

 DataProvider dataProvider;
 std::vector<Trace> traces;
 std::map<int,Trace> history;
 juce::ComboBox speed;
 std::unique_ptr<PlotPane> phasePane,magnitudePane;
 std::unique_ptr<CoherenceHandle> coherenceHandle;
 std::array<double,3> panX{},panY{};
 std::array<double,3> zoomX{{1.,1.,1.}},zoomY{{1.,1.,1.}};
 bool multiWindowMode=true;
 int smoothingMode=2;
 int selectedPlot=1;
 double lastUpdateMs=0;
 bool active=false;
 double coherenceThresholdPercent=0.;
 double coherenceHeight=.25;
 bool coherenceSquared=true,coherenceVisible=true;
};
