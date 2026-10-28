#pragma once
#include "TFSettingsPanel.h"
#include "GraphOptionInventory.h"
#include "Commands.h"
class PendingGraphPage : public juce::Component {
 juce::Viewport viewport;juce::Component content;std::vector<std::unique_ptr<juce::Label>> labels;juce::ComboBox irScale;bool impulse;std::function<int()> irMode;std::function<void(int)> action;std::function<TFDisplayState()> readTF;std::function<void(TFDisplayState)> applyTF;juce::Slider foreground,background;juce::Label foregroundLabel,backgroundLabel;
public:
 PendingGraphPage(const std::string& tab,bool english,std::function<int()> getIR,std::function<void(int)> dispatch,std::function<TFDisplayState()> getter={},std::function<void(TFDisplayState)> setter={}):impulse(tab=="IR"),irMode(std::move(getIR)),action(std::move(dispatch)),readTF(std::move(getter)),applyTF(std::move(setter)){
  viewport.setViewedComponent(&content,false);addAndMakeVisible(viewport);
  juce::String previous;
  for(const auto& option:graphOptions())if(option.tab==tab&&!option.implemented){juce::String group;
   if(!impulse){if(option.key=="spectrum.banding"||option.key=="spectrum.frequency"||option.key=="spectrum.zoom"||option.key=="spectrum.weight"||option.key=="spectrum.weightDisplay")group="General";else if(option.key=="spectrum.sliceHeight"||option.key.find("dynamic")!=std::string::npos||option.key=="spectrum.history"||option.key=="spectrum.gray"||option.key=="spectrum.memory")group=english?"Spectrogram":"Espectrograma";else group="RTA";}
   else if(option.key.find("hpf")!=std::string::npos||option.key.find("lpf")!=std::string::npos)group=english?"Broadband filters":"Filtros de banda ancha";
   else if(option.key=="ir.min"||option.key=="ir.max"||option.key=="ir.trace"||option.key=="ir.frequency")group=english?"Frequency plot":juce::String::fromUTF8("Gráfico de frecuencia");
   else if(option.key=="ir.gray"||option.key=="ir.window"||option.key.find("dynamic")!=std::string::npos)group=english?"Spectrogram":"Espectrograma";
   else if(option.key=="ir.plot")group=english?"Histogram":"Histograma";else group="General";
   if(group!=previous){auto header=std::make_unique<juce::Label>();header->setText(group,juce::dontSendNotification);header->setFont(juce::Font(juce::FontOptions(13.f)));content.addAndMakeVisible(*header);labels.push_back(std::move(header));previous=group;}
   auto label=std::make_unique<juce::Label>();label->setText(juce::String::fromUTF8((english?option.en:option.es).c_str())+juce::String::fromUTF8(english?" — pending engine / representation":" — pendiente: motor / representación"),juce::dontSendNotification);label->setEnabled(false);label->setFont(juce::Font(juce::FontOptions(12.f)));content.addAndMakeVisible(*label);labels.push_back(std::move(label));}
  if(impulse){irScale.addItemList({"Lin (demo)","Log (demo)","ETC (demo)"},1);irScale.setSelectedId(irMode()+1,juce::dontSendNotification);irScale.onChange=[this]{action(929+irScale.getSelectedId());};content.addAndMakeVisible(irScale);foregroundLabel.setText(english?"Foreground width (px)":"Grosor frontal (px)",juce::dontSendNotification);backgroundLabel.setText(english?"Background width (px)":"Grosor fondo (px)",juce::dontSendNotification);for(auto* slider:{&foreground,&background}){slider->setRange(.5,8,.25);slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxLeft,false,55,24);content.addAndMakeVisible(*slider);}content.addAndMakeVisible(foregroundLabel);content.addAndMakeVisible(backgroundLabel);foreground.onValueChange=[this]{if(readTF&&applyTF){auto s=readTF();s.foregroundWidth=foreground.getValue();applyTF(s);}};background.onValueChange=[this]{if(readTF&&applyTF){auto s=readTF();s.backgroundWidth=background.getValue();applyTF(s);}};}
 }
 ~PendingGraphPage() override{viewport.setViewedComponent(nullptr,false);}
 void paint(juce::Graphics&) override{if(impulse&&readTF){irScale.setSelectedId(irMode()+1,juce::dontSendNotification);foreground.setValue(readTF().foregroundWidth,juce::dontSendNotification);background.setValue(readTF().backgroundWidth,juce::dontSendNotification);}}
 void resized() override{viewport.setBounds(getLocalBounds());int y=8;if(impulse){irScale.setBounds(8,y,180,26);y+=34;foregroundLabel.setBounds(8,y,175,26);foreground.setBounds(185,y,juce::jmax(100,getWidth()-215),26);y+=32;backgroundLabel.setBounds(8,y,175,26);background.setBounds(185,y,juce::jmax(100,getWidth()-215),26);y+=34;}for(auto& label:labels){label->setBounds(8,y,juce::jmax(100,getWidth()-30),28);y+=32;}content.setSize(juce::jmax(100,getWidth()-16),y+8);}
};
class GraphSettingsWindow : public juce::DocumentWindow {
 class Body : public juce::Component {
  juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};juce::TextButton measurement,ok;
 public:
  Body(bool english,std::function<TFDisplayState()> read,std::function<void(TFDisplayState)> apply,std::function<void(int)> action,std::function<int()> irMode,std::function<void()> close){
   addAndMakeVisible(tabs);tabs.addTab(english?"Spectrum":"Espectro",juce::Colour(0xff222427),new PendingGraphPage("Spectrum",english,irMode,action),true);tabs.addTab(english?"Transfer Function":juce::String::fromUTF8("Función de transferencia"),juce::Colour(0xff222427),new TFSettingsPanel(read,apply,action,english),true);tabs.addTab(english?"Impulse Response":"Respuesta al impulso",juce::Colour(0xff222427),new PendingGraphPage("IR",english,irMode,action,read,apply),true);
   measurement.setButtonText(english?"Measurement Settings":juce::String::fromUTF8("Ajustes de medición"));measurement.setTooltip(english?"Mya device and input routing; acquisition engines pending":juce::String::fromUTF8("Dispositivos y rutas de Mya; motores de adquisición pendientes"));measurement.onClick=[action]{action(201);};ok.setButtonText("OK");ok.onClick=std::move(close);addAndMakeVisible(measurement);addAndMakeVisible(ok);setSize(700,520);
  }
  void select(int tab){tabs.setCurrentTabIndex(tab);}
  int selected() const{return tabs.getCurrentTabIndex();}
  void resized() override{tabs.setBounds(0,0,getWidth(),getHeight()-42);measurement.setBounds(10,getHeight()-35,210,28);ok.setBounds(getWidth()-85,getHeight()-35,75,28);}
 };
 Body* body;MyaCommands::MeterSequence sequence;
