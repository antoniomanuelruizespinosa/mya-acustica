#include "VirtualDSPPanel.h"
#include <algorithm>

using namespace juce;

struct VirtualDSPPanel::EngineRow : Component {
    int index = 0;
    Colour colour;
    Label name, time;
    Slider gain, delay;
    TextButton polarity{String::fromUTF8("\xC3\x98")}, equalizer{"Equalizer"}, find{"Find"}, clear{"C"};
    ComboBox hpf, lpf;
    std::array<Label,6> filterFields;
    std::function<void(int)> findRequested;
    std::function<void(int)> selected;
    std::function<void(int,double)> gainChanged, delayChanged;
    std::function<void(int,bool)> polarityChanged;
    std::function<void(int,bool,OfflineDSPCore::Crossover)> crossoverChanged;
    std::function<void(int)> equalizerRequested;
    bool isSelected = false;

    EngineRow(int number, const Engine& engine) : index(number), colour(engine.colour) {
        name.setText(engine.name.isEmpty() ? "Engine " + String(number + 1) : engine.name, dontSendNotification);
        time.setText("0.00", dontSendNotification);
        for (auto* label : {&name, &time}) {
            label->setColour(Label::textColourId, Colours::white);
            label->setColour(Label::backgroundColourId, Colour(0xff111a20));
            label->setColour(Label::outlineColourId, Colour(0xff44545e));
            label->setFont(Font(FontOptions(11.f)));
            label->setJustificationType(Justification::centred);
            addAndMakeVisible(*label);
        }
        name.setInterceptsMouseClicks(false, false);
        name.setTooltip("Seleccionar esta vía DSP para cargar una captura");
        for (auto* slider : {&gain, &delay}) {
            slider->setSliderStyle(Slider::IncDecButtons);
            slider->setTextBoxStyle(Slider::TextBoxLeft, false, 48, 22);
            slider->setColour(Slider::textBoxTextColourId, Colours::white);
            slider->setColour(Slider::textBoxBackgroundColourId, Colour(0xff111a20));
            slider->setColour(Slider::textBoxOutlineColourId, Colour(0xff44545e));
            slider->setEnabled(false);
            addAndMakeVisible(*slider);
        }
        gain.setRange(-30.0, 30.0, 0.5);
        delay.setRange(0.0, 1.0, 1.0);
        gain.onValueChange = [this] { if (gainChanged) gainChanged(index, gain.getValue()); };
        delay.onValueChange = [this] { if (delayChanged) delayChanged(index, delay.getValue()); };
        for (auto* choice : {&hpf, &lpf}) {
            choice->addItem("Ninguno", 1);
            for(int order=1;order<=8;++order)choice->addItem("Butter "+String(order),order+1);
            for(int order=2;order<=8;order+=2)choice->addItem("LR "+String(order),9+order/2);
            for(int order=1;order<=8;++order)choice->addItem("Bessel "+String(order),13+order);
            choice->setSelectedId(1, dontSendNotification);
            choice->setEnabled(false);
            addAndMakeVisible(*choice);
        }
        auto choiceChanged=[this](bool high){
            auto& box=high?hpf:lpf;const int id=box.getSelectedId();
            OfflineDSPCore::Crossover c;
            if(id>=2&&id<=9){c.family=OfflineDSPFilters::Family::Butterworth;c.order=id-1;}
            else if(id>=10&&id<=13){c.family=OfflineDSPFilters::Family::LinkwitzRiley;c.order=(id-9)*2;}
            else if(id>=14&&id<=21){c.family=OfflineDSPFilters::Family::Bessel;c.order=id-13;}
            c.frequency=filterFields[(size_t)(high?0:3)].getText().getDoubleValue();
            if(crossoverChanged)crossoverChanged(index,high,c);
        };
        hpf.onChange=[choiceChanged]{choiceChanged(true);};lpf.onChange=[choiceChanged]{choiceChanged(false);};
        for (size_t i = 0; i < filterFields.size(); ++i) {
            auto& field = filterFields[i];
            field.setText(i % 3 == 0 ? "1000" : i % 3 == 1 ? "1" : "1.0", dontSendNotification);
            field.setColour(Label::textColourId, Colours::grey);
            field.setColour(Label::backgroundColourId, Colour(0xff111a20));
            field.setColour(Label::outlineColourId, Colour(0xff44545e));
            field.setFont(Font(FontOptions(11.f)));
            field.setJustificationType(Justification::centred);
            addAndMakeVisible(field);
        }
        for(int base:{0,3}){filterFields[(size_t)base].setEditable(true,false,false);
            filterFields[(size_t)base].onTextChange=[choiceChanged,base]{choiceChanged(base==0);};}
        for (auto* button : {&polarity, &equalizer, &find, &clear}) addAndMakeVisible(*button);
        polarity.setEnabled(false);
        equalizer.setEnabled(false);
        find.setEnabled(false);
        clear.setEnabled(false);
        polarity.setClickingTogglesState(true);
        polarity.onClick = [this] { if (polarityChanged) polarityChanged(index, polarity.getToggleState()); };
        for (auto* button : {&equalizer, &clear}) button->setTooltip("Pendiente de procesamiento DSP por motor");
        find.onClick = [this] { if (findRequested) findRequested(index); };
        equalizer.onClick=[this]{if(equalizerRequested)equalizerRequested(index);};
    }

