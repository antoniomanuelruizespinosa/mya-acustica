#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "OfflineDSPCore.h"
#include "OfflineEQWindow.h"
#include <array>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

// Visual DSP console. Filter and export controls remain disabled until their
// processing commands exist; this component never touches the audio callback.
class VirtualDSPPanel : public juce::Component {
public:
    struct Engine {
        juce::String name;
        juce::Colour colour;
    };

    VirtualDSPPanel();
    ~VirtualDSPPanel() override;
    void setEngines(const std::vector<Engine>& engines);
    void setSelectedEngine(int index);
    void updateComponentLanguage(bool spanish);
    int selectedEngineIndex() const { return selectedEngine; }
    bool loadImpulse(int index, std::vector<float> impulse, double sampleRate, std::size_t capacity = 0);
    void unloadImpulse(int index);
    OfflineDSPCore::State engineState(int index) const { return offlineCore.state(index); }
    std::vector<float> sourceImpulse(int index) const { return offlineCore.source(index); }
    std::vector<float> processedImpulse(int index) const { return offlineCore.render(index); }
    const OfflineDSPCore& core() const { return offlineCore; }
    bool setBand(int engine,int band,OfflineDSPCore::EQBand value);
    std::function<void(int)> onPredictionChanged;
    std::function<void(bool)> onGraphicEQChanged;
    std::function<void(int)> onFind;
    std::function<void()> onSum, onAverage;
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    struct EngineRow;
    juce::Viewport viewport;
    juce::Component rows;
    std::vector<std::unique_ptr<EngineRow>> engines;
    std::array<juce::TextButton,8> globalButtons;
    int selectedEngine = 0;
    bool spanishLanguage=true;
    OfflineDSPCore offlineCore;
    std::array<std::unique_ptr<OfflineEQWindow>,8> eqWindows;
    void openEqualizer(int index);
    static constexpr int rowHeight = 29;
    static constexpr int headerHeight = 30;
    static constexpr int actionsWidth = 0;
};

class VirtualDSPSidebar : public juce::Component {
public:
    VirtualDSPSidebar();
    ~VirtualDSPSidebar() override;
    void setEngines(const std::vector<VirtualDSPPanel::Engine>&);
    void updateComponentLanguage(bool spanish);
    void setInputChannels(const juce::StringArray& channels);
    void setRoute(int engine, int measurementInput, int referenceInput);
    std::pair<int,int> route(int engine) const;
    std::function<void()> onSum, onAverage;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct EngineCard;
    juce::Viewport engineViewport;
    juce::Component engineContent;
    std::vector<std::unique_ptr<EngineCard>> engineCards;
    juce::StringArray inputChannels;
    std::vector<std::pair<int,int>> engineRoutes;
    std::array<juce::TextButton,8> globalButtons;
    int cardHeight = 72;
    bool spanishLanguage=true;
};
