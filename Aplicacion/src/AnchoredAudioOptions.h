#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <cmath>
#include <optional>

// These panels inspect presentation/state only. They own no audio device, callback or route setter.
class AnchoredOptions : public juce::Component {
 std::unique_ptr<juce::Component> body;juce::Label title;juce::TextButton close{"X"};
public:
 std::function<void()> requestClose;
 juce::Component* content()const{return body.get();}
 AnchoredOptions(juce::String heading,std::unique_ptr<juce::Component> content):body(std::move(content)){
  title.setText(heading,juce::dontSendNotification);title.setColour(juce::Label::textColourId,juce::Colour(0xff00e5ed));addAndMakeVisible(title);addAndMakeVisible(close);addAndMakeVisible(*body);close.onClick=[this]{if(requestClose)requestClose();};setSize(body->getWidth()+20,body->getHeight()+42);
 }
 void resized()override{title.setBounds(10,4,getWidth()-48,28);close.setBounds(getWidth()-34,5,26,24);body->setBounds(10,36,getWidth()-20,getHeight()-42);}
 void paint(juce::Graphics& g)override{g.setColour(juce::Colour(0xff172129));g.fillRoundedRectangle(getLocalBounds().toFloat(),5);g.setColour(juce::Colour(0xff748a98));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),5,1);}
};

