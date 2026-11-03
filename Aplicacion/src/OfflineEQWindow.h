#pragma once
#include "OfflineDSPCore.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>

class OfflineEQWindow : public juce::DocumentWindow {
public:
    OfflineEQWindow(int engine,OfflineDSPCore& core,std::function<void(int)> changed,
                    std::function<void(bool)> graphicChanged);
    void closeButtonPressed() override { setVisible(false); }
    void refresh();
private:
    struct Editor;
    Editor* editor=nullptr;
};
