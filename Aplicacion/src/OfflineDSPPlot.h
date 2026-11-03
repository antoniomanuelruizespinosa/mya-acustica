#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include <optional>
#include "OfflineDSPCore.h"

// Displays only local, previously captured impulse data. It has no audio input.
class OfflineDSPPlot : public juce::Component {
public:
    void updateComponentLanguage(bool spanish) { spanishLanguage=spanish;repaint(); }
    void setPrediction(std::vector<float> source, std::vector<float> processed, double sampleRate);
    void clear();
    void setFrequencyView(bool enabled) { frequencyView=enabled;repaint(); }
    void setPhaseView(bool enabled) { phaseView=enabled;frequencyView=enabled;repaint(); }
    void setCore(const OfflineDSPCore* value) { core=value;repaint(); }
    void setEngine(int value) { engine=value;repaint(); }
    void setGraphicEQ(bool enabled) { graphicEQ=enabled;repaint(); }
    void setSumEnabled(bool enabled) { sumEnabled=enabled;repaint(); }
    bool isSumEnabled() const { return sumEnabled; }
    std::function<void(int,int,OfflineDSPCore::EQBand)> onBandDragged;
    struct CursorSample {
        bool frequencyDomain = false;
        double x = 0.0;
        std::optional<double> magnitudeDb, phaseDegrees, amplitude;
    };
    std::function<void(std::optional<CursorSample>)> onCursorReadout;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void paint(juce::Graphics&) override;

private:
    std::vector<float> original, prediction;
    double rate = 0.0;
    float impulsePeak=1.0e-9f;
    const OfflineDSPCore* core=nullptr;
    int engine=0,dragBand=-1;
    bool frequencyView=false,phaseView=false,graphicEQ=false,sumEnabled=false;
    bool spanishLanguage=true;
    bool cursorVisible=false;
    float cursorX=0.f,cursorY=0.f;
    std::optional<CursorSample> cursorSample;
    void clearCursor();
    void updateCursor(juce::Point<float> point);
};
