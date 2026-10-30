#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class RTAPanel : public juce::Component, private juce::Timer {
public:
 struct Trace { juce::Colour colour; bool selected = false; std::vector<float> dbfs; };
 using DataProvider = std::function<std::vector<Trace>()>;
 explicit RTAPanel(DataProvider provider);
 void setActive(bool active);
 void visibilityChanged() override;
 void paint(juce::Graphics& g) override;
private:
 void timerCallback() override;
 DataProvider dataProvider;
 std::vector<Trace> traces;
};
