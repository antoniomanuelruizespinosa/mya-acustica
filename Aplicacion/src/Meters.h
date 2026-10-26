#pragma once
#include "MeterState.h"
#include "MeterAlarm.h"
#include "Commands.h"
#include <juce_gui_extra/juce_gui_extra.h>

inline bool meterInputHasFocus(){
 for(auto* focus=juce::Component::getCurrentlyFocusedComponent();focus;focus=focus->getParentComponent())if(dynamic_cast<juce::TextEditor*>(focus)||dynamic_cast<juce::ComboBox*>(focus)||dynamic_cast<juce::Slider*>(focus))return true;
 return false;
}
class MeterTile : public juce::Component,private juce::Timer {
 MeterState& model;juce::String id;juce::TextEditor name;juce::ComboBox input,metric,weight;juce::TextButton remove{"X"},previous{"<"},next{">"},alarms{"Alarmas"},test{"Probar aviso"};int demo=0;
 void timerCallback() override{if(!isShowing()){demo=0;stopTimer();return;}repaint();}
 void configureAlarms(){auto* c=model.find(id);if(!c)return;auto* box=new juce::AlertWindow("Alarmas / Alarms",juce::String::fromUTF8("105/110 son ejemplos SPL, no límites universales. En dBFS use valores <= 0. La alarma queda ligada al tipo y ponderación actuales. Histéresis en la misma unidad."),juce::MessageBoxIconType::NoIcon);box->addTextEditor("warning",juce::String(c->warning),"Aviso / Warning");box->addTextEditor("alarm",juce::String(c->alarm),"Alarma / Alarm");box->addTextEditor("hysteresis",juce::String(c->hysteresis),"Histeresis / Hysteresis");box->addComboBox("animation",{"Pulsante / Pulsing","Fijo / Fixed"},"Fondo / Background");box->getComboBoxComponent("animation")->setSelectedId(c->animate?1:2);box->addComboBox("enabled",{"Activo / Enabled","Desactivado / Disabled"},"Alarmas / Alarms");box->getComboBoxComponent("enabled")->setSelectedId(c->alarmEnabled?1:2);box->addButton("OK",1);box->addButton("Cancelar / Cancel",0);juce::Component::SafePointer<MeterTile> safe(this);box->enterModalState(true,juce::ModalCallbackFunction::create([safe,box](int result){if(!safe||result!=1)return;auto* old=safe->model.find(safe->id);if(!old)return;auto card=*old;auto warning=juce::JSON::parse(box->getTextEditorContents("warning").replaceCharacter(',','.'));auto alarm=juce::JSON::parse(box->getTextEditorContents("alarm").replaceCharacter(',','.'));auto hysteresis=juce::JSON::parse(box->getTextEditorContents("hysteresis").replaceCharacter(',','.'));auto number=[](const juce::var& v){return v.isDouble()||v.isInt()||v.isInt64();};if(!number(warning)||!number(alarm)||!number(hysteresis))return;card.warning=double(warning);card.alarm=double(alarm);card.hysteresis=double(hysteresis);card.animate=box->getComboBoxComponent("animation")->getSelectedId()==1;card.alarmEnabled=box->getComboBoxComponent("enabled")->getSelectedId()==1;card.alarmMetric=card.metric;card.alarmWeight=card.weight;if(!safe->model.update(card))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Alarmas",juce::String::fromUTF8("Valores inválidos: aviso < alarma; histéresis 0..20; dBFS <=0. No se cambiaron los ajustes."));}),true);}

 std::vector<MeterState::Source> sources;bool constructing=true;
 void update(){if(constructing)return;if(auto* old=model.find(id)){auto c=*old;c.name=name.getText().trim();if(c.name.isEmpty())c.name="Medidor";c.metric=metric.getSelectedId();c.weight=weight.getSelectedId();int selected=input.getSelectedId();if(selected==1){c.deviceKey.clear();c.channel=0;}else if(selected>=2&&(size_t)(selected-2)<sources.size()){const auto& source=sources[(size_t)selected-2];c.deviceKey=source.deviceKey;c.channel=source.channel;}model.update(c);}}
public:
 MeterTile(MeterState& state,const MeterState::Card& c):model(state),id(c.id),sources(model.sources()){
  name.setText(c.name,false);name.setTitle("Nombre del medidor / Meter name");name.onReturnKey=[this]{update();};name.onFocusLost=[this]{update();};addAndMakeVisible(name);
  input.addItem("Sin asignar / Unassigned",1);int selected=c.deviceKey.isEmpty()?1:0;
  for(size_t i=0;i<sources.size();++i){input.addItem(sources[i].label,(int)i+2);if(MeterState::sameInput(c,sources[i]))selected=(int)i+2;}
  if(!selected){input.addItem(c.deviceKey+" / "+juce::String(c.channel)+" [Unavailable]",(int)sources.size()+2);selected=(int)sources.size()+2;}
  input.setSelectedId(selected,juce::dontSendNotification);input.setTitle("Entrada / micro (dispositivo y canal)");input.setTooltip("Physical input; configuration only / Sin audio");input.onChange=[this]{update();};addAndMakeVisible(input);
  metric.addItemList({"dBFS Peak","SPL Fast","SPL Slow","SPL Peak","Leq 10 s"},1);metric.setSelectedId(c.metric,juce::dontSendNotification);metric.setTitle("Tipo de lectura / Meter type");metric.setTooltip(juce::String::fromUTF8("Configuración de lectura futura; no hay estimador de audio activo"));metric.onChange=[this]{update();};addAndMakeVisible(metric);
  weight.addItemList({"Z / sin ponderar","A","C"},1);weight.setSelectedId(c.weight,juce::dontSendNotification);weight.setEnabled(c.metric!=1);weight.setTitle(juce::String::fromUTF8("Ponderación / Weighting"));weight.onChange=[this]{update();};addAndMakeVisible(weight);
  remove.onClick=[this]{model.remove(id);};previous.onClick=[this]{model.move(id,-1);};next.onClick=[this]{model.move(id,1);};
  remove.setTooltip("Quitar tarjeta / Remove tile");previous.setTooltip("Mover antes / Move earlier");next.setTooltip(juce::String::fromUTF8("Mover después / Move later"));alarms.onClick=[this]{configureAlarms();};test.onClick=[this]{demo=(demo+1)%3;auto* currentCard=model.find(id);if(demo&&currentCard&&currentCard->animate)startTimerHz(25);else stopTimer();repaint();};test.setTooltip("DEMO: aviso naranja, alarma roja, fin. No audio; no valor medido.");for(auto* b:{&remove,&previous,&next,&alarms,&test})addAndMakeVisible(*b);
  constructing=false;setEnabled(model.ready());
 }
 void refreshFromModel(){auto* c=model.find(id);if(!c)return;constructing=true;if(!name.hasKeyboardFocus(true))name.setText(c->name,false);int sourceId=c->deviceKey.isEmpty()?1:0;for(size_t i=0;i<sources.size();++i)if(MeterState::sameInput(*c,sources[i]))sourceId=(int)i+2;if(!sourceId){sourceId=(int)sources.size()+2;if(input.indexOfItemId(sourceId)<0)input.addItem(c->deviceKey+" / "+juce::String(c->channel)+" [Unavailable]",sourceId);else input.changeItemText(sourceId,c->deviceKey+" / "+juce::String(c->channel)+" [Unavailable]");}input.setSelectedId(sourceId,juce::dontSendNotification);metric.setSelectedId(c->metric,juce::dontSendNotification);weight.setSelectedId(c->weight,juce::dontSendNotification);weight.setEnabled(c->metric!=1);if(!c->animate)stopTimer();constructing=false;repaint();}
 void paint(juce::Graphics& g) override{
  auto r=getLocalBounds().toFloat().reduced(2);auto* c=model.find(id);double pulse=c&&c->animate?.72+.18*std::sin(juce::Time::getMillisecondCounterHiRes()*.004):.9;auto base=juce::Colour(0xff080a0c);auto colour=demo==1?juce::Colour(0xffff9500):juce::Colour(0xffe83232);g.setColour(demo?base.interpolatedWith(colour,(float)pulse):base);g.fillRoundedRectangle(r,5);g.setColour(juce::Colour(0xff626e78));g.drawRoundedRectangle(r,5,1);
  g.setColour(juce::Colours::white);g.setFont((float)juce::jlimit(28,76,getHeight()/6));g.drawText("--",8,177,getWidth()-16,juce::jmax(30,getHeight()-251),juce::Justification::centred);
  g.setFont(11.f);if(demo)g.drawText(juce::String::fromUTF8(demo==1?"AVISO · DEMO":"ALARMA · DEMO"),8,157,getWidth()-16,18,juce::Justification::centred);g.drawText("Max: --",8,getHeight()-55,juce::jmax(20,getWidth()-74),20,juce::Justification::centredLeft);g.setFont(10.f);g.drawFittedText("Sin audio / sin calibrar",6,getHeight()-25,getWidth()-12,18,juce::Justification::centred,1);
 }
 void resized() override{
  int w=getWidth(),h=getHeight();name.setBounds(8,8,w-45,24);remove.setBounds(w-32,8,24,24);alarms.setBounds(8,133,(w-22)/2,22);test.setBounds(14+(w-22)/2,133,(w-22)/2,22);input.setBounds(8,41,w-16,24);metric.setBounds(8,72,w-16,24);weight.setBounds(8,103,w-16,24);previous.setBounds(w-62,h-55,24,22);next.setBounds(w-34,h-55,24,22);
 }
};
class MeterPanel : public juce::Component,private juce::AsyncUpdater {
 MeterState& model;bool compact;int listener=0;juce::TextButton add{"+"};juce::ComboBox rows,columns;juce::Label rowLabel,colLabel,help;juce::Viewport viewport;juce::Component content;
 std::vector<std::unique_ptr<MeterTile>> tiles;std::vector<juce::String> ids;juce::String sourceSignature;MyaCommands::MeterSequence sequence;bool refreshing=false;
 void handleAsyncUpdate() override{refresh();}
 void grid(bool rowChanged){if(refreshing)return;int r=rows.getSelectedId(),c=columns.getSelectedId();int count=(int)model.state().cards.size();if(r<1||c<1){refresh();return;}if(rowChanged)c=std::max(c,(count+r-1)/r);else r=std::max(r,(count+c-1)/c);if(!model.setGrid(r,c))refresh();}
 void refresh(){
  refreshing=true;const auto& s=model.state();std::vector<juce::String> currentIds;for(const auto& c:s.cards)currentIds.push_back(c.id);juce::String signature;for(const auto& source:model.sources())signature+=source.deviceKey+"/"+juce::String(source.channel)+"/"+source.label+"\n";bool rebuild=currentIds!=ids||signature!=sourceSignature;if(rebuild){tiles.clear();ids=currentIds;sourceSignature=signature;}for(int n:{s.rows,s.columns}){if(rows.indexOfItemId(n)<0)rows.addItem(juce::String(n),n);if(columns.indexOfItemId(n)<0)columns.addItem(juce::String(n),n);}rows.setSelectedId(s.rows,juce::dontSendNotification);columns.setSelectedId(s.columns,juce::dontSendNotification);
  if(rebuild){for(const auto& c:s.cards){auto tile=std::make_unique<MeterTile>(model,c);content.addAndMakeVisible(*tile);tiles.push_back(std::move(tile));}}else for(auto& tile:tiles)tile->refreshFromModel();
  help.setText(model.ready()?"Sin audio / SPL sin calibrar":model.lastError(),juce::dontSendNotification);add.setEnabled(model.ready());rows.setEnabled(model.ready());columns.setEnabled(model.ready());refreshing=false;resized();
 }
public:
 std::function<void()> toggleRequested;
 MeterPanel(MeterState& state,bool docked=false):model(state),compact(docked){
  for(int i=1;i<=64;++i){rows.addItem(juce::String(i),i);columns.addItem(juce::String(i),i);}rows.onChange=[this]{grid(true);};columns.onChange=[this]{grid(false);};add.onClick=[this]{model.add();};add.setTooltip(juce::String::fromUTF8("Añadir medidor / Add meter"));
  rowLabel.setText("F",juce::dontSendNotification);colLabel.setText("C",juce::dontSendNotification);rows.setTitle("Filas / Rows");columns.setTitle("Columnas / Columns");help.setFont(juce::Font(juce::FontOptions(10.f)));
  for(auto* c:std::initializer_list<juce::Component*>{&add,&rows,&columns,&rowLabel,&colLabel,&help,&viewport})addAndMakeVisible(c);
  viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,true);listener=model.listen([this]{triggerAsyncUpdate();});refresh();
 }
 ~MeterPanel() override{model.unlisten(listener);cancelPendingUpdate();viewport.setViewedComponent(nullptr,false);}
 bool keyPressed(const juce::KeyPress& key) override{
  if(meterInputHasFocus()){sequence.reset();return false;}auto mods=key.getModifiers();if(!mods.isCommandDown()&&!mods.isCtrlDown()&&!mods.isAltDown()&&(key.getKeyCode()=='E'||key.getKeyCode()=='e')){int id=sequence.press(juce::Time::getMillisecondCounterHiRes(),true);if(id&&toggleRequested)toggleRequested();return true;}sequence.reset();return false;
 }
 void paint(juce::Graphics& g) override{g.fillAll(juce::Colour(0xff222427));}
 void resized() override{
  int w=getWidth(),header=compact?64:50;add.setBounds(3,3,28,25);rowLabel.setBounds(35,3,12,25);rows.setBounds(48,3,42,25);colLabel.setBounds(94,3,12,25);columns.setBounds(107,3,juce::jmax(38,compact?w-110:48),25);help.setBounds(compact?3:168,compact?31:3,compact?w-6:juce::jmax(10,w-174),25);
  if(compact){rows.setBounds(48,3,37,25);colLabel.setBounds(89,3,10,25);columns.setBounds(101,3,juce::jmax(35,w-104),25);}
  viewport.setBounds(0,header,w,juce::jmax(1,getHeight()-header));const auto& s=model.state();int cellW=std::max(compact?140:220,(w-16)/s.columns),cellH=std::max(280,(viewport.getHeight()-16)/s.rows);content.setSize(cellW*s.columns,cellH*s.rows);
  for(size_t i=0;i<tiles.size();++i)tiles[i]->setBounds(((int)i%s.columns)*cellW,((int)i/s.columns)*cellH,cellW-3,cellH-3);
 }
};
class MeterWindow : public juce::DocumentWindow,private juce::Timer {
 MeterState& model;bool initialising=true;MyaCommands::MeterSequence sequence;
 void saveBounds(){if(!initialising)model.window(getX(),getY(),getWidth(),getHeight());}
 void timerCallback() override{stopTimer();saveBounds();}
public:
 MeterPanel* panel;
 explicit MeterWindow(MeterState& state):DocumentWindow("Mya Acustica - dB Meter",juce::Colour(0xff222427),DocumentWindow::allButtons),model(state){
  setUsingNativeTitleBar(true);panel=new MeterPanel(model);setContentOwned(panel,true);setResizable(true,false);setResizeLimits(480,340,6000,6000);
  const auto& s=model.state();juce::Rectangle<int> bounds(s.x,s.y,s.width,s.height);if(auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(bounds)){if(!bounds.intersects(display->userArea))bounds=bounds.withPosition(display->userArea.getPosition());}setBounds(bounds);initialising=false;
 }
 ~MeterWindow() override{stopTimer();saveBounds();}
 void moved() override{if(!initialising)startTimer(200);}
 void resized() override{DocumentWindow::resized();if(!initialising)startTimer(200);}
 void closeButtonPressed() override{stopTimer();saveBounds();setVisible(false);}
 bool keyPressed(const juce::KeyPress& key) override{
  if(meterInputHasFocus()){sequence.reset();return false;}auto mods=key.getModifiers();if(!mods.isCommandDown()&&!mods.isCtrlDown()&&!mods.isAltDown()&&(key.getKeyCode()=='E'||key.getKeyCode()=='e')){if(sequence.press(juce::Time::getMillisecondCounterHiRes(),true))closeButtonPressed();return true;}sequence.reset();return false;
 }
};