    void setOfflineState(const OfflineDSPCore::State& state) {
        gain.setEnabled(state.loaded);
        delay.setEnabled(state.loaded);
        polarity.setEnabled(state.loaded);
        hpf.setEnabled(state.loaded);lpf.setEnabled(state.loaded);equalizer.setEnabled(state.loaded);
        gain.setValue(state.gainDb, dontSendNotification);
        polarity.setToggleState(state.inverted, dontSendNotification);
        if (state.loaded) {
            const double step = 1000.0 / state.sampleRate;
            delay.setRange(0.0, static_cast<double>(state.capacity - 1) * step, step);
        }
        delay.setValue(state.delayMs, dontSendNotification);
    }
    void setCrossoverState(const OfflineDSPCore::Crossover& high,const OfflineDSPCore::Crossover& low){
        auto display=[this](ComboBox& choice,int base,const OfflineDSPCore::Crossover& c){
            int id=1;
            if(c.family==OfflineDSPFilters::Family::Butterworth)id=c.order+1;
            else if(c.family==OfflineDSPFilters::Family::LinkwitzRiley)id=9+c.order/2;
            else if(c.family==OfflineDSPFilters::Family::Bessel)id=13+c.order;
            choice.setSelectedId(id,dontSendNotification);
            filterFields[(size_t)base].setText(String(c.frequency,0),dontSendNotification);
            filterFields[(size_t)base+1].setText(c.family==OfflineDSPFilters::Family::None?"--":String(c.order),dontSendNotification);
            filterFields[(size_t)base].setEnabled(true);
        };
        display(hpf,0,high);display(lpf,3,low);
    }

    void resized() override {
        const double scale = getWidth() / 970.0;
        int x = roundToInt(34 * scale), h = getHeight() - 4;
        auto place = [&](Component& c, int w) { int scaled = roundToInt(w * scale); c.setBounds(x, 2, scaled, h); x += scaled + roundToInt(3 * scale); };
        place(name, 106); place(gain, 52); place(polarity, 27); place(delay, 54);
        place(hpf, 92); for (int i = 0; i < 3; ++i) place(filterFields[(size_t)i], 38);
        place(equalizer, 112); place(lpf, 92);
        for (int i = 3; i < 6; ++i) place(filterFields[(size_t)i], 38);
        place(find, 48); place(time, 48); place(clear, 26);
    }

    void mouseDown(const MouseEvent&) override {
        if (selected) selected(index);
    }