public:
 std::function<void()> toggleMeters;
 GraphSettingsWindow(bool english,std::function<TFDisplayState()> read,std::function<void(TFDisplayState)> apply,std::function<void(int)> action,std::function<int()> irMode):DocumentWindow(english?"Mya - Graph Settings":juce::String::fromUTF8("Mya - Ajustes de gráficos"),juce::Colour(0xff222427),closeButton){setUsingNativeTitleBar(true);body=new Body(english,std::move(read),std::move(apply),std::move(action),std::move(irMode),[this]{setVisible(false);});setContentOwned(body,true);setResizable(true,false);setResizeLimits(600,460,1600,1400);centreWithSize(740,560);}
 bool keyPressed(const juce::KeyPress& key) override{for(auto* focus=juce::Component::getCurrentlyFocusedComponent();focus;focus=focus->getParentComponent())if(dynamic_cast<juce::TextEditor*>(focus)||dynamic_cast<juce::ComboBox*>(focus)||dynamic_cast<juce::Slider*>(focus)){sequence.reset();return false;}auto mods=key.getModifiers();if(MyaCommands::isPlainMeterKey(key.getKeyCode(),mods.isCommandDown(),mods.isCtrlDown(),mods.isAltDown(),mods.isShiftDown())){if(sequence.press(juce::Time::getMillisecondCounterHiRes(),true)&&toggleMeters)toggleMeters();return true;}sequence.reset();return false;}
 int selectedTab() const{return body->selected();}
 void select(int tab){body->select(tab);}
 void closeButtonPressed() override{setVisible(false);}
};
