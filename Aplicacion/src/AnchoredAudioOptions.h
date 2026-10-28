#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

// These panels inspect presentation/state only. They own no audio device, callback or route setter.
class AnchoredOptions : public juce::Component {
 std::unique_ptr<juce::Component> body;juce::Label title;juce::TextButton close{"X"};
public:
 std::function<void()> requestClose;
 AnchoredOptions(juce::String heading,std::unique_ptr<juce::Component> content):body(std::move(content)){
  title.setText(heading,juce::dontSendNotification);title.setColour(juce::Label::textColourId,juce::Colour(0xff00e5ed));addAndMakeVisible(title);addAndMakeVisible(close);addAndMakeVisible(*body);close.onClick=[this]{if(requestClose)requestClose();};setSize(body->getWidth()+20,body->getHeight()+42);
 }
 void resized()override{title.setBounds(10,4,getWidth()-48,28);close.setBounds(getWidth()-34,5,26,24);body->setBounds(10,36,getWidth()-20,getHeight()-42);}
 void paint(juce::Graphics& g)override{g.setColour(juce::Colour(0xff172129));g.fillRoundedRectangle(getLocalBounds().toFloat(),5);g.setColour(juce::Colour(0xff748a98));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),5,1);}
};

class AudioOptionsView : public juce::Component {
 class UnavailableGainSlider : public juce::Slider {void paint(juce::Graphics& g)override{g.setColour(juce::Colour(0xff455863));g.fillRoundedRectangle(3.f,(float)getHeight()*.5f-2.f,(float)getWidth()-6.f,4.f,2.f);}};
 std::vector<std::unique_ptr<juce::Component>> controls;int nextY=0;
 juce::Label& label(juce::String value,int x,int y,int width,int height=24){auto c=std::make_unique<juce::Label>();c->setText(value,juce::dontSendNotification);c->setFont(juce::Font(juce::FontOptions(12.f)));c->setBounds(x,y,width,height);auto* result=c.get();addAndMakeVisible(*c);controls.push_back(std::move(c));return *result;}
 void textRow(juce::String name,juce::String value){label(name,0,nextY,114);auto& field=label(value,116,nextY,214);field.setColour(juce::Label::backgroundColourId,juce::Colour(0xff0e171e));field.setTooltip(value);nextY+=28;}
 void gainRow(juce::String unit){label(unit=="REF Gain"?unit:juce::String("Ganancia"),0,nextY,94);auto slider=std::make_unique<UnavailableGainSlider>();slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);slider->setEnabled(false);slider->setBounds(96,nextY,160,24);slider->setTooltip("Sin estado de motor ni rango fisico validado");addAndMakeVisible(*slider);controls.push_back(std::move(slider));label(unit=="REF Gain"?"-- dB":"-- dBFS",258,nextY,72);nextY+=28;}
 void pendingButton(juce::String caption,int x,int y,int w){auto b=std::make_unique<juce::TextButton>(caption);b->setBounds(x,y,w,24);b->setEnabled(false);b->setTooltip("Motor de audio/calibracion pendiente");addAndMakeVisible(*b);controls.push_back(std::move(b));}
public:
 AudioOptionsView(bool loop,juce::String deviceAndBackend,juce::String projectOutput){
  if(loop){auto enabled=std::make_unique<juce::ToggleButton>("Internal Loop Enabled: OFF");enabled->setToggleState(false,juce::dontSendNotification);enabled->setEnabled(false);enabled->setBounds(0,nextY,330,25);addAndMakeVisible(*enabled);controls.push_back(std::move(enabled));nextY+=30;gainRow("REF Gain");textRow("Tarjeta/backend",deviceAndBackend);textRow("OutputDevice",projectOutput);textRow("OutMixer / canal","Sin asignar al loop");textRow("OutRef / canal","Sin asignar al loop");textRow("Loop Delay","-- ms");pendingButton("Calib Loop",0,nextY,160);pendingButton("Reset Loop",170,nextY,160);nextY+=30;label(juce::String::fromUTF8("Sin motor de loop. La salida indicada es la del proyecto; no confirma un loop conectado. Parámetros/calibración pendientes."),0,nextY,330,54);nextY+=58;}
  else{gainRow("dBFS");textRow("HPF","-- Hz / pendiente");textRow("LPF","-- Hz / pendiente");label(juce::String::fromUTF8("Sin generación. Ganancia/HPF/LPF pendientes de motor. Tipos y rangos sin verificar; abrir no cambia parámetros."),0,nextY,330,58);nextY+=62;}
  setSize(330,nextY);
 }
};