    void paint(Graphics& g) override {
        g.fillAll(Colour(0xff080b0e));
        g.setColour(isSelected ? Colour(0xff00d5e8) : Colour(0xff253944));
        g.drawRect(getLocalBounds(), isSelected ? 2 : 1);
        int patch = roundToInt(26 * getWidth() / 970.0);
        g.setColour(colour); g.fillRect(4, 4, patch, getHeight() - 8);
        g.setColour(Colours::black); g.setFont(12.f);
        g.drawText(String(index + 1), 4, 4, patch, getHeight() - 8, Justification::centred);
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
        if (!globalButtons[i].isEnabled()) globalButtons[i].setTooltip("Comando pendiente de implementacion");
        addChildComponent(globalButtons[i]);
    }
    globalButtons[0].onClick = [this] { if (onSum) onSum(); };
    globalButtons[4].onClick = [this] { if (onAverage) onAverage(); };
}

VirtualDSPPanel::~VirtualDSPPanel() = default;

void VirtualDSPPanel::setEngines(const std::vector<Engine>& entries) {
    engines.clear();
    rows.removeAllChildren();
    static const std::array<Colour,8> defaults = { Colour(0xff20b8ff), Colour(0xff28ca67),
        Colour(0xffedb52b), Colour(0xffed6932), Colour(0xff1ed8db), Colour(0xffe33bce),
        Colour(0xffd2ca54), Colour(0xffe43751) };
    for (int i = 0; i < OfflineDSPCore::channelCount; ++i) {
        Engine entry = (size_t)i < entries.size() ? entries[(size_t)i] : Engine{"Engine " + String(i + 1), defaults[(size_t)i]};
        auto row = std::make_unique<EngineRow>(i, entry);
        row->findRequested = [this](int index) { if (onFind) onFind(index); };
        row->selected = [this](int index) { setSelectedEngine(index); };
        row->gainChanged = [this](int index, double value) {
            if (offlineCore.setGainDb(index, value)) {
                engines[(size_t)index]->setOfflineState(offlineCore.state(index));
                if (onPredictionChanged) onPredictionChanged(index);
            }
        };
        row->delayChanged = [this](int index, double value) {
            if (offlineCore.setDelayMs(index, value)) {
                engines[(size_t)index]->setOfflineState(offlineCore.state(index));
                if (onPredictionChanged) onPredictionChanged(index);
            }
        };
        row->polarityChanged = [this](int index, bool value) {
            if (offlineCore.setPolarity(index, value) && onPredictionChanged) onPredictionChanged(index);
        };
        row->crossoverChanged=[this](int index,bool high,OfflineDSPCore::Crossover value){
            offlineCore.setCrossover(index,high,value);
            engines[(size_t)index]->setCrossoverState(offlineCore.crossover(index,true),offlineCore.crossover(index,false));
            if(onPredictionChanged)onPredictionChanged(index);
        };
        row->equalizerRequested=[this](int index){openEqualizer(index);};
        row->setOfflineState(offlineCore.state(i));
        row->setCrossoverState(offlineCore.crossover(i,true),offlineCore.crossover(i,false));
        row->isSelected = i == selectedEngine;
        rows.addAndMakeVisible(*row);
        engines.push_back(std::move(row));
    }
    resized();
}

void VirtualDSPPanel::setSelectedEngine(int index) {
    if (index < 0 || index >= OfflineDSPCore::channelCount) return;
    selectedEngine = index;
    for (size_t i = 0; i < engines.size(); ++i) {
        engines[i]->isSelected = (int)i == index;
        engines[i]->repaint();
    }
    if (onPredictionChanged) onPredictionChanged(index);
}

bool VirtualDSPPanel::loadImpulse(int index, std::vector<float> impulse, double sampleRate, std::size_t capacity) {
    if (!offlineCore.load(index, std::move(impulse), sampleRate, capacity)) return false;
    if ((size_t)index < engines.size()){engines[(size_t)index]->setOfflineState(offlineCore.state(index));engines[(size_t)index]->setCrossoverState(offlineCore.crossover(index,true),offlineCore.crossover(index,false));}
    if (onPredictionChanged) onPredictionChanged(index);
    return true;
}

