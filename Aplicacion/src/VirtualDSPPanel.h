#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <memory>
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
    static constexpr int rowHeight = 29;
    static constexpr int headerHeight = 30;
    static constexpr int actionsWidth = 0;
};

class VirtualDSPSidebar : public juce::Component {
public:
    VirtualDSPSidebar();
    ~VirtualDSPSidebar() override;
    void setEngines(const std::vector<VirtualDSPPanel::Engine>&);
    std::function<void()> onSum, onAverage;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct EngineCard;
    juce::Viewport engineViewport;
    juce::Component engineContent;
    std::vector<std::unique_ptr<EngineCard>> engineCards;
    std::array<juce::TextButton,8> globalButtons;
    int cardHeight = 62;
};
