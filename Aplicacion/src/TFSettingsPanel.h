#pragma once
#include "TFDisplayState.h"
#include <juce_gui_extra/juce_gui_extra.h>
class TFSettingsPanel : public juce::Component {
 bool english=false;
 static juce::String translated(const juce::String& name,bool en){if(!en)return name;
  if(name=="Frecuencia / reticula")return "Frequency scale / grid";
  if(name=="Mostrar IR (demo)")return "Display IR (demo)";
  if(name=="Grosor frontal (px)")return "Foreground width (px)";
  if(name=="Grosor fondo (px)")return "Background width (px)";
  if(name=="Vincular fase y magnitud")return "Link phase and magnitude smoothing";
  if(name=="Vincular zoom de frecuencia X")return "Link frequency X zoom";
  if(name=="Permitir sin suavizado")return "Allow no smoothing";
  if(name=="Ponderacion visual / selector")return "Visual weighting / selector";
  if(name=="IR proporcional")return "Proportional IR";
  if(name=="Respuesta instantanea")return "Instantaneous response";
  if(name=="Minimo (dB)")return "Minimum (dB)";
  if(name=="Maximo (dB)")return "Maximum (dB)";
  if(name=="Reticula (dB)")return "Y grid (dB)";
  if(name=="Paso desplazar (dB)")return "Y scroll step (dB)";
  if(name=="Paso zoom (dB)")return "Y zoom step (dB)";
  if(name=="Ponderacion visual")return "Visual weighting";
  if(name=="Minimo (grados)")return "Minimum (degrees)";
  if(name=="Maximo (grados)")return "Maximum (degrees)";
  if(name=="Reticula (grados)")return "Y grid (degrees)";
  if(name=="Paso desplazar (grados)")return "Y scroll step (degrees)";
  if(name=="Paso zoom (grados)")return "Y zoom step (degrees)";
  if(name=="Frecuencia central unwrap")return "Unwrap centre frequency";
  if(name=="Rango unwrap")return "Unwrap range";
  if(name=="Mostrar coherencia roja (C)")return "Show red coherence (C)";
  if(name=="Coherencia cuadrada (gamma^2)")return "Squared coherence (gamma^2)";
  if(name=="Umbral (%) mag + fase")return "Threshold (%) magnitude + phase";
  if(name=="Altura de coherencia")return "Coherence height";
  if(name=="Grosor propio (px)")return "Coherence width (px)";
  if(name=="Seguir grosor frontal")return "Follow foreground width";
  if(name=="Dato base: gamma^2 (demo)")return "Stored base: gamma^2 (demo)";
  if(name=="Ocultar no desactiva umbral")return "Hiding does not disable the threshold";
  return name;
 }
 struct Page : juce::Component {
  bool english=false;juce::Viewport viewport;juce::Component content;Page(){viewport.setViewedComponent(&content,false);addAndMakeVisible(viewport);}~Page() override{viewport.setViewedComponent(nullptr,false);}
  struct Row {std::unique_ptr<juce::Label> label;std::unique_ptr<juce::Component> control;};std::vector<Row> rows;
  template<class T>T* add(juce::String title,std::unique_ptr<T> control){auto label=std::make_unique<juce::Label>();label->setText(TFSettingsPanel::translated(title,english),juce::dontSendNotification);label->setFont(juce::Font(juce::FontOptions(12.f)));content.addAndMakeVisible(*label);content.addAndMakeVisible(*control);auto* ptr=control.get();rows.push_back({std::move(label),std::move(control)});return ptr;}
  void resized() override{viewport.setBounds(getLocalBounds());content.setSize(juce::jmax(100,getWidth()-16),(int)rows.size()*32+16);int y=8,split=juce::jlimit(150,250,content.getWidth()/2);for(auto& row:rows){row.label->setBounds(8,y,split-12,27);row.control->setBounds(split,y,juce::jmax(80,content.getWidth()-split-8),27);y+=32;}}
 };
 juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};std::function<TFDisplayState()> read;std::function<void(TFDisplayState)> apply;std::function<void(int)> dispatch;
 std::vector<std::function<void()>> sync;
 void number(Page& page,juce::String name,double TFDisplayState::*field){
  auto* editor=page.add(name,std::make_unique<juce::TextEditor>());editor->setText(juce::String(read().*field,2),false);editor->setInputRestrictions(12,"-0123456789.,");
  auto update=[this,editor,field]{auto parsed=juce::JSON::parse(editor->getText().replaceCharacter(',','.'));if(parsed.isDouble()||parsed.isInt()||parsed.isInt64()){auto s=read();s.*field=double(parsed);apply(s);if(std::abs(read().*field-double(parsed))>1.e-9)editor->setText(juce::String(read().*field,2),false);}else editor->setText(juce::String(read().*field,2),false);};editor->onReturnKey=update;editor->onFocusLost=update;
  sync.push_back([this,editor,field]{if(field==&TFDisplayState::coherenceWidth)editor->setEnabled(read().ownCoherenceWidthEditable());if(!editor->hasKeyboardFocus(true))editor->setText(juce::String(read().*field,2),false);});
 }
 void flag(Page& page,juce::String name,bool TFDisplayState::*field,int command=0){
  auto* button=page.add(name,std::make_unique<juce::ToggleButton>());button->setClickingTogglesState(false);button->onClick=[this,field,command]{if(command)dispatch(command);else{auto s=read();s.*field=!(s.*field);apply(s);}repaint();};sync.push_back([this,button,field]{button->setToggleState(read().*field,juce::dontSendNotification);});
 }
 void pending(Page& page,juce::String name,juce::String why){auto* label=page.add(name,std::make_unique<juce::Label>());label->setText(english?juce::String("Pending: engine / representation / validation"):why,juce::dontSendNotification);label->setFont(juce::Font(juce::FontOptions(11.f)));label->setEnabled(false);label->setTooltip(why);}
 void information(Page& page,juce::String name,juce::String es,juce::String en){auto* label=page.add(name,std::make_unique<juce::Label>());label->setText(english?en:es,juce::dontSendNotification);label->setFont(juce::Font(juce::FontOptions(10.f)));label->setTooltip(english?en:es);}
