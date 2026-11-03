#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <map>
#include <vector>

class RTAPanel : public juce::Component, private juce::Timer {
public:
 struct Trace { int id=-1; juce::Colour colour; bool selected = false; double nyquistHz=20000.; std::vector<float> dbfs; };
 using DataProvider = std::function<std::vector<Trace>()>;
 explicit RTAPanel(DataProvider provider);
 void setActive(bool active);
 void setFrozen(bool shouldFreeze);
 void clearTraces();
 void visibilityChanged() override;
 void paint(juce::Graphics& g) override;
 void resized() override;
 void mouseMove(const juce::MouseEvent& event) override;
 void mouseExit(const juce::MouseEvent&) override;
 std::function<void(bool)> onResolutionChanged;
 int smoothingChoice() const { return smoothingMode; }
 bool multiWindow() const { return multiWindowMode; }
 void setDisplayOptions(bool multi, int smooth);
 void setWindowCount(int count);
 void updateComponentLanguage(bool spanish);
 int windowCountChoice() const { return windowCount.getSelectedId(); }
 void setLineThickness(float pixels);
 std::function<void(juce::String)> onCursorReadout;
private:
 void timerCallback() override;
 juce::Rectangle<int> plotArea() const;
 juce::Rectangle<int> spectrogramArea() const;
 const Trace* selectedTrace() const;
 void updateCursor(const Trace& trace,juce::Rectangle<int> area,double index);
 int peakIndex(const Trace& trace) const;
 void publishCursor();
 DataProvider dataProvider;
 std::vector<Trace> traces;
 std::map<int,std::vector<float>> history;
 juce::ComboBox speed;
 juce::ComboBox windowCount;
 juce::Slider upperLimit,lowerLimit;
 juce::ToggleButton peakTracker{"Peak Tracker"};
 std::vector<std::vector<float>> spectrogramHistory;
 size_t spectrogramHead=0;
 int spectrogramTraceId=-1;
 bool cursorVisible=false;
 float cursorX=0.f,cursorY=0.f;
 juce::String cursorReading;
 bool multiWindowMode=true;
 int smoothingMode=2;
 double lastUpdateMs=0;
 float lineThickness=2.f;
 bool spanishLanguage=true;
 bool frozen=false;
};