void VirtualDSPPanel::unloadImpulse(int index) {
    offlineCore.unload(index);
    if (index >= 0 && (size_t)index < engines.size()) engines[(size_t)index]->setOfflineState(offlineCore.state(index));
    if (onPredictionChanged) onPredictionChanged(index);
}

bool VirtualDSPPanel::setBand(int engine,int band,OfflineDSPCore::EQBand value){
    if(!offlineCore.setBand(engine,band,value))return false;
    if(eqWindows[(size_t)engine])eqWindows[(size_t)engine]->refresh();
    if(onPredictionChanged)onPredictionChanged(engine);
    return true;
}

void VirtualDSPPanel::openEqualizer(int index){
    if(index<0||index>=OfflineDSPCore::channelCount||!offlineCore.state(index).loaded)return;
    setSelectedEngine(index);
    auto& window=eqWindows[(size_t)index];
    if(!window)window=std::make_unique<OfflineEQWindow>(index,offlineCore,
        [this](int channel){if(onPredictionChanged)onPredictionChanged(channel);},
        [this](bool enabled){if(onGraphicEQChanged)onGraphicEQChanged(enabled);});
    window->refresh();window->setVisible(true);window->toFront(true);
}

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
    g.setColour(Colour(0xffdce8ee)); g.setFont(Font(FontOptions(12.f)).boldened());
    const double scale = rows.getWidth() / 970.0;
    auto heading = [&](const String& label, int x, int width) {
        g.drawText(label, roundToInt(4 + x * scale), 23, roundToInt(width * scale), 15, Justification::centred, false);
    };
    heading("ENGINE", 38, 106); heading("GAIN", 147, 52); heading(String::fromUTF8("\xC3\x98"), 202, 27);
    heading("DLY", 232, 54); heading("HPF", 289, 92);
    heading("FREQ", 384, 38); heading("ORD", 425, 38); heading("Q", 466, 38);
    heading("EQ", 507, 112); heading("LPF", 622, 92);
    heading("FREQ", 717, 38); heading("ORD", 758, 38);
    heading("Q", 799, 38); heading("FIND", 840, 48);
    heading("TIME", 891, 48); heading("C", 942, 26);
}

struct VirtualDSPSidebar::EngineCard : Component {
    int number;
    Colour colour;
    ComboBox measurement, reference;
    std::function<void(int,int)> routeChanged;
    EngineCard(int index, Colour engineColour) : number(index + 1), colour(engineColour) {
        for (auto* choice : {&measurement, &reference}) {
            choice->setTooltip("Entrada física asignada a este motor DSP; no modifica Mya Analyzer");
            addAndMakeVisible(*choice);
        }
        measurement.onChange = [this] { if (routeChanged) routeChanged(measurement.getSelectedId(), reference.getSelectedId()); };
        reference.onChange = [this] { if (routeChanged) routeChanged(measurement.getSelectedId(), reference.getSelectedId()); };
    }
    void setChannels(const StringArray& channels, int mic, int ref) {
        for (auto* choice : {&measurement, &reference}) {
            choice->clear(dontSendNotification);
            choice->addItem("--", 1);
            for (int i = 0; i < channels.size(); ++i) choice->addItem(String(i + 1), i + 2);
            choice->setEnabled(!channels.isEmpty());
        }
        measurement.setSelectedId(mic > 0 && mic <= channels.size() ? mic + 1 : 1, dontSendNotification);
        reference.setSelectedId(ref > 0 && ref <= channels.size() ? ref + 1 : 1, dontSendNotification);
    }
    void resized() override {
        const int boxWidth = 42;
        measurement.setBounds(getWidth() - 2 * boxWidth - 8, 26, boxWidth, 25);
        reference.setBounds(getWidth() - boxWidth - 4, 26, boxWidth, 25);
    }
    void paint(Graphics& g) override {
        g.fillAll(Colour(0xff141c22));
        g.setColour(Colour(0xff52616a)); g.drawRect(getLocalBounds(), 1);
        g.setColour(colour); g.fillRect(5, 6, 24, 17);
        g.setFont(11.f); g.drawText("Engine " + String(number), 35, 4, getWidth() - 125, 20, Justification::centredLeft);
        const int meterRight = getWidth() - 105;
        g.setColour(Colour(0xffb7c8d1)); g.setFont(10.f);
        g.drawText("M", getWidth() - 2 * 42 - 8, 8, 42, 15, Justification::centred);
        g.drawText("R", getWidth() - 42 - 4, 8, 42, 15, Justification::centred);
        for (int row = 0; row < 2; ++row) {
            const int y = 29 + row * 19;
            g.setColour(Colours::white); g.drawText(row == 0 ? "M" : "R", 6, y - 2, 13, 13, Justification::centred);
            g.setColour(Colour(0xff0a0d10)); g.fillRect(22, y, std::max(10, meterRight - 22), 11);
            g.setColour(Colour(0xff4a5a62)); g.drawRect(22, y, std::max(10, meterRight - 22), 11, 1);
            g.setColour(Colour(0xffb7c8d1)); g.setFont(9.f);
            g.drawText("-- dBFS", meterRight - 50, y - 2, 48, 11, Justification::centredRight);
        }
    }
};