public:
 TFSettingsPanel(std::function<TFDisplayState()> getter,std::function<void(TFDisplayState)> setter,std::function<void(int)> action,bool en=false):english(en),read(std::move(getter)),apply(std::move(setter)),dispatch(std::move(action)){
  addAndMakeVisible(tabs);auto* general=new Page;auto* magnitude=new Page;auto* phase=new Page;auto* coherence=new Page;for(auto* page:{general,magnitude,phase,coherence})page->english=english;
  tabs.addTab("General",juce::Colour(0xff222427),general,true);tabs.addTab(english?"Magnitude":"Magnitud",juce::Colour(0xff222427),magnitude,true);tabs.addTab(english?"Phase":"Fase",juce::Colour(0xff222427),phase,true);tabs.addTab(english?"Coherence":"Coherencia",juce::Colour(0xff222427),coherence,true);
  auto* grid=general->add("Frecuencia / reticula",std::make_unique<juce::ComboBox>());if(english)grid->addItemList({"Log: octaves","Log: decades","Log: thirds","Linear"},1);else grid->addItemList({"Log: octavas","Log: decadas","Log: tercios","Lineal"},1);grid->onChange=[this,grid]{auto s=read();s.frequencyGrid=grid->getSelectedId();apply(s);};sync.push_back([this,grid]{grid->setSelectedId(read().frequencyGrid,juce::dontSendNotification);});
  flag(*general,"Mostrar IR (demo)",&TFDisplayState::showIR,407);number(*general,"Grosor frontal (px)",&TFDisplayState::foregroundWidth);number(*general,"Grosor fondo (px)",&TFDisplayState::backgroundWidth);
  flag(*general,"Vincular fase y magnitud",&TFDisplayState::linkPhaseMag,301);flag(*general,"Vincular zoom de frecuencia X",&TFDisplayState::linkFrequencyZoom);flag(*general,"Permitir sin suavizado",&TFDisplayState::allowNone);
  pending(*general,"Link Global Plot Smoothing","Pendiente: enlace global del mismo tipo");pending(*general,"Ponderacion visual / selector","Pendiente; distinta de SPL");pending(*general,"IR proporcional","Pendiente: datos compatibles y motor");pending(*general,"Respuesta instantanea","Pendiente: adquisicion real");
  number(*magnitude,"Minimo (dB)",&TFDisplayState::magMin);number(*magnitude,"Maximo (dB)",&TFDisplayState::magMax);number(*magnitude,"Reticula (dB)",&TFDisplayState::magGrid);number(*magnitude,"Paso desplazar (dB)",&TFDisplayState::magScroll);number(*magnitude,"Paso zoom (dB)",&TFDisplayState::magZoom);pending(*magnitude,"Track Peak","Pendiente: seguimiento de pico validado");pending(*magnitude,"Mag Averaging Display Polar","Pendiente: estimador y modo de promedio");pending(*magnitude,"Ponderacion visual","Pendiente: ponderacion de presentacion");pending(*magnitude,"Display Weighting","Pendiente: selector visual");
  number(*phase,"Minimo (grados)",&TFDisplayState::phaseMin);number(*phase,"Maximo (grados)",&TFDisplayState::phaseMax);number(*phase,"Reticula (grados)",&TFDisplayState::phaseGrid);number(*phase,"Paso desplazar (grados)",&TFDisplayState::phaseScroll);number(*phase,"Paso zoom (grados)",&TFDisplayState::phaseZoom);pending(*phase,"Unwrap","Pendiente; se muestra fase envuelta");pending(*phase,"Frecuencia central unwrap","Pendiente: fase desenrollada");pending(*phase,"Rango unwrap","Pendiente: fase desenrollada");pending(*phase,"Group delay","Pendiente: calculo y pruebas");
  flag(*coherence,"Mostrar coherencia roja (C)",&TFDisplayState::coherenceOn,408);flag(*coherence,"Coherencia cuadrada (gamma^2)",&TFDisplayState::coherenceSquared);number(*coherence,"Umbral (%) mag + fase",&TFDisplayState::coherenceThreshold);
  auto* height=coherence->add("Altura de coherencia",std::make_unique<juce::ComboBox>());height->addItem("1/4",1);height->addItem("1/2",2);height->onChange=[this,height]{auto s=read();s.coherenceHeight=height->getSelectedId()==2?.5:.25;apply(s);};sync.push_back([this,height]{height->setSelectedId(read().coherenceHeight>.3?2:1,juce::dontSendNotification);});
  number(*coherence,"Grosor propio (px)",&TFDisplayState::coherenceWidth);flag(*coherence,"Seguir grosor frontal",&TFDisplayState::coherenceFollowTrace);
  pending(*coherence,"Always Display MTW Coherence","Pendiente: motor MTW");information(*coherence,"Dato base: gamma^2 (demo)","Modelo S/(S+N). Normal = raiz; cuadrada = base, sin volver a cuadrar.","Synthetic model S/(S+N). Normal = square root; squared = stored base.");information(*coherence,"Ocultar no desactiva umbral","El original no cambia; ausente no es 100%.","Hiding keeps the threshold; originals unchanged; missing is not 100%.");
  setSize(650,430);
 }
 void paint(juce::Graphics&) override{for(auto& f:sync)f();}
 void resized() override{tabs.setBounds(getLocalBounds());}
};
