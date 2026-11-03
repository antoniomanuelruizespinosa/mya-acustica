#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
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
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void paint(juce::Graphics&) override;

private:
    std::vector<float> original, prediction;
    double rate = 0.0;
    const OfflineDSPCore* core=nullptr;
    int engine=0,dragBand=-1;
    bool frequencyView=false,phaseView=false,graphicEQ=false,sumEnabled=false;
    bool spanishLanguage=true;
};