VirtualDSPSidebar::VirtualDSPSidebar() {
    setOpaque(true);
    engineViewport.setViewedComponent(&engineContent, false);
    engineViewport.setScrollBarsShown(true, false);
    engineViewport.setScrollBarThickness(7);
    addAndMakeVisible(engineViewport);
    const char* labels[] = {"Suma", "Snap All", "Export All", "Sinc All", "AVG", "Limpiar Delays", "Limpiar Find", "Limpiar DSP"};
    for (size_t i = 0; i < globalButtons.size(); ++i) {
        auto& button = globalButtons[i];
        button.setButtonText(labels[i]);
        button.setEnabled(i == 0 || i == 4);
        if (!button.isEnabled()) button.setTooltip("Comando pendiente de implementacion");
        addAndMakeVisible(button);
    }
    globalButtons[0].onClick = [this] { if (onSum) onSum(); };
    globalButtons[4].onClick = [this] { if (onAverage) onAverage(); };
}

VirtualDSPSidebar::~VirtualDSPSidebar() = default;

void VirtualDSPSidebar::setEngines(const std::vector<VirtualDSPPanel::Engine>& entries) {
    engineCards.clear(); engineContent.removeAllChildren();
    engineRoutes.resize(entries.size(), {0, 0});
    for (size_t i = 0; i < entries.size(); ++i) {
        auto card = std::make_unique<EngineCard>((int)i, entries[i].colour);
        card->routeChanged = [this, i](int mic, int ref) { engineRoutes[i] = {mic > 1 ? mic - 1 : 0, ref > 1 ? ref - 1 : 0}; };
        card->setChannels(inputChannels, engineRoutes[i].first, engineRoutes[i].second);
        engineContent.addAndMakeVisible(*card);
        engineCards.push_back(std::move(card));
    }
    resized();
}

void VirtualDSPSidebar::setInputChannels(const StringArray& channels) {
    inputChannels = channels;
    for (size_t i = 0; i < engineCards.size(); ++i)
        engineCards[i]->setChannels(inputChannels, engineRoutes[i].first, engineRoutes[i].second);
}

void VirtualDSPSidebar::setRoute(int engine, int mic, int ref) {
    if (engine < 0 || (size_t)engine >= engineRoutes.size()) return;
    engineRoutes[(size_t)engine] = {mic, ref};
    engineCards[(size_t)engine]->setChannels(inputChannels, mic, ref);
}

std::pair<int,int> VirtualDSPSidebar::route(int engine) const {
    return engine >= 0 && (size_t)engine < engineRoutes.size() ? engineRoutes[(size_t)engine] : std::pair<int,int>{0, 0};
}

