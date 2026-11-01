#include "VirtualDSPPanel.h"
#include <algorithm>

using namespace juce;

struct VirtualDSPPanel::EngineRow : Component {
    int index = 0;
    Colour colour;
    Label name, gain, delay, time;
    TextButton polarity{String::fromUTF8("Ø")}, equalizer{"Equalizer"}, find{"Find"}, clear{"C"};
    ComboBox hpf, lpf;
    std::array<Label,6> filterFields;
    std::function<void(int)> findRequested;

    EngineRow(int number, const Engine& engine) : index(number), colour(engine.colour) {
        name.setText(engine.name.isEmpty() ? "Engine " + String(number + 1) : engine.name, dontSendNotification);
        gain.setText("0.00", dontSendNotification);
        delay.setText("0.00", dontSendNotification);
        time.setText("0.00", dontSendNotification);
        for (auto* label : {&name, &gain, &delay, &time}) {
            label->setColour(Label::textColourId, Colours::white);
            label->setJustificationType(Justification::centred);
            addAndMakeVisible(*label);
        }
        for (auto* choice : {&hpf, &lpf}) {
            choice->addItem("Ninguno", 1);
            choice->setSelectedId(1, dontSendNotification);
            choice->setEnabled(false);
            choice->setTooltip("Filtro pendiente del motor DSP");
            addAndMakeVisible(*choice);
        }
        for (auto& field : filterFields) {
            field.setText("—", dontSendNotification);
            field.setColour(Label::textColourId, Colours::grey);
            field.setJustificationType(Justification::centred);
            addAndMakeVisible(field);
        }
        for (auto* button : {&polarity, &equalizer, &find, &clear}) addAndMakeVisible(*button);
        polarity.setEnabled(false);
        equalizer.setEnabled(false);
        find.setEnabled(false);
        clear.setEnabled(false);
        for (auto* button : {&polarity, &equalizer, &clear}) button->setTooltip("Pendiente de procesamiento DSP por motor");
        find.onClick = [this] { if (findRequested) findRequested(index); };
    }

    void resized() override {
        int x = 34, h = getHeight() - 4;
        auto place = [&](Component& c, int w) { c.setBounds(x, 2, w, h); x += w + 3; };
        place(name, 106); place(gain, 52); place(polarity, 27); place(delay, 54);
        place(hpf, 92); for (int i = 0; i < 3; ++i) place(filterFields[(size_t)i], 38);
        place(equalizer, 112 + std::max(0, getWidth() - 970)); place(lpf, 92);
        for (int i = 3; i < 6; ++i) place(filterFields[(size_t)i], 38);
        place(find, 48); place(time, 48); place(clear, 26);
    }

    void paint(Graphics& g) override {
        g.fillAll(Colour(0xff080b0e));
        g.setColour(Colour(0xff253944)); g.drawRect(getLocalBounds(), 1);
        g.setColour(colour); g.fillRect(4, 4, 26, getHeight() - 8);
        g.setColour(Colours::black); g.setFont(12.f);
        g.drawText(String(index + 1), 4, 4, 26, getHeight() - 8, Justification::centred);
    }
};

VirtualDSPPanel::VirtualDSPPanel() {
    setOpaque(true);
    viewport.setViewedComponent(&rows, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(8);
    addAndMakeVisible(viewport);
    const char* names[] = {"Suma", "Snap All", "Export All", "Sinc All", "AVG", "Limpiar Delays", "Limpiar Find", "Limpiar DSP"};
    for (size_t i = 0; i < globalButtons.size(); ++i) {
        globalButtons[i].setButtonText(names[i]);
        globalButtons[i].setEnabled(i == 0 || i == 4);
        if (!globalButtons[i].isEnabled()) globalButtons[i].setTooltip("Comando pendiente de implementación");
        addAndMakeVisible(globalButtons[i]);
    }
    globalButtons[0].onClick = [this] { if (onSum) onSum(); };
    globalButtons[4].onClick = [this] { if (onAverage) onAverage(); };
}

VirtualDSPPanel::~VirtualDSPPanel() = default;

void VirtualDSPPanel::setEngines(const std::vector<Engine>& entries) {
    engines.clear();
    rows.removeAllChildren();
    for (size_t i = 0; i < entries.size(); ++i) {
        auto row = std::make_unique<EngineRow>((int)i, entries[i]);
        row->findRequested = [this](int index) { if (onFind) onFind(index); };
        rows.addAndMakeVisible(*row);
        engines.push_back(std::move(row));
    }
    resized();
}

void VirtualDSPPanel::setSelectedEngine(int index) { selectedEngine = index; }

void VirtualDSPPanel::resized() {
    auto table = getLocalBounds().reduced(4).removeFromLeft(std::max(200, getWidth() - actionsWidth - 12));
    viewport.setBounds(table.withTrimmedTop(headerHeight));
    rows.setSize(std::max(970, viewport.getWidth() - 12), std::max(viewport.getHeight(), (int)engines.size() * rowHeight));
    for (size_t i = 0; i < engines.size(); ++i) engines[i]->setBounds(0, (int)i * rowHeight, rows.getWidth(), rowHeight);
    auto actions = Rectangle<int>(getWidth() - actionsWidth - 4, 4, actionsWidth, getHeight() - 8);
    int cell = (actions.getWidth() - 8) / 3;
    int y = actions.getY() + 26;
    for (int i = 0; i < 6; ++i) globalButtons[(size_t)i].setBounds(actions.getX() + (i % 3) * (cell + 4), y + (i / 3) * 36, cell, 32);
    globalButtons[6].setBounds(actions.getX(), y + 72, (actions.getWidth() - 4) / 2, 32);
    globalButtons[7].setBounds(actions.getX() + (actions.getWidth() + 4) / 2, y + 72, (actions.getWidth() - 4) / 2, 32);
}

void VirtualDSPPanel::paint(Graphics& g) {
    g.fillAll(Colours::black);
    g.setColour(Colour(0xff284652)); g.drawRect(getLocalBounds(), 1);
    g.setColour(Colours::white); g.setFont(11.f);
    g.drawText("DSP", 12, 3, 40, 20, Justification::centredLeft);
    const String headings = "ENGINE                     GAIN    Ø     DLY          HPF      FREQ    ORD      Q             EQ                  LPF      FREQ    ORD      G          FIND      TIME      C";
    g.setColour(Colour(0xff9db5c4)); g.drawFittedText(headings, 10, 23, getWidth() - actionsWidth - 22, 15, Justification::centredLeft, 1);
    g.setColour(Colours::white); g.drawText("GLOBAL", getWidth() - actionsWidth + 3, 5, 90, 18, Justification::centredLeft);
}
