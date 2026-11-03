#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

// Displays only local, previously captured impulse data. It has no audio input.
class OfflineDSPPlot : public juce::Component {
public:
    void setPrediction(std::vector<float> source, std::vector<float> processed, double sampleRate);
    void clear();
    void paint(juce::Graphics&) override;

private:
    std::vector<float> original, prediction;
    double rate = 0.0;
};
