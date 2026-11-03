#include "OfflineEQWindow.h"
#include <memory>

using namespace juce;

struct OfflineEQWindow::Editor : Component {
    int engine;
    OfflineDSPCore& core;
    std::function<void(int)> changed;
    std::function<void(bool)> graphicChanged;
    Viewport viewport;
    Component content;
    struct BandRow : Component {
        ToggleButton enabled;
        ComboBox type;
        Slider frequency,gain,q;
        int number;
        std::function<void()> changed;
        BandRow(int index):number(index){
            enabled.setButtonText(String(index+1));addAndMakeVisible(enabled);
            type.addItem("Bell",1);type.addItem("Lo Shelf",2);type.addItem("Hi Shelf",3);addAndMakeVisible(type);
            auto setup=[this](Slider& s,double low,double high,double step){s.setSliderStyle(Slider::IncDecButtons);s.setTextBoxStyle(Slider::TextBoxLeft,false,66,22);s.setRange(low,high,step);addAndMakeVisible(s);s.onValueChange=[this]{if(changed)changed();};};
            setup(frequency,10,20000,1);setup(gain,-15,15,0.1);setup(q,0.1,128,0.01);
            enabled.onClick=[this]{if(changed)changed();};type.onChange=[this]{if(changed)changed();};
        }
        void resized() override {auto r=getLocalBounds().reduced(2);enabled.setBounds(r.removeFromLeft(48));type.setBounds(r.removeFromLeft(105));frequency.setBounds(r.removeFromLeft(110));gain.setBounds(r.removeFromLeft(100));q.setBounds(r);}
        void set(const OfflineDSPCore::EQBand& b){enabled.setToggleState(b.enabled,dontSendNotification);type.setSelectedId((int)b.type+1,dontSendNotification);frequency.setValue(b.frequency,dontSendNotification);gain.setValue(b.gainDb,dontSendNotification);q.setValue(b.q,dontSendNotification);}
        OfflineDSPCore::EQBand value()const{return {enabled.getToggleState(),(OfflineDSPFilters::EQType)(type.getSelectedId()-1),frequency.getValue(),gain.getValue(),q.getValue()};}
    };
    std::array<std::unique_ptr<BandRow>,20> bands;
    std::array<ToggleButton,2> apEnable;
    std::array<Slider,2> apFrequency,apQ;
    ToggleButton chingEnable{"Chingonizer"},graphic{"Graphic EQ: On"};
    std::array<Slider,4> ching;
    Editor(int e,OfflineDSPCore& c,std::function<void(int)> cb,std::function<void(bool)> graphicCb)
        :engine(e),core(c),changed(std::move(cb)),graphicChanged(std::move(graphicCb)){
        viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);
        for(int i=0;i<20;++i){bands[(size_t)i]=std::make_unique<BandRow>(i);auto* row=bands[(size_t)i].get();row->changed=[this,i]{if(core.setBand(engine,i,bands[(size_t)i]->value())&&changed)changed(engine);};content.addAndMakeVisible(*row);}
        auto setup=[this](Slider& s,double low,double high,double step){s.setSliderStyle(Slider::IncDecButtons);s.setTextBoxStyle(Slider::TextBoxLeft,false,65,22);s.setRange(low,high,step);addAndMakeVisible(s);};
        for(int i=0;i<2;++i){apEnable[(size_t)i].setButtonText("APF "+String(i+1));addAndMakeVisible(apEnable[(size_t)i]);setup(apFrequency[(size_t)i],10,20000,1);setup(apQ[(size_t)i],0.1,128,0.01);auto update=[this,i]{OfflineDSPCore::AllPass a{apEnable[(size_t)i].getToggleState(),i+1,apFrequency[(size_t)i].getValue(),apQ[(size_t)i].getValue()};if(core.setAllPass(engine,i,a)&&changed)changed(engine);};apEnable[(size_t)i].onClick=update;apFrequency[(size_t)i].onValueChange=update;apQ[(size_t)i].onValueChange=update;}
        addAndMakeVisible(chingEnable);for(auto& s:ching)setup(s,0,20000,0.1);
        ching[0].setRange(10,1000,1);ching[1].setRange(1000,20000,1);ching[2].setRange(-15,15,0.1);ching[3].setRange(-15,15,0.1);
        auto updateChing=[this]{OfflineDSPCore::Chingonizer value{chingEnable.getToggleState(),ching[0].getValue(),ching[1].getValue(),ching[2].getValue(),ching[3].getValue()};if(core.setChingonizer(engine,value)&&changed)changed(engine);};
        chingEnable.onClick=updateChing;for(auto& s:ching)s.onValueChange=updateChing;
        addAndMakeVisible(graphic);graphic.onClick=[this]{if(graphicChanged)graphicChanged(graphic.getToggleState());};
        refresh();
    }
    void refresh(){for(int i=0;i<20;++i)bands[(size_t)i]->set(core.band(engine,i));for(int i=0;i<2;++i){auto a=core.allPass(engine,i);apEnable[(size_t)i].setToggleState(a.enabled,dontSendNotification);apFrequency[(size_t)i].setValue(a.frequency,dontSendNotification);apQ[(size_t)i].setValue(a.q,dontSendNotification);}auto c=core.chingonizer(engine);chingEnable.setToggleState(c.enabled,dontSendNotification);ching[0].setValue(c.lowFrequency,dontSendNotification);ching[1].setValue(c.highFrequency,dontSendNotification);ching[2].setValue(c.lowGainDb,dontSendNotification);ching[3].setValue(c.highGainDb,dontSendNotification);}
    void resized()override{auto area=getLocalBounds().reduced(8);auto footer=area.removeFromBottom(110);viewport.setBounds(area);content.setSize(std::max(530,viewport.getWidth()-8),20*29);for(int i=0;i<20;++i)bands[(size_t)i]->setBounds(0,i*29,content.getWidth(),28);for(int i=0;i<2;++i){auto line=footer.removeFromTop(26);apEnable[(size_t)i].setBounds(line.removeFromLeft(80));apFrequency[(size_t)i].setBounds(line.removeFromLeft(120));apQ[(size_t)i].setBounds(line.removeFromLeft(120));}auto line=footer.removeFromTop(27);chingEnable.setBounds(line.removeFromLeft(115));for(auto& s:ching)s.setBounds(line.removeFromLeft(100));graphic.setBounds(footer.removeFromTop(25));}
    void paint(Graphics& g)override{g.fillAll(Colour(0xff10171c));}
};

OfflineEQWindow::OfflineEQWindow(int engine,OfflineDSPCore& core,std::function<void(int)> changed,std::function<void(bool)> graphicChanged)
    :DocumentWindow("Equalizer - Engine "+String(engine+1),Colour(0xff10171c),DocumentWindow::closeButton){
    editor=new Editor(engine,core,std::move(changed),std::move(graphicChanged));
    setUsingNativeTitleBar(true);setContentOwned(editor,true);setResizable(true,false);setSize(580,700);centreWithSize(580,700);setVisible(false);
}
void OfflineEQWindow::refresh(){if(editor)editor->refresh();}