class AudioOptionsView : public juce::Component {
 std::vector<std::unique_ptr<juce::Component>> controls;int nextY=0;
 juce::String outputName,referenceName;
 juce::ToggleButton* loopToggle=nullptr;juce::Label* routeDevice=nullptr;juce::Label* routeOutput=nullptr;juce::Label* routeReference=nullptr;juce::Label* outputLevel=nullptr;juce::Label* referenceLevel=nullptr;juce::Label* returnStatus=nullptr;juce::Label* loopDelay=nullptr;juce::Label* calibrationStatus=nullptr;juce::TextButton* calibrate=nullptr;juce::TextButton* reset=nullptr;juce::Slider* levelSlider=nullptr;juce::Slider* refSlider=nullptr;
 juce::Label& label(juce::String value,int x,int y,int width,int height=24){auto c=std::make_unique<juce::Label>();c->setText(value,juce::dontSendNotification);c->setFont(juce::Font(juce::FontOptions(12.f)));c->setBounds(x,y,width,height);auto* result=c.get();addAndMakeVisible(*c);controls.push_back(std::move(c));return *result;}
 juce::Label* textRow(juce::String name,juce::String value){label(name,0,nextY,114);auto& field=label(value,116,nextY,214);field.setColour(juce::Label::backgroundColourId,juce::Colour(0xff0e171e));field.setTooltip(value);nextY+=28;return &field;}
 juce::Slider* sliderRow(juce::String title,double low,double high,double value,juce::String suffix,std::function<void(double)> changed){label(title,0,nextY,92);auto s=std::make_unique<juce::Slider>();auto* raw=s.get();s->setSliderStyle(juce::Slider::LinearHorizontal);s->setTextBoxStyle(juce::Slider::TextBoxRight,false,76,22);s->setRange(low,high,.5);s->setValue(value,juce::dontSendNotification);s->setTextValueSuffix(suffix);s->setBounds(94,nextY,236,24);s->onValueChange=[raw,changed]{if(changed)changed(raw->getValue());};addAndMakeVisible(*s);controls.push_back(std::move(s));nextY+=30;return raw;}
 juce::TextButton* actionButton(juce::String caption,int x,int y,int w,std::function<void()> action){auto b=std::make_unique<juce::TextButton>(caption);auto* raw=b.get();b->setBounds(x,y,w,24);b->onClick=std::move(action);addAndMakeVisible(*b);controls.push_back(std::move(b));return raw;}
public:
 std::function<void(bool)> onPhysicalLoopToggle;
 std::function<void(double)> onPinkLevel,onReferenceGain;
 std::function<void()> onCalibrate,onReset;
 AudioOptionsView(bool loop,juce::String deviceAndBackend,juce::String projectOutput,juce::String referenceInput,bool loopActive=false,double pinkDb=-40.,double refDb=0.):outputName(std::move(projectOutput)),referenceName(std::move(referenceInput)){
  if(loop){auto enabled=std::make_unique<juce::ToggleButton>(juce::String::fromUTF8("Loop físico / salida → referencia"));loopToggle=enabled.get();enabled->setToggleState(loopActive,juce::dontSendNotification);enabled->onClick=[this]{if(onPhysicalLoopToggle)onPhysicalLoopToggle(loopToggle->getToggleState());};enabled->setBounds(0,nextY,330,25);addAndMakeVisible(*enabled);controls.push_back(std::move(enabled));nextY+=30;levelSlider=sliderRow("Nivel Pink",-60,-20,pinkDb," dBFS",[this](double v){if(onPinkLevel)onPinkLevel(v);});refSlider=sliderRow("REF Gain",-24,24,refDb," dB",[this](double v){if(onReferenceGain)onReferenceGain(v);});routeDevice=textRow("Tarjeta/backend",deviceAndBackend);routeOutput=textRow("Salida",outputName.isNotEmpty()?outputName:"Sin asignar");routeReference=textRow(juce::String::fromUTF8("Referencia física"),referenceName.isNotEmpty()?referenceName:"Sin asignar");outputLevel=&label("Salida: -- dBFS",0,nextY,330);nextY+=26;referenceLevel=&label("Referencia: -- dBFS",0,nextY,330);nextY+=26;returnStatus=&label("Loop detenido",0,nextY,330,36);nextY+=38;loopDelay=&label("Loop Delay: -- ms",0,nextY,330);nextY+=27;calibrate=actionButton("Calib Loop",0,nextY,160,[this]{if(onCalibrate)onCalibrate();});reset=actionButton("Reset Loop",170,nextY,160,[this]{if(onReset)onReset();});nextY+=29;calibrationStatus=&label(juce::String::fromUTF8("Sin calibración"),0,nextY,330,42);nextY+=44;label(juce::String::fromUTF8("El delay de loop es electrónico; no es distancia acústica."),0,nextY,330,32);nextY+=34;}
  else{levelSlider=sliderRow("Nivel Pink",-60,-20,pinkDb," dBFS",[this](double v){if(onPinkLevel)onPinkLevel(v);});textRow("Salida",outputName.isNotEmpty()?outputName:"Sin asignar");textRow("HPF","-- Hz / pendiente");textRow("LPF","-- Hz / pendiente");label(juce::String::fromUTF8("El nivel controla la salida configurada. HPF/LPF aún no están disponibles."),0,nextY,330,42);nextY+=44;}
  setSize(330,nextY);
 }
 void setRoute(juce::String device,juce::String output,juce::String reference){if(!loopToggle)return;outputName=std::move(output);referenceName=std::move(reference);for(auto pair:{std::pair<juce::Label*,juce::String>{routeDevice,device},{routeOutput,outputName},{routeReference,referenceName}})if(pair.first){pair.first->setText(pair.second,juce::dontSendNotification);pair.first->setTooltip(pair.second);}}
 void updatePhysicalLoop(bool active,bool routeReady,std::optional<double> outputDbfs,std::optional<double> referenceDbfs,bool referenceClipped,std::optional<double> delayMs,bool delayCurrent,juce::String calibrationText,bool measuring,bool saved,double pinkDb,double refDb){
  if(!loopToggle)return;
  loopToggle->setToggleState(active,juce::dontSendNotification);
  loopToggle->setEnabled(routeReady||active);
  levelSlider->setValue(pinkDb,juce::dontSendNotification);
  refSlider->setValue(refDb,juce::dontSendNotification);
  levelSlider->setEnabled(!measuring);
  refSlider->setEnabled(!measuring);
  auto out=outputName.isNotEmpty()&&outputName!="Sin asignar"?outputName:juce::String("Salida");auto ref=referenceName.isNotEmpty()&&referenceName!="Sin asignar"?referenceName:juce::String("Referencia");
  outputLevel->setText(out+": "+(active&&outputDbfs&&std::isfinite(*outputDbfs)?juce::String(*outputDbfs,1):juce::String("--"))+" dBFS",juce::dontSendNotification);
  referenceLevel->setText(ref+": "+(active&&referenceDbfs&&std::isfinite(*referenceDbfs)?juce::String(*referenceDbfs,1):juce::String("--"))+" dBFS",juce::dontSendNotification);
  returnStatus->setText(!active?"Loop detenido":referenceClipped?"Referencia recortada; reduce Pink":referenceDbfs&&std::isfinite(*referenceDbfs)&&*referenceDbfs>-75.?"Retorno detectado / comprobar nivel":"Sin retorno detectable en "+ref,juce::dontSendNotification);
  auto delayText=delayMs?"Loop Delay: "+juce::String(*delayMs,2)+" ms":"Loop Delay: -- ms";
  if(delayMs&&!delayCurrent)delayText+=juce::String::fromUTF8(" · guardado / pendiente de validar");
  loopDelay->setText(delayText,juce::dontSendNotification);
  loopDelay->setTooltip(delayText);
  calibrationStatus->setText(calibrationText,juce::dontSendNotification);
  calibrate->setEnabled(active&&routeReady&&!measuring);
  reset->setEnabled(routeReady&&(saved||measuring||std::abs(refDb)>.01));
 }
};
