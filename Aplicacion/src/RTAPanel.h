#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <map>
#include <vector>

class RTAPanel : public juce::Component, private juce::Timer {
public:
 struct Trace { int id=-1; juce::Colour colour; bool selected = false; std::vector<float> dbfs; };
 using DataProvider = std::function<std::vector<Trace>()>;
 explicit RTAPanel(DataProvider provider);
 void setActive(bool active);
 void clearTraces();
 void visibilityChanged() override;
 void paint(juce::Graphics& g) override;
 void resized() override;
 std::function<void(bool)> onResolutionChanged;
 int smoothingChoice() const { return smoothingMode; }
 bool multiWindow() const { return multiWindowMode; }
 void setDisplayOptions(bool multi, int smooth);
private:
 void timerCallback() override;
 DataProvider dataProvider;
 std::vector<Trace> traces;
 std::map<int,std::vector<float>> history;
 juce::ComboBox speed;
 bool multiWindowMode=true;
 int smoothingMode=2;
 double lastUpdateMs=0;
};
