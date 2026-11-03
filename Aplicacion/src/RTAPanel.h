#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <map>
#include <vector>

class RTAPanel : public juce::Component, private juce::Timer {
public:
 struct Trace { int id=-1; juce::String name; juce::Colour colour; bool selected = false; double nyquistHz=20000.; std::vector<float> dbfs; };
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
 void mouseDown(const juce::MouseEvent& event) override;
 void mouseDrag(const juce::MouseEvent& event) override;
 void mouseUp(const juce::MouseEvent&) override;
 std::function<void(bool)> onResolutionChanged;
 std::function<void(int)> onSmoothingChanged;
 std::function<void(int)> onTemporalModeChanged;
 int temporalModeChoice() const { return speed.getSelectedId(); }
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
 void refreshMicrophoneChoices();
 void updateSpectrogramImage();
 void dragLimitTo(float y);
 DataProvider dataProvider;
 std::vector<Trace> traces;
 juce::ComboBox speed;
 juce::ComboBox windowCount;
 juce::ComboBox bandedStyle;
 juce::ComboBox smoothingChoiceBox,microphoneChoice;
 juce::String microphoneSignature;
 double upperLimit=-10.,lowerLimit=-90.;
 enum class DragLimit { None, Upper, Lower };
 DragLimit draggedLimit=DragLimit::None;
 juce::Image spectrogramImage;
 bool spectrogramDirty=true;
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
