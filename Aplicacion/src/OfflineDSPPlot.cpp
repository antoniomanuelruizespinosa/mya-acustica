#include "OfflineDSPPlot.h"
#include <algorithm>
#include <cmath>

void OfflineDSPPlot::setPrediction(std::vector<float> source, std::vector<float> processed, double sampleRate) {
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0 || source.empty() || processed.empty()) {
        clear();
        return;
    }
    original = std::move(source);
    prediction = std::move(processed);
    rate = sampleRate;
    repaint();
}

void OfflineDSPPlot::clear() {
    original.clear();
    prediction.clear();
    rate = 0.0;
    repaint();
}

void OfflineDSPPlot::paint(juce::Graphics& g) {
    using namespace juce;
    g.fillAll(Colours::black);
    auto plot = getLocalBounds().reduced(30, 18).toFloat();
    if (plot.isEmpty()) return;
    g.setColour(Colour(0xff142b38));
    for (int i = 0; i <= 8; ++i) {
        const float x = plot.getX() + plot.getWidth() * (float)i / 8.0f;
        g.drawVerticalLine(roundToInt(x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 4; ++i) {
        const float y = plot.getY() + plot.getHeight() * (float)i / 4.0f;
        g.drawHorizontalLine(roundToInt(y), plot.getX(), plot.getRight());
    }
    g.setColour(Colour(0xffdcebf0));
    g.setFont(12.0f);
    if (prediction.empty() || rate <= 0.0) {
        g.drawText("IR - SIN MEDICIÓN", plot.toNearestInt(), Justification::centred);
        return;
    }
    const auto count = prediction.size();
    float peak = 1.0e-9f;
    for (float sample : original) peak = std::max(peak, std::abs(sample));
    for (float sample : prediction) peak = std::max(peak, std::abs(sample));
    auto draw = [&](const std::vector<float>& samples, Colour colour) {
        Path path;
        for (std::size_t i = 0; i < count; ++i) {
            const float value = i < samples.size() ? samples[i] : 0.0f;
            const float x = plot.getX() + plot.getWidth() * (float)i / (float)std::max<std::size_t>(1, count - 1);
            const float y = plot.getCentreY() - (value / peak) * plot.getHeight() * 0.45f;
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(colour);
        g.strokePath(path, PathStrokeType(1.5f));
    };
    draw(original, Colour(0xff19d7ea));
    draw(prediction, Colour(0xff6df23d));
    g.setColour(Colour(0xffdcebf0));
    g.drawText("IR offline · " + String((double)count * 1000.0 / rate, 2) + " ms", 34, 1, getWidth() - 68, 16, Justification::centredLeft);
}