void VirtualDSPSidebar::resized() {
    auto bounds = getLocalBounds().reduced(5);
    const int globalHeight = 110;
    const int engineTop = 239;
    engineViewport.setBounds(bounds.getX(), engineTop, bounds.getWidth(), std::max(20, getHeight() - engineTop - globalHeight - 8));
    engineContent.setSize(std::max(100, engineViewport.getWidth() - 8), (int)engineCards.size() * cardHeight);
    for (size_t i = 0; i < engineCards.size(); ++i)
        engineCards[i]->setBounds(0, (int)i * cardHeight, engineContent.getWidth(), cardHeight);
    const int x = bounds.getX(), width = bounds.getWidth(), top = getHeight() - globalHeight;
    const int third = (width - 8) / 3;
    for (int i = 0; i < 6; ++i)
        globalButtons[(size_t)i].setBounds(x + (i % 3) * (third + 4), top + 18 + (i / 3) * 29, third, 26);
    globalButtons[6].setBounds(x, top + 76, (width - 4) / 2, 27);
    globalButtons[7].setBounds(x + (width + 4) / 2, top + 76, (width - 4) / 2, 27);
}

void VirtualDSPSidebar::paint(Graphics& g) {
    g.fillAll(Colour(0xff222a30));
    g.setColour(Colour(0xff56636d)); g.drawRect(getLocalBounds(), 1);
    auto box = [&](Rectangle<int> r, const String& text, bool cyan = false) {
        g.setColour(Colour(0xff141c22)); g.fillRect(r);
        g.setColour(Colour(0xff52616a)); g.drawRect(r, 1);
        g.setColour(cyan ? Colour(0xff00e5ed) : Colours::white); g.setFont(11.f);
        g.drawText(text, r, Justification::centred);
    };
    const int x = 5, w = getWidth() - 10;
    g.setColour(Colours::white); g.setFont(11.f); g.drawText("Generador", x + 3, 2, w, 19, Justification::centredLeft);
    box({x, 22, (w - 4) / 2, 30}, "Gen"); box({x + (w + 4) / 2, 22, (w - 4) / 2, 30}, "-24");
    box({x, 56, (w - 4) / 2, 29}, "Pink"); box({x + (w + 4) / 2, 56, (w - 4) / 2, 29}, "Sweep");
    g.setColour(Colours::white); g.drawText("OUT 1", x, 91, 44, 23, Justification::centredLeft);
    box({x + 44, 91, (w - 96) / 2, 23}, "1");
    g.drawText("OUT 2", x + 50 + (w - 96) / 2, 91, 44, 23, Justification::centredLeft);
    box({x + 96 + (w - 96) / 2, 91, (w - 96) / 2, 23}, "2");
    g.setColour(Colour(0xff14212a)); g.fillRect(x + 3, 125, w - 52, 5);
    g.setColour(Colour(0xff00c9eb)); g.fillRect(x + 3, 125, (w - 52) / 2, 5);
    box({x + w - 42, 116, 42, 23}, "0");
    g.setColour(Colour(0xff536775)); g.drawHorizontalLine(152, (float)x, (float)(x + w));
    g.setColour(Colours::white); g.drawText("FFT", x + 2, 158, 42, 26, Justification::centredLeft);
    box({x + 47, 158, w - 47, 26}, "64 FPPO");
    g.drawText("Smth", x + 2, 190, 42, 26, Justification::centredLeft);
    box({x + 47, 190, w - 47, 26}, "1/48 Oct");
    g.setColour(Colour(0xff536775)); g.drawHorizontalLine(230, (float)x, (float)(x + w));
    const int footer = getHeight() - 110;
    g.drawHorizontalLine(footer, (float)x, (float)(x + w));
    g.setColour(Colours::white); g.drawText("GLOBAL", x + 3, footer + 1, 75, 17, Justification::centredLeft);
    g.drawText("RTA", x + 82, footer + 1, 45, 17, Justification::centredLeft);
}
