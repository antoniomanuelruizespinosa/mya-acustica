#include <juce_gui_extra/juce_gui_extra.h>
#include <complex>
#include <optional>
#include "AudioRouting.h"
#include "LiveInputEngine.h"
#include "LoopCalibrationStore.h"
#include "SplCalibrationPanel.h"
#include "InputRoutePreferences.h"
#include "Meters.h"
#include "FractionalSmoothing.h"
#include "PlotPresentation.h"
#include "CommandUI.h"
#include "CaptureLibraryPanel.h"
#include "CapturedTraceDisplay.h"
#include "ImpulseDisplay.h"
#include "GraphViewState.h"
#include "CompactLayout.h"
#include "GraphSettingsWindow.h"
#include "TFInputMeters.h"
#include "WorkspaceViewState.h"
#include "AnchoredAudioOptions.h"
#include "PrincipalMeter.h"
#include "Branding.h"
#include "NativeTitleBranding.h"
#include "UpdateCheck.h"
#include "AppDataLocation.h"
#include "DelayFinderCore.h"
#include "SpectrumCore.h"
using namespace juce;
namespace Palette { const Colour bg{0xff222427}, panel{0xff2c3036}, plot{0xff080a0c}, ink{0xffe5edf4}, muted{0xffa9bac8}, cyan{0xff00e5ed}, blue{0xff159fe5}, orange{0xffffac24}, green{0xff54ee22}, line{0xff626e78}; }
class MyaLook : public LookAndFeel_V4 {
public:
 Font getComboBoxFont(ComboBox& box) override{return Font(FontOptions(box.getHeight()<=20?11.f:13.f));}
 MyaLook(){setColour(Label::textColourId,Palette::ink);setColour(Slider::textBoxTextColourId,Palette::cyan);setColour(Slider::textBoxBackgroundColourId,Colour(0xff111920));setColour(Slider::textBoxOutlineColourId,Palette::line);setColour(Slider::trackColourId,Palette::cyan);setColour(ComboBox::backgroundColourId,Colour(0xff18242d));setColour(ComboBox::textColourId,Palette::cyan);setColour(PopupMenu::backgroundColourId,Palette::panel);setColour(PopupMenu::textColourId,Palette::ink);}
 void drawButtonBackground(Graphics& g,Button& b,const Colour&,bool over,bool down) override {auto r=b.getLocalBounds().toFloat().reduced(.5f);if(b.getProperties()["avgDot"].toString().isNotEmpty()){g.setColour(Colour::fromString(b.getProperties()["avgDot"].toString()));g.fillEllipse(r.reduced(4));return;}if(b.getProperties()["trackCircle"]){float d=jmin(r.getWidth(),r.getHeight())-4;auto c=Rectangle<float>{r.getCentreX()-d/2,r.getCentreY()-d/2,d,d};auto col=Colour::fromString(b.getProperties()["trackColour"].toString());if(b.getToggleState()){g.setColour(col.withAlpha(.3f));g.fillEllipse(c);}g.setColour(b.getToggleState()?col:Palette::muted);g.drawEllipse(c,2);g.drawEllipse(c.reduced(4),1);return;}g.setGradientFill(ColourGradient(Colour(0xff3a4b58),0,0,Colour(0xff172028),0,r.getHeight(),false));g.fillRoundedRectangle(r,4);g.setColour(b.getToggleState()?Palette::cyan:(over?Palette::ink:Palette::line));g.drawRoundedRectangle(r,4,down?2.f:1.f);}
 void drawButtonText(Graphics& g,TextButton& b,bool,bool) override {
 if(b.getProperties()["trackCircle"]||b.getProperties()["avgDot"].toString().isNotEmpty())return;
 auto utility=b.getProperties()["utilityIcon"].toString();if(utility.isNotEmpty()){auto c=b.getLocalBounds().getCentre().toFloat();g.setColour(b.isEnabled()?Palette::ink:Palette::muted);if(utility=="timer"){g.drawEllipse(c.x-7,c.y-7,14,14,1.4f);g.drawLine(c.x,c.y,c.x,c.y-5,1.4f);g.drawLine(c.x,c.y,c.x+4,c.y+2,1.4f);}else{g.drawLine(c.x-6,c.y+6,c.x+5,c.y-5,3);g.drawEllipse(c.x+1,c.y-8,8,8,1.4f);g.drawEllipse(c.x-8,c.y+3,5,5,1.2f);}return;}
 auto icon=b.getProperties()["transportIcon"].toString();
 if(icon.isNotEmpty()){g.setColour(b.isEnabled()?Palette::ink:Palette::muted);float cx=(float)b.getWidth()*.5f,cy=(float)b.getHeight()*.5f;float r=jmin(7.f,(float)b.getHeight()*.27f);if(icon=="play"){Path p;p.addTriangle(cx-r*.7f,cy-r,cx-r*.7f,cy+r,cx+r,cy);g.fillPath(p);}else if(icon=="stop")g.fillRect(cx-r,cy-r,r*2,r*2);else{g.fillRect(cx-r,cy-r,r*.65f,r*2);g.fillRect(cx+r*.35f,cy-r,r*.65f,r*2);}return;}
g.setColour(b.isEnabled()?(b.getToggleState()?Palette::cyan:Palette::ink):Palette::muted.withAlpha(.55f));g.setFont(13.f);g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(4).withTrimmedRight(b.getProperties()["hasOptions"]?24:0),Justification::centred,1);}
};
class TraceSwitch : public Button, private Timer {
 void timerCallback() override{stopTimer();setToggleState(!getToggleState(),dontSendNotification);if(onClick)onClick();}

public:
 std::function<void()> openSettings;
 Colour colour{Palette::green};
 TraceSwitch():Button("Mostrar trazo") {setClickingTogglesState(true);setToggleState(true,dontSendNotification);}
 bool canToggle=true;
 void mouseUp(const MouseEvent& e) override {if(canToggle&&e.getNumberOfClicks()==1&&getLocalBounds().contains(e.getPosition()))startTimer(MouseEvent::getDoubleClickTimeout());}
 void mouseDoubleClick(const MouseEvent&) override{stopTimer();if(openSettings)openSettings();}
 void paintButton(Graphics& g,bool over,bool) override {auto r=getLocalBounds().toFloat().reduced(4);g.setColour(getToggleState()?colour:colour.withAlpha(.18f));g.fillEllipse(r);g.setColour(colour);g.drawEllipse(r,over?2.5f:1.5f);if(!getToggleState()){g.setColour(Palette::muted);g.drawLine(r.getX(),r.getBottom(),r.getRight(),r.getY(),1.5f);g.drawLine(r.getX(),r.getY(),r.getRight(),r.getBottom(),1.5f);}}
};
class ColourEditor : public Component, private ChangeListener {
 ColourSelector selector{ColourSelector::showColourAtTop|ColourSelector::showSliders|ColourSelector::showColourspace};
 std::function<void(Colour)> changed;
public:
 ColourEditor(Colour c,std::function<void(Colour)> fn):changed(std::move(fn)){selector.setCurrentColour(c);selector.addChangeListener(this);addAndMakeVisible(selector);setSize(300,350);}
 ~ColourEditor() override{selector.removeChangeListener(this);}
 void resized() override{selector.setBounds(getLocalBounds());}
 void changeListenerCallback(ChangeBroadcaster*) override{changed(selector.getCurrentColour().withAlpha(1.f));}
};

// Keep a configured channel even while its device/channel is absent. Display state
// must never turn an unrelated name/colour/delay edit into a route reassignment.
static void restoreRouteChannel(ComboBox& box,const StringArray& labels,int selected){
 box.clear(dontSendNotification);box.addItemList(labels,1);
 if(selected>0&&box.indexOfItemId(selected)<0){box.addItem("Input "+String(selected)+" [Unavailable]",selected);box.setItemEnabled(selected,false);}
 box.setSelectedId(selected,dontSendNotification);box.setTextWhenNothingSelected("Not assigned");box.setEnabled(!labels.isEmpty());
}
class RoutingPanel : public Component {
 std::vector<AudioEndpoint> devices;std::vector<String> unavailableKeys;
 std::array<Label,4> names;
 std::array<ComboBox,4> device,mic,ref;
 ComboBox outputDevice,output;
 Label heading;TextButton calibrationButton{"Calibrar SPL RMS"};
 int spectrumRows=0, tfTop=0, avgTop=0, generatorTop=0;
 std::function<Colour(int)> traceColour;
 std::function<void(int,String,String,int,int)> changed;
 std::function<void(String,int)> outputChanged;
 String keyFor(int id)const{if(id>0&&id<=(int)devices.size())return devices[(size_t)id-1].key;int missing=id-(int)devices.size()-1;return missing>=0&&(size_t)missing<unavailableKeys.size()?unavailableKeys[(size_t)missing]:String{};}
 void channels(ComboBox& box,const StringArray& labels,int selected){restoreRouteChannel(box,labels,selected);}
 void update(int i){auto k=(size_t)i;changed(i,names[k].getText(),keyFor(device[k].getSelectedId()),mic[k].getSelectedId(),ref[k].getSelectedId());}
 void inputChannels(int i,int m,int r){auto k=(size_t)i;auto* d=findEndpoint(devices,keyFor(device[k].getSelectedId()));channels(mic[k],d?d->inputs:StringArray(),m);channels(ref[k],d?d->inputs:StringArray(),r);}
 void populateDevice(ComboBox& box,const String& key,bool input){
  for(size_t i=0;i<devices.size();++i)if(!(input?devices[i].inputs:devices[i].outputs).isEmpty())box.addItem(devices[i].name+" ["+devices[i].backend+"]",(int)i+1);
  int selected=0;for(size_t i=0;i<devices.size();++i)if(devices[i].key==key&&box.indexOfItemId((int)i+1)>=0)selected=(int)i+1;
  if(selected==0&&key.isNotEmpty()){
   auto it=std::find(unavailableKeys.begin(),unavailableKeys.end(),key);if(it==unavailableKeys.end()){unavailableKeys.push_back(key);it=std::prev(unavailableKeys.end());}
   selected=(int)devices.size()+1+(int)std::distance(unavailableKeys.begin(),it);box.addItem(key+" [Unavailable]",selected);box.setItemEnabled(selected,false);
  }
  box.setSelectedId(selected,dontSendNotification);
  box.setTextWhenNothingSelected("Not assigned");
 }
public:
 std::function<void()> calibrateRequested;
 void setAcquisitionState(bool active,bool waiting=false,const String& details={}){auto text=String((int)devices.size())+" dispositivos / "+(waiting?"Esperando permiso de microfono":active?"Entradas dBFS activas":"Entradas detenidas");if(heading.getText()!=text)heading.setText(text,dontSendNotification);heading.setTooltip(details);}
 #if defined(MYA_OFFSCREEN_RENDER)
 String previewStatus()const{return heading.getText();}
 void fixtureSelectDevice(int i,int id){device[(size_t)i].setSelectedId(id,sendNotificationSync);}
 void fixtureRename(int i){names[(size_t)i].setText("Renamed offline",dontSendNotification);update(i);}
 #endif
 RoutingPanel(std::vector<AudioEndpoint> catalog,const std::array<String,4>& n,const std::array<String,4>& routes,const std::array<int,4>& m,const std::array<int,4>& r,String outKey,int outChannel,std::function<Colour(int)> colours,std::function<void(int,String,String,int,int)> change,std::function<void(String,int)> outChange):devices(std::move(catalog)),traceColour(std::move(colours)),changed(std::move(change)),outputChanged(std::move(outChange)){
 for(const auto& d:devices)spectrumRows+=d.inputs.size();
 tfTop=116+jmax(1,spectrumRows)*30;avgTop=tfTop+234;generatorTop=avgTop+82;
 setAcquisitionState(false);addAndMakeVisible(heading);addAndMakeVisible(calibrationButton);calibrationButton.onClick=[this]{if(calibrateRequested)calibrateRequested();};
 for(int i=0;i<4;++i){auto k=(size_t)i;names[k].setText(n[k],dontSendNotification);names[k].setEditable(false,true,false);addAndMakeVisible(names[k]);for(auto* c:{&device[k],&mic[k],&ref[k]})addAndMakeVisible(c);
 populateDevice(device[k],routes[k],true);inputChannels(i,m[k],r[k]);
 names[k].onTextChange=[this,i]{update(i);};device[k].onChange=[this,i]{inputChannels(i,0,0);update(i);};
 mic[k].onChange=[this,i]{update(i);};ref[k].onChange=[this,i]{update(i);};
 }
 addAndMakeVisible(outputDevice);addAndMakeVisible(output);populateDevice(outputDevice,outKey,false);
 auto* d=findEndpoint(devices,outKey);channels(output,d?d->outputs:StringArray(),outChannel);
 outputDevice.onChange=[this]{auto key=keyFor(outputDevice.getSelectedId());auto* endpoint=findEndpoint(devices,key);channels(output,endpoint?endpoint->outputs:StringArray(),0);outputChanged(key,0);};
 output.onChange=[this]{outputChanged(keyFor(outputDevice.getSelectedId()),output.getSelectedId());};
 setSize(690,generatorTop+120);
 }
 void paint(Graphics& g) override{
 g.setFont(12);g.setColour(Palette::ink);
 g.drawText("Measurement Configuration",16,46,200,26,Justification::centredLeft);g.drawText("Device",226,46,180,26,Justification::centredLeft);g.drawText("Meas. Ch.",422,46,110,26,Justification::centredLeft);g.drawText("Ref. Ch.",554,46,110,26,Justification::centredLeft);
 g.setColour(Palette::line);g.drawRect(12,44,660,avgTop+68-44);for(int x:{218,418,550})g.drawVerticalLine(x,44.f,(float)(avgTop+68));
 auto section=[&](String title,int y){g.setColour(Colour(0xff1b2228));g.fillRect(13,y,658,28);g.setColour(Palette::ink);g.drawText("-  "+title,18,y,640,28,Justification::centredLeft);};
 section("Spectrum / available inputs",78);
 int row=0;for(const auto& d:devices)for(const auto& ch:d.inputs){int y=110+row*30;g.setColour(Colour::fromHSV((float)(row%12)/12.f,.8f,.9f,1.f));g.fillRect(40,y+6,20,16);g.setColour(Palette::ink);g.drawFittedText(ch,68,y,145,28,Justification::centredLeft,1);g.drawFittedText(d.name,226,y,185,28,Justification::centredLeft,1);g.drawFittedText(ch,426,y,118,28,Justification::centredLeft,1);++row;}
 if(row==0){g.setColour(Palette::muted);g.drawText("No input channels detected",32,110,620,28,Justification::centredLeft);}
 section("Transfer Function",tfTop);
 for(int i=0;i<4;++i){int y=tfTop+34+i*48;g.setColour(traceColour(i));g.fillRect(32,y+9,20,18);}
 section("Transfer Function Live Average",avgTop);
 g.setColour(Palette::muted);g.drawText("No live averages configured / averaging engine pending",32,avgTop+32,630,26,Justification::centredLeft);
 section("Signal Generator",generatorTop);
 g.setColour(Palette::muted);g.drawText("Output Device",16,generatorTop+30,390,24,Justification::centredLeft);g.drawText("Output",422,generatorTop+30,240,24,Justification::centredLeft);
 }
 void resized() override{
 heading.setBounds(12,4,460,32);calibrationButton.setBounds(490,6,182,28);
 for(int i=0;i<4;++i){auto k=(size_t)i;int y=tfTop+38+i*48;names[k].setBounds(58,y,155,30);device[k].setBounds(224,y,188,30);mic[k].setBounds(424,y,120,30);ref[k].setBounds(556,y,110,30);}
 outputDevice.setBounds(16,generatorTop+60,398,30);output.setBounds(422,generatorTop+60,250,30);
 }

};

class EnginePanel : public Component {
 TextEditor name;
 Slider delay;
 ComboBox mic,ref;
 ColourSelector colour{ColourSelector::showColourAtTop|ColourSelector::showSliders|ColourSelector::showColourspace};
 std::vector<std::unique_ptr<Label>> labels;
 std::vector<std::unique_ptr<ComboBox>> pending;
 std::function<void(String,double,int,int,Colour)> apply;
 class ColourListener : public ChangeListener {public:std::function<void()> callback;void changeListenerCallback(ChangeBroadcaster*) override{if(callback)callback();}} listener;
 void update(){apply(name.getText(),delay.getValue(),mic.getSelectedId(),ref.getSelectedId(),colour.getCurrentColour().withAlpha(1.f));}
 void label(String title,int x,int y,int w){auto l=std::make_unique<Label>();l->setText(title,dontSendNotification);l->setBounds(x,y,w,24);l->setColour(Label::textColourId,Palette::ink);addAndMakeVisible(*l);labels.push_back(std::move(l));}
public:
 EnginePanel(String n,double d,int m,int r,Colour c,String deviceName,StringArray inputChannels,std::function<void(String,double,int,int,Colour)> fn):apply(std::move(fn)){
 setSize(690,570);label("AJUSTES DE MEDICION",16,8,320);label("Nombre",16,42,85);name.setText(n);name.setBounds(105,42,220,25);addAndMakeVisible(name);
 label("Delay (ms)",16,79,90);delay.setRange(0,2000,.01);delay.setSliderStyle(Slider::IncDecButtons);delay.setTextBoxStyle(Slider::TextBoxLeft,false,160,25);delay.setValue(d);delay.setBounds(105,79,220,25);addAndMakeVisible(delay);
 label("AJUSTES DE VISUALIZACION / COLOR",16,127,400);colour.setCurrentColour(c);colour.setBounds(16,158,310,280);colour.addChangeListener(&listener);addAndMakeVisible(colour);
 const StringArray options{"FFT / usar global","Profundidad de promedio / global","Tipo de promedio / global","Suavizado de fase / seguir grafica","Suavizado de magnitud / seguir grafica","Ponderacion / seguir grafica","Grafica de destino","Invertir magnitud visual"};
 for(int i=0;i<options.size();++i){int y=8+i*48;label(options[i],350,y,325);auto cb=std::make_unique<ComboBox>();cb->addItem("Pendiente de motor de analisis",1);cb->setSelectedId(1);cb->setEnabled(false);cb->setBounds(350,y+23,320,23);addAndMakeVisible(*cb);pending.push_back(std::move(cb));}
 label("Input / "+deviceName,16,447,650);restoreRouteChannel(mic,inputChannels,m);restoreRouteChannel(ref,inputChannels,r);mic.setBounds(16,480,310,28);ref.setBounds(350,480,320,28);addAndMakeVisible(mic);addAndMakeVisible(ref);label("Cambios aplicados a la demo. Guardar en la ventana principal conserva el proyecto.",16,529,660);
 name.onTextChange=[this]{update();};delay.onValueChange=[this]{update();};mic.onChange=[this]{update();};ref.onChange=[this]{update();};listener.callback=[this]{update();};
 }
 #if defined(MYA_OFFSCREEN_RENDER)
 void fixtureRename(){name.setText("Renamed offline",dontSendNotification);update();}
 #endif
 ~EnginePanel()override{colour.removeChangeListener(&listener);}
};

class SettingsDrawer : public Component {
 std::unique_ptr<Component> content;
 Viewport viewport;
 Label title;
 TextButton close{"X"};
 Colour accent=Palette::cyan; bool hasAccent=false;
public:
 std::function<void()> requestClose;
 SettingsDrawer(String text,std::unique_ptr<Component> body):content(std::move(body)){
 title.setText(text,dontSendNotification);title.setColour(Label::textColourId,Palette::cyan);addAndMakeVisible(title);close.setTooltip("Plegar panel");close.onClick=[this]{if(requestClose)requestClose();else setVisible(false);};addAndMakeVisible(close);
 viewport.setViewedComponent(content.get(),false);viewport.setScrollBarsShown(true,true);addAndMakeVisible(viewport);setWantsKeyboardFocus(true);
 }
 void setAccent(Colour c){accent=c;hasAccent=true;title.setColour(Label::textColourId,c);resized();repaint();}
 void paint(Graphics& g) override {g.setColour(Colour(0xf51b2228));g.fillRoundedRectangle(getLocalBounds().toFloat(),6);g.setColour(Palette::line);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),6,1.5f);if(hasAccent){g.setColour(accent);g.fillEllipse(14,13,18,18);}}
 void resized() override{title.setBounds(hasAccent?42:14,5,getWidth()-(hasAccent?93:65),30);close.setBounds(getWidth()-40,7,28,26);viewport.setBounds(8,42,getWidth()-16,getHeight()-50);}
 bool keyPressed(const KeyPress& key) override{if(key==KeyPress::escapeKey&&Component::getCurrentlyFocusedComponent()==this){if(requestClose)requestClose();else setVisible(false);return true;}return false;}
};
class PaintSurface : public Component {public: std::function<void(Graphics&)> draw;void paint(Graphics& g) override{if(draw)draw(g);}};
class PlotDivider : public Component {
public:
 std::function<void(int)> moved;
 std::function<void()> reset;
 PlotDivider(){setMouseCursor(MouseCursor::UpDownResizeCursor);setTitle("Arrastrar para redimensionar graficas");}
 void mouseDrag(const MouseEvent& e)override{if(moved)moved(e.getEventRelativeTo(getParentComponent()).y);}
 void mouseDoubleClick(const MouseEvent&)override{if(reset)reset();}
 void paint(Graphics& g)override{g.setColour(isMouseOverOrDragging()?Palette::cyan:Palette::line);g.fillRoundedRectangle((float)getWidth()/2-24,2,48,3,1.5f);}
};
class SidebarDivider : public Component {
public:
 std::function<void(int)> moved;
 SidebarDivider(){setMouseCursor(MouseCursor::LeftRightResizeCursor);setTitle("Arrastrar ancho lateral");}
 void mouseDrag(const MouseEvent& e)override{if(moved)moved(e.getEventRelativeTo(getParentComponent()).x);}
 void paint(Graphics& g)override{g.setColour(isMouseOverOrDragging()?Palette::cyan:Palette::line.withAlpha(.4f));g.fillRoundedRectangle(2.f,(float)getHeight()*.45f,2.f,32.f,1.f);}
};
class Workstation : public Component, public KeyListener, public MenuBarModel, private ChangeListener, private Timer {
 MyaLook look;
 std::unique_ptr<MyaUpdate::Checker> updateChecker;
 PaintSurface navigationOverlay;TextButton navigationTab{"v"},navigationMenus{"Todos los menus..."};
 AudioDeviceWatcher deviceWatcher;
 std::unique_ptr<SettingsDrawer> settingsDrawer;
 String drawerKey;
 std::vector<std::unique_ptr<TextButton>> buttons;
 TextButton *measure,*dsp,*vision,*lab,*invert,*bypass;
 Slider gain,delay,stacks,boxes,spacing,rotation;
 Label gainLabel,delayLabel,stacksLabel,boxesLabel,spacingLabel,rotationLabel;
 ComboBox source, plotChoice, averageChoice, meterInput, meterWeight, generatorOutput,spectrumFFT,spectrumSpeed,spectrumWindow,spectrumAverage;
 std::array<ComboBox,3> plotMic;
 std::array<ComboBox,2> plotSmooth,plotTypeChoice;
 std::array<TextButton,3> graphGear;
 std::unique_ptr<GraphSettingsWindow> graphWindow;bool graphWindowEnglish=false;
 TFDisplayState tfSettings;
 ComboBox impulseScale;
 std::array<int,2> plotTypes{{1,2}};
 int irScale=0;
 ImpulseDisplay impulseDisplay; // retained only for legacy DSP mock views; never used as measured IR
 int displayType(int panel) const{return panel==0?0:plotTypes[(size_t)(panel-1)];}
 String displayName(int panel) const{return displayType(panel)==1?juce::String::fromUTF8("FASE · grados"):juce::String::fromUTF8("MAGNITUD · dB / Hz");}

 Slider coherenceThreshold;
 ToggleButton showCoherence{"Coherencia"};
 // Deterministic synthetic direct sound plus reflections; never presented as a measurement.
 static std::complex<double> demoTransfer(double f){double u=std::log(f/20.)/std::log(1000.);double db=2.8*std::sin(u*12)+1.1*std::sin(u*31);auto h=std::polar(std::pow(10.,db/20),.65*std::sin(u*9));auto tau=2*MathConstants<double>::pi*f;auto room=std::complex<double>(1.,0)+std::polar(.42,-tau*.0037)+std::polar(.23,-tau*.0113)+std::polar(.12,-tau*.0271);double ripple=.23*std::sin(u*1731)+.16*std::sin(u*2963)+.09*std::sin(u*4877);return h*room*std::polar(std::pow(10.,ripple/20),.012*std::sin(u*2137));}
 static double demoCoherence(double u,int s){double f=20*std::pow(1000.,u);double power=std::norm(demoTransfer(f))*std::pow(10.,(.65*s*std::sin(u*8+s))/10);double noise=.025+.22*std::exp(-std::pow((u-.12)*9,2))+.09*std::pow(jlimit(0.,1.,u),4);noise*=1.+.65*std::pow(std::sin(u*2173+s*.9),2)+.4*std::pow(std::sin(u*3917+s),8);return jlimit(0.,1.,power/(power+noise));}
 ToggleButton linkSmooth{"Link Phase & Mag Smoothing"};
 std::array<PlotDivider,2> plotDividers;
 double irFraction=.14, phaseFraction=.5;
 int activePlot=1;
 std::array<double,3> plotPanX{},plotPanY{};
 std::array<double,3> plotZoomX{{1.,1.,1.}}, plotZoomY{{1.,1.,1.}};
 Rectangle<int> plotBounds(int type)const{
 Rectangle<int> c{centerX()+8,118,centerWidth()-16,getHeight()-183};
 if(mode==0&&measurementView!=0)return type==0?c:Rectangle<int>{};
 bool ir=plotChoice.getSelectedId()!=2;int gap=8;
 auto heights=CompactLayout::heights(c.getHeight(),ir,irFraction,phaseFraction);int ih=heights.ir,ph=heights.phase;
 if(type==0)return {c.getX(),c.getY(),c.getWidth(),ih};
 int top=c.getY()+(ir?ih+gap:0);if(type==1)return {c.getX(),top,c.getWidth(),ph};return {c.getX(),top+ph+gap,c.getWidth(),heights.magnitude};
 }

 class Readout : public Label {
 public:
 struct Cell {String label,value;Colour colour;};std::vector<Cell> parts;
 void paint(Graphics& g) override{g.setColour(findColour(Label::backgroundColourId));g.fillRect(getLocalBounds());if(parts.empty())return;
  float cellWidth=(float)getWidth()/(float)parts.size();
  for(size_t i=0;i<parts.size();++i){auto& cell=parts[i];Font labelFont(FontOptions(10.5f)),valueFont(FontOptions(16.5f));float lw=GlyphArrangement::getStringWidth(labelFont,cell.label),vw=GlyphArrangement::getStringWidth(valueFont,cell.value);float available=cellWidth-12;
   if(lw+8+vw>available){valueFont=Font(FontOptions(jmax(12.f,16.5f*(available-lw-8)/jmax(1.f,vw))));vw=GlyphArrangement::getStringWidth(valueFont,cell.value);}
   float total=jmin(available,lw+8+vw),x=(float)i*cellWidth+(cellWidth-total)*.5f;
   g.setFont(labelFont);g.setColour(Palette::muted);g.drawText(cell.label,Rectangle<float>{x,0,lw,(float)getHeight()},Justification::centredLeft);
   g.setFont(valueFont);g.setColour(cell.colour);g.drawFittedText(cell.value,Rectangle<int>{roundToInt(x+lw+8),0,jmax(1,roundToInt(total-lw-8)),getHeight()},Justification::centredLeft,1,.8f);
  }
 }
 };
 Readout cursorReadout;
 int cursorPlot=-1;double cursorU=.5,cursorY=.5;
 std::array<double,3> plotCursorU{{.5,.5,.5}};
 std::array<bool,3> plotCursorVisible{};
 Label generatorLevel;
 PaintSurface tfContent;
 Viewport tfViewport;
 std::vector<std::unique_ptr<TextButton>> measurementButtons;
 std::array<std::array<TextButton,4>,4> tfActions;
 std::array<TFInputMeters,4> inputMeters;
 std::array<TextButton,7> aiButtons;TextButton sumButton,alignmentFold,avgPlay,loopOptions{"..."};Label avgReserve;TraceSwitch avgColourButton;
 Colour avgColour{Palette::orange};std::unique_ptr<AnchoredOptions> anchoredOptions;Component::SafePointer<Component> optionsAnchor;String anchoredKey;
 bool alignmentExpanded=false;
 Slider selectedDelay, phasePreview;
 void captureTrace(){
 int index=source.getSelectedId()-1;if(index<0||index>=4||!tfEngines[(size_t)index].ready()){status=String::fromUTF8("No hay una TF real estabilizada para capturar");repaint();return;}auto& measured=tfEngines[(size_t)index];
 auto k=(size_t)index;auto stamp=Time::getCurrentTime();auto name=micNames[k].getText()+" / "+stamp.formatted("%H:%M:%S");
 DynamicObject::Ptr data=new DynamicObject();data->setProperty("schema",1);data->setProperty("synthetic",false);data->setProperty("name",name);data->setProperty("deviceProfile",deviceKeys[k]);data->setProperty("micInput",micInputs[k]);data->setProperty("refInput",refInputs[k]);data->setProperty("measurementDelayMs",micDelay[k].getValue());data->setProperty("colour",traceSwitch[k].colour.toString());Array<var> samples;
 for(int n=0;n<=1400;++n){double u=n/1400.,f=20*std::pow(1000.,u);auto p=measured.at(f);if(!p)continue;auto h=std::polar(std::pow(10.,p->magnitudeDb/20.),p->phaseDegrees*MathConstants<double>::pi/180.);DynamicObject::Ptr point=new DynamicObject();point->setProperty("frequencyHz",f);point->setProperty("real",h.real());point->setProperty("imaginary",h.imag());point->setProperty("coherence",p->coherence);samples.add(var(point.get()));}
 data->setProperty("createdAtUnixMs",stamp.toMilliseconds());data->setProperty("samples",samples);data->setProperty("dataKind","snapshot-tf");data->setProperty("acquisitionMethod","dual-channel-live");data->setProperty("sampleRateHz",measured.sampleRate());data->setProperty("timeReferenceMs",micDelay[k].getValue());data->setProperty("coherenceConvention","gamma-squared");
 auto id=captureLibrary.capture(var(data.get()),name,traceSwitch[k].colour.toString());
 if(id.isNotEmpty()){status=juce::String::fromUTF8("Captura TF real guardada → ")+captureLibrary.folderPath(captureLibrary.state().active);libraryPanel.refresh();refreshLibrary();}
 else status=captureLibrary.lastError();repaint();
 }
 void addMeasurementButton(String name){auto b=std::make_unique<TextButton>(name);addAndMakeVisible(*b);measurementButtons.push_back(std::move(b));}

 std::array<TraceSwitch,4> traceSwitch;
 std::array<Label,4> micNames;
 std::array<int,4> micInputs{}, refInputs{};
 std::array<String,4> deviceKeys;
 std::vector<AudioEndpoint> audioEndpoints;
 String outputKey; int outputChannel=0;

 std::array<TextButton,4> colourEdit;
 std::array<Slider,4> micDelay;

 bool leftVisible=true,rightVisible=true;
 int leftWidth=WorkspaceViewState::leftMin,rightWidth=WorkspaceViewState::rightMin;
 TFMeterSettings inputMeterSettings;
 SidebarDivider leftDivider,rightDivider;TextButton closeLeft{"<"},closeRight{">"},restoreRight{"<"};
 int centerX() const{return leftVisible?leftWidth+16:8;}
 int centerWidth() const{return getWidth()-centerX()-(rightVisible?rightWidth+16:8);}
 AffineTransform rightTransform()const{float scale=(float)rightWidth/276.f;return AffineTransform::scale(scale,1.f).translated((float)getWidth()-8-scale*(float)(getWidth()-20),0);}
 void persistWorkspace(){WorkspaceViewState state{leftWidth,rightWidth,leftVisible,rightVisible,inputMeterSettings,avgColour.toString()};if(!state.save(projectFile.getParentDirectory().getChildFile("vista-trabajo.json")))status="No se pudo guardar la vista de trabajo";}
 void reflectInputMeters(){for(size_t k=0;k<4;++k){inputMeters[k].colour=traceSwitch[k].colour;inputMeters[k].settings=inputMeterSettings;inputMeters[k].repaint();tfActions[k][1].getProperties().set("trackColour",traceSwitch[k].colour.toString());tfActions[k][1].repaint();}}
 void positionAnchored(){if(!anchoredOptions||!anchoredOptions->isVisible()||!optionsAnchor)return;if(mode!=0||!rightVisible||!optionsAnchor->isVisible()){anchoredOptions->setVisible(false);return;}auto r=getLocalArea(optionsAnchor,optionsAnchor->getLocalBounds());int w=anchoredOptions->getWidth(),h=anchoredOptions->getHeight();int x=r.getX()-w-6;if(x<8)x=jlimit(8,jmax(8,getWidth()-w-8),r.getRight()+6);int y=jlimit(82,jmax(82,getHeight()-h-33),r.getBottom()-h);if(principalMeter.isVisible()&&Rectangle<int>{x,y,w,h}.intersects(principalMeter.getBounds())){int below=principalMeter.getBottom()+6;y=jlimit(below,jmax(below,getHeight()-h-33),jmax(below,r.getY()));}anchoredOptions->setTopLeftPosition(x,y);anchoredOptions->toFront(false);}
 void showAnchored(String key,String title,std::unique_ptr<Component> content,Component& anchor){if(anchoredOptions&&anchoredOptions->isVisible()&&anchoredKey==key){anchoredOptions->setVisible(false);return;}closeCalibration();if(settingsDrawer)settingsDrawer->setVisible(false);anchoredKey=key;optionsAnchor=&anchor;anchoredOptions=std::make_unique<AnchoredOptions>(title,std::move(content));anchoredOptions->requestClose=[this]{anchoredOptions->setVisible(false);};addAndMakeVisible(*anchoredOptions);positionAnchored();}
 void signalOptions(String key){
  Component& anchor=key=="loop"?static_cast<Component&>(loopOptions):static_cast<Component&>(*measurementButtons[key=="pink"?10:12]);
  int selected=source.getSelectedId()-1;bool hasTF=selected>=0&&selected<4;
  String deviceName="Sin tarjeta TF asignada";
  if(hasTF){if(auto* d=findEndpoint(audioEndpoints,deviceKeys[(size_t)selected]))deviceName=d->name+" ["+d->backend+"]";else if(deviceKeys[(size_t)selected].isNotEmpty())deviceName=deviceKeys[(size_t)selected]+" [no disponible]";}
  String outputName=outputKey.isNotEmpty()&&outputChannel>0?"Output "+String(outputChannel):"Sin asignar";if(auto* d=findEndpoint(audioEndpoints,outputKey))outputName+=" / "+d->name;
  String referenceName=hasTF&&refInputs[(size_t)selected]>0?"Input "+String(refInputs[(size_t)selected]):"Sin asignar";
  const auto* tfEndpoint=hasTF?findEndpoint(audioEndpoints,deviceKeys[(size_t)selected]):nullptr;
  StringArray tfChoices,deviceChoices;for(int i=0;i<4;++i)tfChoices.add(micNames[(size_t)i].getText());int selectedDevice=0;for(size_t i=0;i<audioEndpoints.size();++i){const auto& endpoint=audioEndpoints[i];deviceChoices.add(endpoint.name+" ["+endpoint.backend+"]");if((key=="loop"&&hasTF&&endpoint.key==deviceKeys[(size_t)selected])||(key!="loop"&&endpoint.key==outputKey))selectedDevice=(int)i+1;}
  const auto* generatorEndpoint=findEndpoint(audioEndpoints,outputKey);bool isSweep=key=="sweep";double generatorLevel=isSweep?inputEngine.sweepLevel():inputEngine.pinkLevel();
  auto content=std::make_unique<AudioOptionsView>(key=="loop",deviceName,outputName,referenceName,pinkLoopActive&&!internalLoopEnabled,generatorLevel,inputEngine.referenceGain(),key=="loop"?(tfEndpoint?tfEndpoint->outputs:StringArray{}):(generatorEndpoint?generatorEndpoint->outputs:StringArray{}),key=="loop"?(hasTF&&outputKey==deviceKeys[(size_t)selected]?outputChannel:0):outputChannel,tfEndpoint?tfEndpoint->inputs:StringArray{},hasTF?refInputs[(size_t)selected]:0,hasTF?micInputs[(size_t)selected]:0,tfChoices,hasTF?selected+1:0,deviceChoices,selectedDevice,isSweep,isSweep?sweepActive:pinkLoopActive,inputEngine.sweepStart(),inputEngine.sweepEnd(),inputEngine.sweepSeconds(),inputEngine.pinkHighPass(),inputEngine.pinkLowPass());
  auto* contentView=content.get();
  Component::SafePointer<Workstation> safe(this);
  content->onPinkLevel=[safe,isSweep](double db){if(!safe)return;if(isSweep)safe->inputEngine.setSweepLevelDb(db);else safe->inputEngine.setPinkLevelDb(db);safe->persistInputs();safe->refreshLoopOptions();};
  content->onPinkHighPass=[safe](double hz){if(safe){safe->inputEngine.setPinkBand(hz,safe->inputEngine.pinkLowPass());safe->persistInputs();}};content->onPinkLowPass=[safe](double hz){if(safe){safe->inputEngine.setPinkBand(safe->inputEngine.pinkHighPass(),hz);safe->persistInputs();}};
  content->onSweepStart=[safe](double v){if(safe){safe->inputEngine.setSweep(v,safe->inputEngine.sweepEnd(),safe->inputEngine.sweepSeconds());safe->persistInputs();}};content->onSweepEnd=[safe](double v){if(safe){safe->inputEngine.setSweep(safe->inputEngine.sweepStart(),v,safe->inputEngine.sweepSeconds());safe->persistInputs();}};content->onSweepDuration=[safe](double v){if(safe){safe->inputEngine.setSweep(safe->inputEngine.sweepStart(),safe->inputEngine.sweepEnd(),v);safe->persistInputs();}};
  if(key=="loop"){
   content->onTransferFunction=[safe](int tf){if(!safe||tf<1||tf>4){if(safe)safe->refreshLoopOptions();return;}safe->source.setSelectedId(tf,sendNotificationSync);safe->refreshLoopOptions();};
   content->onDevice=[safe](int device){if(!safe)return;int i=safe->source.getSelectedId()-1;if(i<0||i>=4){safe->status="Selecciona TF 1-4 para configurar el loop";safe->refreshLoopOptions();return;}String selectedKey;if(device>0&&(size_t)(device-1)<safe->audioEndpoints.size())selectedKey=safe->audioEndpoints[(size_t)device-1].key;auto k=(size_t)i;if(safe->deviceKeys[k]==selectedKey){safe->refreshLoopOptions();return;}if(safe->inputEngine.running()||safe->permissionPending)safe->stopInputs("Tarjeta de loop modificada; audio detenido.");safe->deviceKeys[k]=selectedKey;safe->outputKey=selectedKey;safe->outputChannel=0;auto* endpoint=findEndpoint(safe->audioEndpoints,selectedKey);if(!endpoint||safe->micInputs[k]>endpoint->inputs.size())safe->micInputs[k]=0;if(!endpoint||safe->refInputs[k]>endpoint->inputs.size()||safe->refInputs[k]==safe->micInputs[k])safe->refInputs[k]=0;safe->inputEngine.setReferenceGainDb(0);safe->loopCalMessage.clear();safe->persistInputs();safe->syncNames();safe->refreshLoopOptions();};
   content->onOutputChannel=[safe](int channel){if(!safe)return;int i=safe->source.getSelectedId()-1;if(i<0||i>=4)return;auto key=safe->deviceKeys[(size_t)i];auto* endpoint=findEndpoint(safe->audioEndpoints,key);if(!endpoint||channel<0||channel>endpoint->outputs.size())return;if(safe->inputEngine.running()||safe->permissionPending)safe->stopInputs("Ruta de salida modificada; loop detenido.");safe->outputKey=channel>0?key:String{};safe->outputChannel=channel;safe->inputEngine.setReferenceGainDb(0);safe->loopCalMessage.clear();safe->persistInputs();safe->refreshLoopOptions();};
   content->onMicrophoneChannel=[safe](int channel){if(!safe)return;int i=safe->source.getSelectedId()-1;if(i<0||i>=4)return;auto key=safe->deviceKeys[(size_t)i];auto* endpoint=findEndpoint(safe->audioEndpoints,key);if(!endpoint||channel<0||channel>endpoint->inputs.size()||channel==safe->refInputs[(size_t)i]){safe->status="El micro debe ser distinto de la referencia";safe->refreshLoopOptions();return;}safe->micInputs[(size_t)i]=channel;safe->inputRoutesChanged();};
   content->onReferenceChannel=[safe](int channel){if(!safe)return;int i=safe->source.getSelectedId()-1;if(i<0||i>=4)return;auto key=safe->deviceKeys[(size_t)i];auto* endpoint=findEndpoint(safe->audioEndpoints,key);if(!endpoint||channel<0||channel>endpoint->inputs.size()||channel==safe->micInputs[(size_t)i]){safe->status="La referencia debe ser distinta del micro de la TF";safe->refreshLoopOptions();return;}safe->refInputs[(size_t)i]=channel;safe->inputRoutesChanged();};
   content->onReferenceGain=[safe](double db){if(!safe)return;safe->inputEngine.setReferenceGainDb(db);safe->inputEngine.discardPendingTF();safe->liveTF.reset();safe->loopCalMessage="REF Gain manual: "+String(db,1)+" dB";safe->status=safe->loopCalMessage;safe->refreshLoopOptions();};
   content->onPhysicalLoopToggle=[safe](bool active){if(!safe)return;if(active){int i=safe->source.getSelectedId()-1;if(i<0||i>=4){safe->status="Selecciona TF 1-4 para el loop físico";safe->refreshLoopOptions();return;}safe->internalLoopEnabled=false;safe->commands.refresh();safe->beginInputs(1u<<i,{},LiveInputEngine::Signal::Pink);}else safe->stopInputs(String::fromUTF8("Loop físico detenido; salida en silencio."));safe->refreshLoopOptions();};
   content->onCalibrate=[safe]{if(safe)safe->startPhysicalLoopCalibration();};content->onReset=[safe]{if(safe)safe->resetPhysicalLoopCalibration();};
  }else{
   content->onDevice=[safe,contentView](int device){if(!safe)return;String key;if(device>0&&(size_t)(device-1)<safe->audioEndpoints.size())key=safe->audioEndpoints[(size_t)device-1].key;if(safe->inputEngine.running()||safe->permissionPending)safe->stopInputs("Tarjeta del generador modificada; salida en silencio.");safe->outputKey=key;safe->outputChannel=0;safe->persistInputs();safe->syncNames();if(auto* endpoint=findEndpoint(safe->audioEndpoints,key))contentView->setGeneratorOutputs(endpoint->outputs,0);else contentView->setGeneratorOutputs({},0);};
   content->onOutputChannel=[safe](int channel){if(!safe)return;auto* endpoint=findEndpoint(safe->audioEndpoints,safe->outputKey);if(!endpoint||channel<0||channel>endpoint->outputs.size())return;if(safe->inputEngine.running()||safe->permissionPending)safe->stopInputs("Salida del generador modificada; salida en silencio.");safe->outputChannel=channel;safe->persistInputs();safe->syncNames();};
   content->onGeneratorToggle=[safe,isSweep](bool active){if(!safe)return;if(!active){safe->stopInputs(isSweep?"Sweep detenido; salida en silencio.":"Ruido rosa detenido; salida en silencio.");return;}int i=safe->source.getSelectedId()-1;unsigned mask=(isSweep||safe->internalLoopEnabled)&&i>=0&&i<4?1u<<i:0;safe->beginInputs(mask,{},isSweep?LiveInputEngine::Signal::Sweep:LiveInputEngine::Signal::Pink);};
  }
  showAnchored(key,key=="loop"?"Loop Settings":key=="pink"?"Pink / opciones":"Sweep / opciones",std::move(content),anchor);refreshLoopOptions();
 }
 void startPhysicalLoopCalibration(){
  int selected=source.getSelectedId()-1;
  if(selected<0||selected>=4||!pinkLoopActive||internalLoopEnabled){loopCalMessage=String::fromUTF8("Inicia el loop físico con una TF seleccionada");refreshLoopOptions();return;}
  auto k=(size_t)selected;auto key=deviceKeys[k];int reference=refInputs[k];
  if(key.isEmpty()||outputKey!=key||outputChannel<1||reference<1||!inputEngine.startLoopCalibration(key.toStdString())){loopCalMessage=String::fromUTF8("Configura la salida y la referencia de esta TF en la misma tarjeta");refreshLoopOptions();return;}
  loopCalDevice=key;loopCalInput=reference;loopCalOutput=outputChannel;
  loopCapture.reset();loopCalibrating=true;loopCalStartedMs=Time::getMillisecondCounterHiRes();loopCalMessage="Midiendo retorno de Input "+String(reference)+"...";refreshLoopOptions();
 }
 void resetPhysicalLoopCalibration(){
  int selected=source.getSelectedId()-1;
  if(selected<0||selected>=4||deviceKeys[(size_t)selected]!=outputKey||outputChannel<1||refInputs[(size_t)selected]<1){loopCalMessage=String::fromUTF8("Selecciona una ruta de loop para borrar su calibración");refreshLoopOptions();return;}
  inputEngine.cancelLoopCalibration();loopCapture.reset();loopCalibrating=false;
  int reference=refInputs[(size_t)selected];auto key=outputKey.toStdString();
  auto* record=inputEngine.running()?loopStore.find(outputKey,inputEngine.rate(key),inputEngine.bufferSamples(key),outputChannel,reference):loopStore.latestForRoute(outputKey,outputChannel,reference);
  bool hadRecord=record!=nullptr;
  if(record&&!loopStore.clear(*record)){loopCalMessage=String::fromUTF8("No se pudo borrar la calibración de este formato");refreshLoopOptions();return;}
  bool activeRoute=!inputEngine.running()||liveTFIndex==selected;
  if(activeRoute){inputEngine.setReferenceGainDb(0);inputEngine.discardPendingTF();liveTF.reset();}
  loopCalMessage=hadRecord?(activeRoute?String::fromUTF8("Calibración de este formato borrada; REF Gain 0 dB"):String::fromUTF8("Calibración de este formato borrada")):String::fromUTF8("REF Gain restablecido a 0 dB");refreshLoopOptions();
 }
 void pollPhysicalLoopCalibration(){
  if(!loopCalibrating)return;auto key=loopCalDevice.toStdString();
  inputEngine.drainLoopCalibration(key,[this](const TransferFunctionCore::RawBlock& b){if(!loopCapture.ready()&&!loopCapture.failed())loopCapture.feed(b.ref.data(),b.mic.data(),b.count,b.sampleRate);});
  if(inputEngine.loopCalibrationOverflow(key)||loopCapture.failed()||Time::getMillisecondCounterHiRes()-loopCalStartedMs>5000){inputEngine.cancelLoopCalibration();loopCalibrating=false;loopCalMessage=String::fromUTF8("Calibración fallida: retorno inválido o bloques perdidos");refreshLoopOptions();return;}
  if(!loopCapture.ready())return;
  inputEngine.cancelLoopCalibration();loopCalibrating=false;auto result=loopCapture.calculate();
  if(!result||std::abs(result->gainDb)>24){loopCalMessage=String::fromUTF8("Calibración fallida: correlación o nivel insuficiente");refreshLoopOptions();return;}
  LoopCalibrationRecord record;record.deviceKey=loopCalDevice;record.sampleRate=inputEngine.rate(key);record.bufferSamples=inputEngine.bufferSamples(key);record.outputChannel=loopCalOutput;record.inputChannel=loopCalInput;record.result=*result;
  if(!loopStore.set(record)){loopCalMessage=String::fromUTF8("No se pudo guardar la calibración");refreshLoopOptions();return;}
  inputEngine.setReferenceGainDb(result->gainDb);inputEngine.discardPendingTF();liveTF.reset();loopCalMessage=String::fromUTF8("Calibrado: correlación ")+String(result->correlation,2);refreshLoopOptions();
 }
 void refreshLoopOptions(){
  generatorLevel.setText(String(inputEngine.pinkLevel(),0)+" dBFS",dontSendNotification);
  if(outputKey.isEmpty())generatorOutput.setTextWhenNothingSelected("Not assigned");
  if(!anchoredOptions||anchoredKey!="loop")return;auto* view=dynamic_cast<AudioOptionsView*>(anchoredOptions->content());if(!view)return;
  int selected=source.getSelectedId()-1;bool hasTF=selected>=0&&selected<4;String key=hasTF?deviceKeys[(size_t)selected]:String{};int reference=hasTF?refInputs[(size_t)selected]:0;
  String deviceName=key;if(auto* endpoint=findEndpoint(audioEndpoints,key))deviceName=endpoint->name+" ["+endpoint->backend+"]";
  String outputName=outputKey.isNotEmpty()&&outputChannel>0?"Output "+String(outputChannel):"Sin asignar";if(auto* d=findEndpoint(audioEndpoints,outputKey))outputName+=" / "+d->name;String referenceName=reference>0?"Input "+String(reference):"Sin asignar";
  view->setRoute(deviceName,outputName,referenceName);
  StringArray tfChoices,deviceChoices;for(int i=0;i<4;++i)tfChoices.add(micNames[(size_t)i].getText());int selectedDevice=0;for(size_t i=0;i<audioEndpoints.size();++i){const auto& endpoint=audioEndpoints[i];deviceChoices.add(endpoint.name+" ["+endpoint.backend+"]");if(endpoint.key==key)selectedDevice=(int)i+1;}if(auto* endpoint=findEndpoint(audioEndpoints,key))view->setChoices(endpoint->outputs,outputKey==key?outputChannel:0,endpoint->inputs,reference,micInputs[(size_t)selected],tfChoices,hasTF?selected+1:0,deviceChoices,selectedDevice);else view->setChoices({},0,{},0,0,tfChoices,hasTF?selected+1:0,deviceChoices,0);
  auto* routeDevice=findEndpoint(audioEndpoints,key);bool routeReady=hasTF&&routeDevice&&outputKey==key&&outputChannel>0&&outputChannel<=routeDevice->outputs.size()&&reference>0&&reference<=routeDevice->inputs.size()&&micInputs[(size_t)selected]>0&&micInputs[(size_t)selected]<=routeDevice->inputs.size();auto matched=routeReady?loopStore.find(key,inputEngine.rate(key.toStdString()),inputEngine.bufferSamples(key.toStdString()),outputChannel,reference):nullptr;auto record=matched?matched:(routeReady?loopStore.latestForRoute(key,outputChannel,reference):nullptr);bool sameRoute=record!=nullptr;bool valid=matched!=nullptr;
  String message=loopCalibrating?"Midiendo "+String(100*loopCapture.collected()/LoopCalibrationCore::Capture::required)+" %":loopCalMessage.isNotEmpty()?loopCalMessage:valid?"Calibrado; REF Gain "+String(inputEngine.referenceGain(),1)+" dB":record&&inputEngine.running()?String::fromUTF8("Calibración anterior no coincide con el formato"):record?String::fromUTF8("Calibración guardada; se verificará al iniciar"):!routeReady?String::fromUTF8("Configura aquí TF, tarjeta, salida y referencia"):loopStore.size()>0?String::fromUTF8("Sin calibración para esta ruta; otras rutas guardadas"):String::fromUTF8("Sin calibración");
  auto peak=inputEngine.channel(key.toStdString(),reference);
  view->updatePhysicalLoop(pinkLoopActive&&!internalLoopEnabled,routeReady,inputEngine.outputDbfs(key.toStdString()),InputLevels::dbfs(peak),peak.clipped,sameRoute?std::optional<double>{record->result.delayMs}:std::optional<double>{},valid&&pinkLoopActive&&!internalLoopEnabled,message,loopCalibrating,inputEngine.running()?matched!=nullptr:record!=nullptr,inputEngine.pinkLevel(),inputEngine.referenceGain());
 }
 void reflectAvgColour(){avgColourButton.colour=avgColour;avgColourButton.getProperties().set("avgDot",avgColour.toString());avgColourButton.repaint();avgReserve.setColour(Label::textColourId,avgColour);tfContent.repaint();}
 void editAvgColour(){Component::SafePointer<Workstation> safe(this);showAnchored("avg-colour","AVG / color",std::make_unique<ColourEditor>(avgColour,[safe](Colour colour){if(safe){safe->avgColour=colour;safe->reflectAvgColour();safe->persistWorkspace();}}),avgColourButton);}
 void inputMeterOptions(){showDrawer("input-meters","M/R / Pico retenido",std::make_unique<TFMeterSettingsPanel>(inputMeterSettings,[this](TFMeterSettings state){inputMeterSettings=state;reflectInputMeters();persistWorkspace();},[this]{for(auto& meter:inputMeters)meter.resetPeaks();}));}


 int measurementView=0;
 int mode=0; bool polarity=false,disabled=false,adjustmentLinked=false;
 String status=String::fromUTF8("Mya Acústica Analyzer ")+MyaIdentity::version()+String::fromUTF8(" · entradas detenidas");
 static File defaultProjectFile(){
 #if defined(MYA_OFFSCREEN_RENDER)
 return File(String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("MYA-006-A-fixture").getChildFile("proyecto-demo.json");
 #else
 return MyaAppData::projectFile();
 #endif
 }
 File projectFile;
 SplCalibrationStore calibrationStore{projectFile.getParentDirectory().getChildFile("calibraciones-spl.json")};
 LoopCalibrationStore loopStore{projectFile.getParentDirectory().getChildFile("calibracion-loop.json")};
 LoopCalibrationCore::Capture loopCapture;bool loopCalibrating=false;double loopCalStartedMs=0;String loopCalMessage,loopCalDevice;int loopCalInput=0,loopCalOutput=0;
 RmsCalibration::Session calibrationCapture;unsigned calibrationDrops=0;
 MeterState meterState{projectFile.getParentDirectory().getChildFile("medidores.json")};
 MeterState dockMeterState{projectFile.getParentDirectory().getChildFile("medidores-lateral.json"),&meterState};
 MeterState floatingMeterState{projectFile.getParentDirectory().getChildFile("medidores-flotantes.json"),&meterState};
 std::array<MeterState*,3> meterModels(){return {{&meterState,&dockMeterState,&floatingMeterState}};}
 std::unique_ptr<MeterWindow> meters;
 MeterPanel dockMeters{dockMeterState,true};
 PrincipalMeter principalMeter{meterState};
 TextButton tracesPage{"Trazos"},metersPage{"dB Meter"};
 int meterListener=0,dockListener=0;unsigned meterRevision=0,dockRevision=0;
 Component::SafePointer<RoutingPanel> routingStatusPanel;
 LiveInputEngine inputEngine;FirstCallbackRetry firstCallbackRetry;unsigned reportedDrops=0;unsigned activation=0;bool permissionPending=false;bool pinkLoopActive=false,sweepActive=false,internalLoopEnabled=false;InputRoutePreferences activeRoutes;
 TransferFunctionCore::Estimator liveTF;std::array<TransferFunctionCore::Estimator,4> tfEngines;std::array<DelayFinderCore::Finder,4> tfDelayFinders;std::array<bool,4> tfTracking{},tfFinding{};DelayFinderCore::Finder delayFinder;SpectrumCore::Analyzer liveSpectrum;int liveTFIndex=-1,liveTFState=0;bool findingDelay=false,trackingDelay=false;
 unsigned retryMask=0;std::optional<InputLevels::Source> retryExtra;LiveInputEngine::Signal retrySignal=LiveInputEngine::Signal::None;
 InputRoutePreferences routes()const{InputRoutePreferences p;for(size_t k=0;k<4;++k)p.routes[k]={deviceKeys[k].toStdString(),micInputs[k],refInputs[k]};p.outputDevice=outputKey;p.outputChannel=outputChannel;p.outputStored=true;p.pinkDb=inputEngine.pinkLevel();p.pinkHighPass=inputEngine.pinkHighPass();p.pinkLowPass=inputEngine.pinkLowPass();p.sweepDb=inputEngine.sweepLevel();p.sweepStart=inputEngine.sweepStart();p.sweepEnd=inputEngine.sweepEnd();p.sweepSeconds=inputEngine.sweepSeconds();return p;}
 void refreshRoutingStatus(){if(routingStatusPanel)routingStatusPanel->setAcquisitionState(inputEngine.running(),permissionPending,inputEngine.details());}
 void persistInputs(){if(!routes().save(projectFile.getParentDirectory().getChildFile("entradas-niveles.json")))status="No se pudieron guardar las rutas de entrada";}
 void clearInputLevels(){for(auto& m:inputMeters){for(auto& c:m.channels)c.reset();m.repaint();}for(auto* model:meterModels())model->clearReadings();}
 void stopInputs(String message="Entradas detenidas"){calibrationCapture.fail("Entrada detenida durante calibracion");calibrationStore.clearAcknowledgements();loopCalibrating=false;loopCapture.reset();inputEngine.cancelLoopCalibration();++activation;permissionPending=false;firstCallbackRetry.cancel();stopTimer();inputEngine.stop();liveTF.reset();for(auto& engine:tfEngines)engine.reset();for(auto& finder:tfDelayFinders)finder.reset();tfTracking.fill(false);tfFinding.fill(false);liveSpectrum.reset();delayFinder.reset();findingDelay=trackingDelay=false;liveTFIndex=-1;liveTFState=0;pinkLoopActive=false;sweepActive=false;reportedDrops=0;clearInputLevels();refreshRoutingStatus();refreshLoopOptions();status=message;commands.refresh();repaint();}
 void finishSweep(){++activation;permissionPending=false;firstCallbackRetry.cancel();stopTimer();inputEngine.stop();sweepActive=pinkLoopActive=false;clearInputLevels();refreshRoutingStatus();refreshLoopOptions();bool ready=liveTFIndex>=0&&liveTFIndex<4&&tfEngines[(size_t)liveTFIndex].ready();status=ready?"Sweep finalizado / respuesta e IR reales disponibles para capturar":"Sweep finalizado sin respuesta valida; comprueba ruta, nivel y micro";commands.refresh();repaint();}
 void beginInputs(unsigned mask,std::optional<InputLevels::Source> extra={},LiveInputEngine::Signal signal=LiveInputEngine::Signal::None,bool allowFirstCallbackRetry=true){
 #if defined(MYA_OFFSCREEN_RENDER)
  juce::ignoreUnused(mask,extra,signal,allowFirstCallbackRetry);status="Renderer: apertura de audio bloqueada";return;
 #else
  stopInputs();retryMask=mask;retryExtra=extra;retrySignal=signal;
  audioEndpoints=discoverAudioEndpoints();refreshMeterSources();auto chosen=routes();bool virtualRef=internalLoopEnabled||signal==LiveInputEngine::Signal::Sweep;
  int selectedTF=source.getSelectedId()-1;
  std::vector<InputLevels::Source> observers;
  if(mask==15)for(auto* model:meterModels())for(const auto& c:model->state().cards)if(c.deviceKey.isNotEmpty()&&c.channel>0)observers.push_back({c.deviceKey.toStdString(),c.channel});
  if(extra)observers.push_back(*extra);
  auto plan=InputLevels::plan(chosen.routes,mask,observers,LiveInputEngine::catalog(audioEndpoints),virtualRef);
  String loopDevice;String configuredOutputKey=outputKey;int configuredOutput=outputChannel;
  if(signal!=LiveInputEngine::Signal::None){
   auto* endpoint=findEndpoint(audioEndpoints,configuredOutputKey);
   if(!endpoint||configuredOutput<1||configuredOutput>endpoint->outputs.size()){status=String::fromUTF8("Configura una tarjeta y una salida válidas en Pink/Sweep o Audio / E-S");signalOptions(signal==LiveInputEngine::Signal::Sweep?"sweep":"pink");repaint();return;}
   bool tfReady=selectedTF>=0&&selectedTF<4&&plan.valid()&&plan.groups.size()==1&&String::fromUTF8(plan.groups[0].key.c_str())==configuredOutputKey;
   if(tfReady)loopDevice=configuredOutputKey;else{plan={};plan.groups.push_back({configuredOutputKey.toStdString(),{}});selectedTF=-1;virtualRef=false;}
  }
  else if(!plan.valid()){status=String::fromUTF8(plan.error.c_str());repaint();return;}
  if(selectedTF<0||selectedTF>=4||(mask&(1u<<selectedTF))==0)selectedTF=-1;
  std::vector<LiveInputEngine::TransferRoute> transferRoutes;for(int i=0;i<4;++i)if(mask&(1u<<i)){const auto& route=chosen.routes[(size_t)i];bool virtualRoute=(internalLoopEnabled||signal==LiveInputEngine::Signal::Sweep)&&String::fromUTF8(route.device.c_str())==configuredOutputKey;if(!route.device.empty()&&route.mic>0&&(route.ref>0||virtualRoute))transferRoutes.push_back({i,route.device,route.mic,route.ref,virtualRoute});}
  unsigned request=++activation;permissionPending=true;refreshRoutingStatus();Component::SafePointer<Workstation> safe(this);
  auto open=[safe,request,plan,chosen,signal,loopDevice,configuredOutputKey,configuredOutput,allowFirstCallbackRetry,selectedTF,virtualRef,transferRoutes](bool granted){
   if(!safe||safe->activation!=request)return;safe->permissionPending=false;
   if(!granted){safe->status="Permiso de microfono denegado. Entradas cerradas.";safe->refreshRoutingStatus();safe->refreshLoopOptions();safe->repaint();return;}
   const auto& route=chosen.routes[(size_t)std::max(0,selectedTF)];juce::String tfKey=selectedTF>=0?juce::String::fromUTF8(route.device.c_str()):juce::String{};
   if(safe->inputEngine.start(plan,safe->audioEndpoints,signal,configuredOutputKey,tfKey,selectedTF>=0?route.mic:0,selectedTF>=0&&!virtualRef?route.ref:0,virtualRef,configuredOutput,transferRoutes)){
    safe->activeRoutes=chosen;safe->liveTFIndex=selectedTF;safe->pinkLoopActive=signal==LiveInputEngine::Signal::Pink;safe->sweepActive=signal==LiveInputEngine::Signal::Sweep;safe->loopCalMessage.clear();safe->inputEngine.setReferenceGainDb(0);if(selectedTF>=0){const int depths[]={1,2,4,5,8,16,32,64};safe->liveTF.setAveraging(signal==LiveInputEngine::Signal::Sweep?1:depths[jlimit(1,8,safe->averageChoice.getSelectedId())-1],true);safe->liveTF.setDelayMs(safe->micDelay[(size_t)selectedTF].getValue());}
    if(!virtualRef&&selectedTF>=0&&configuredOutputKey==tfKey){if(auto stored=safe->loopStore.find(tfKey,safe->inputEngine.rate(tfKey.toStdString()),safe->inputEngine.bufferSamples(tfKey.toStdString()),configuredOutput,route.ref)){safe->inputEngine.setReferenceGainDb(stored->result.gainDb);safe->inputEngine.discardPendingTF();}}
    safe->firstCallbackRetry.arm(allowFirstCallbackRetry);
    auto generated=signal==LiveInputEngine::Signal::Pink?(virtualRef?"Loop interno rosa ":"Ruido rosa ")+juce::String(safe->inputEngine.pinkLevel(),1)+" dBFS activo / ":signal==LiveInputEngine::Signal::Sweep?"Sweep "+juce::String(safe->inputEngine.sweepLevel(),1)+" dBFS activo / ":"Entradas dBFS activas / ";safe->status=generated+safe->inputEngine.summary()+(selectedTF>=0?" TF reales activas: "+String((int)transferRoutes.size()):" Selecciona una TF configurada para medir.");safe->commands.refresh();safe->startTimerHz(30);
   }else{safe->status=safe->inputEngine.lastError();safe->writeInputDiagnostic();}
   safe->refreshRoutingStatus();safe->refreshLoopOptions();safe->repaint();
  };
  if(RuntimePermissions::isRequired(RuntimePermissions::recordAudio)&&!RuntimePermissions::isGranted(RuntimePermissions::recordAudio))RuntimePermissions::request(RuntimePermissions::recordAudio,std::move(open));else open(true);
 #endif
 }
 void timerCallback()override{
  if(!pollCalibration()){auto failure=inputEngine.lastError();bool retry=firstCallbackRetry.consume(failure);auto mask=retryMask;auto extra=retryExtra;auto signal=retrySignal;writeInputDiagnostic();stopInputs(retry?"Sin primer callback; reintentando una vez...":failure);if(retry){unsigned expected=activation;Component::SafePointer<Workstation> safe(this);MessageManager::callAsync([safe,expected,mask,extra,signal]{if(safe&&safe->activation==expected)safe->beginInputs(mask,extra,signal,false);});}return;}
  if(sweepActive&&inputEngine.sweepComplete()){finishSweep();return;}
  if((findingDelay||trackingDelay)&&delayFinder.ready()){
   auto found=delayFinder.calculate();if(found.valid&&liveTFIndex>=0){auto k=(size_t)liveTFIndex;micDelay[k].setValue(found.milliseconds,dontSendNotification);selectedDelay.setValue(found.milliseconds,dontSendNotification);liveTF.setDelayMs(found.milliseconds);status="TF "+String(liveTFIndex+1)+" / delay "+String(found.milliseconds,2)+" ms / confianza "+String(found.confidence*100.,0)+" %";}else status="Find: correlacion insuficiente; comprueba referencia y nivel";findingDelay=false;delayFinder.reset();if(trackingDelay)delayFinder.configure(.5);repaint();
  }
  for(size_t k=0;k<tfTracking.size();++k)if(tfDelayFinders[k].ready()){
   auto found=tfDelayFinders[k].calculate();
   if(found.valid){micDelay[k].setValue(found.milliseconds,dontSendNotification);tfEngines[k].setDelayMs(found.milliseconds);if((int)k==source.getSelectedId()-1)selectedDelay.setValue(found.milliseconds,dontSendNotification);status="TF "+String((int)k+1)+" / delay "+String(found.milliseconds,2)+" ms / confianza "+String(found.confidence*100.,0)+" %";}
   else if(tfFinding[k])status="Find TF "+String((int)k+1)+": correlacion insuficiente";
   tfFinding[k]=false;bool keep=tfTracking[k];tfDelayFinders[k].reset();if(keep)tfDelayFinders[k].configure(.5);
  }
  pollPhysicalLoopCalibration();double now=Time::getMillisecondCounterHiRes()*.001;for(size_t k=0;k<4;++k){auto key=deviceKeys[k].toStdString();auto refDb=internalLoopEnabled?(pinkLoopActive?inputEngine.outputDbfs(key):std::optional<double>{}):InputLevels::dbfs(inputEngine.channel(key,refInputs[k]));inputMeters[k].channels[0].update(InputLevels::dbfs(inputEngine.channel(key,micInputs[k])),now,inputMeterSettings);inputMeters[k].channels[1].update(refDb,now,inputMeterSettings);inputMeters[k].repaint();}refreshLoopOptions();if(liveTFIndex>=0){auto key=deviceKeys[(size_t)liveTFIndex].toStdString();auto refDb=internalLoopEnabled?(pinkLoopActive?inputEngine.outputDbfs(key):std::optional<double>{}):InputLevels::dbfs(inputEngine.channel(key,refInputs[(size_t)liveTFIndex]));int state=tfEngines[(size_t)liveTFIndex].ready()?1:(refDb&&*refDb< -60.?2:0);if(state!=liveTFState){liveTFState=state;status=state==1?"TF "+String(liveTFIndex+1)+" M/R en directo; comprueba coherencia antes de ajustar.":state==2?String::fromUTF8("Referencia inferior a -60 dBFS: TF oculta hasta recibir señal suficiente."):"TF "+String(liveTFIndex+1)+" promediando M/R...";}repaint();}
  for(auto* model:meterModels()){for(const auto& c:model->state().cards){auto peak=inputEngine.channel(c.deviceKey.toStdString(),c.channel);if(c.metric==6){auto db=inputEngine.rms(c.deviceKey.toStdString(),c.channel);bool clip=inputEngine.rmsClip(c.deviceKey.toStdString(),c.channel);model->publishRmsSpl(c.id,c.deviceKey,c.channel,calibrationStore.spl(c.deviceKey,c.channel,db,clip),db.has_value(),calibrationStore.isAcknowledged(c.deviceKey,c.channel),clip);}else if(c.metric>=2&&c.metric<=5)model->validateWeighted(c.id,peak.valid,calibrationStore.isAcknowledged(c.deviceKey,c.channel),peak.clipped);else model->publishFs(c.id,c.deviceKey,c.channel,InputLevels::dbfs(peak),peak.clipped);}model->readingsChanged();}if(reportedDrops!=inputEngine.drops()){reportedDrops=inputEngine.drops();status="Entradas dBFS / bloques perdidos: "+String(reportedDrops)+". Las ventanas incompletas muestran --.";repaint(0,getHeight()-30,getWidth(),30);}
 }
 void writeInputDiagnostic(){FileOutputStream out(projectFile.getParentDirectory().getChildFile("diagnostico-entrada-ultimo.txt"));if(out.openedOk()){out.writeText(Time::getCurrentTime().toISO8601(true)+" / "+MyaIdentity::version()+"\n"+inputEngine.lastError()+"\n"+inputEngine.details()+"\n\n",false,false,"\n");out.flush();}}
 bool pollCalibration(){bool ok=inputEngine.poll([this](const std::string& key,const InputLevels::Frame& frame,double rate){calibrationCapture.feed(key,frame,rate);auto device=String::fromUTF8(key.c_str());for(auto* model:meterModels())for(const auto& card:model->state().cards)if(card.metric>=2&&card.metric<=5&&card.deviceKey==device&&card.channel>0&&card.channel<=InputLevels::maxChannels){auto* record=calibrationStore.find(device,card.channel);std::optional<double> offset;if(record&&calibrationStore.isAcknowledged(device,card.channel))offset=record->offset;model->feedWeighted(card.id,device,card.channel,frame.channels[(size_t)card.channel-1],frame.sequence,rate,offset);}},[this](const std::string&,const TransferFunctionCore::RawBlock& block){liveTF.feed(block);liveSpectrum.feed(block.mic.data(),block.count,block.sampleRate);if(findingDelay||trackingDelay)delayFinder.feed(block);});const int depths[]={1,2,4,5,8,16,32,64};inputEngine.pollRoutes([this,&depths](int id,const std::string&,const TransferFunctionCore::RawBlock& block){if(id<0||id>=4)return;auto& engine=tfEngines[(size_t)id];engine.setAveraging(sweepActive?1:depths[jlimit(1,8,averageChoice.getSelectedId())-1],true);engine.setDelayMs(micDelay[(size_t)id].getValue());engine.feed(block);if(tfTracking[(size_t)id]||tfFinding[(size_t)id])tfDelayFinders[(size_t)id].feed(block);});auto drops=inputEngine.drops();if(drops!=calibrationDrops)calibrationCapture.fail("Bloques perdidos durante calibracion");calibrationDrops=drops;return ok;}
 CalibrationInputSnapshot calibrationSnapshot(const String& key,int ch){auto k=key.toStdString();return {inputEngine.running(),inputEngine.channel(k,ch).valid,inputEngine.rmsClip(k,ch),inputEngine.rate(k),inputEngine.rms(k,ch),inputEngine.cursor(k),inputEngine.generation(),inputEngine.drops()};}
 void invalidateCalibration(const String& key,int ch){for(auto* model:meterModels())model->invalidateRms(key,ch);}
 void closeCalibration(){if(drawerKey=="spl-calibration"&&(calibrationCapture.state==RmsCalibration::Session::State::Measuring||calibrationCapture.state==RmsCalibration::Session::State::Ready))calibrationCapture.cancel();}
 void showCalibration(String key={},int ch=0){
  if(key.isEmpty())if(auto* c=meterState.primaryCard()){key=c->deviceKey;ch=c->channel;}
  Component::SafePointer<Workstation> safe(this);
  auto panel=std::make_unique<SplCalibrationPanel>(calibrationStore,calibrationCapture,audioEndpoints,key,ch,
   [safe](const String& k,int c){return safe?safe->calibrationSnapshot(k,c):CalibrationInputSnapshot{};},
   [safe](String k,int c){if(safe)safe->beginInputs(0,InputLevels::Source{k.toStdString(),c});},
   [safe](const SplCalibrationStore::Record& record){if(!safe)return false;auto generation=safe->inputEngine.generation();if(!safe->pollCalibration()||generation!=safe->inputEngine.generation()||!safe->inputEngine.channel(record.deviceKey.toStdString(),record.channel).valid||safe->inputEngine.rmsClip(record.deviceKey.toStdString(),record.channel)){safe->calibrationCapture.fail("Entrada no valida al confirmar");return false;}if(!safe->inputEngine.caughtUp(record.deviceKey.toStdString())){safe->status="Datos pendientes; vuelve a confirmar";return false;}auto finalRecord=record;finalRecord.range=safe->calibrationCapture.maximum-safe->calibrationCapture.minimum;if(!safe->calibrationStore.confirm(finalRecord,safe->calibrationCapture))return false;safe->calibrationCapture.confirmed();safe->invalidateCalibration(record.deviceKey,record.channel);return true;},
   [safe](String k,int c){if(safe)safe->invalidateCalibration(k,c);});
  panel->onCancel=[safe]{if(safe)safe->commands.invoke(602);};showDrawer("spl-calibration","Calibracion SPL RMS",std::move(panel));
 }
 void deferredCalibration(String key={},int ch=0){Component::SafePointer<Workstation> safe(this);MessageManager::callAsync([safe,key,ch]{if(safe)safe->showCalibration(key,ch);});}
 void inputRoutesChanged(){if(inputEngine.running()||permissionPending)stopInputs("Ruta modificada. Pulsa Play para abrir las entradas seleccionadas.");inputEngine.setReferenceGainDb(0);loopCalMessage.clear();refreshLoopOptions();persistInputs();}

 void refreshMeterSources(){std::vector<MeterState::Source> sources;for(const auto& device:audioEndpoints)for(int i=0;i<device.inputs.size();++i){String label=device.name+" / "+device.inputs[i];for(size_t k=0;k<4;++k)if(deviceKeys[k]==device.key&&micInputs[k]==i+1)label+=" / "+micNames[k].getText();sources.push_back({device.key,label,i+1});}for(auto* model:meterModels())model->setSources(sources);}
 void principalMeterOptions(){showDrawer("principal-meter","SPL / configuracion",std::make_unique<MeterConfiguration>(meterState,[this]{commands.invoke(203);},[this]{deferredCalibration();}));}
 void showMeters(){if(meters&&meters->isVisible()){meters->closeButtonPressed();return;}refreshMeterSources();if(!meters){meters=std::make_unique<MeterWindow>(floatingMeterState);meters->setLookAndFeel(&look);meters->panel->calibrateRequested=[this](String k,int c){deferredCalibration(k,c);};meters->panel->toggleRequested=[this]{commands.invoke(203);};}meters->setVisible(true);meters->toFront(true);}
 Rectangle<int> plotArea(Rectangle<int> r) const{auto p=CompactLayout::plot(r.getWidth(),r.getHeight());return {r.getX()+p.x,r.getY()+p.y,p.w,p.h};}
 Rectangle<int> activeGraphBounds(int type) const{if(mode==0)return plotBounds(type);Rectangle<int> center{centerX()+8,118,centerWidth()-16,getHeight()-183};int usable=center.getHeight()-100,irHeight=plotChoice.getSelectedId()==2?0:104,gh=(usable-irHeight)/2;if(type==0)return {center.getX(),center.getY(),center.getWidth(),90};if(type==1)return {center.getX(),center.getY()+(irHeight?97:0),center.getWidth(),gh};return {center.getX(),center.getY()+irHeight+gh,center.getWidth(),gh};}
 File tfSettingsFile() const{return projectFile.getParentDirectory().getChildFile("tf-graficos.json");}
 void persistTF(){tfSettings.linkPhaseMag=linkSmooth.getToggleState();tfSettings.showIR=plotChoice.getSelectedId()==1;tfSettings.coherenceOn=showCoherence.getToggleState();tfSettings.coherenceThreshold=coherenceThreshold.getValue();if(!tfSettings.save(tfSettingsFile()))status="No se pudieron guardar ajustes TF";}
 void reflectTF(){linkSmooth.setToggleState(tfSettings.linkPhaseMag,dontSendNotification);showCoherence.setToggleState(tfSettings.coherenceOn,dontSendNotification);coherenceThreshold.setValue(tfSettings.coherenceThreshold,dontSendNotification);plotChoice.setSelectedId(tfSettings.showIR?1:2,dontSendNotification);for(auto& combo:plotSmooth){combo.setItemEnabled(1,tfSettings.allowNone);if(!tfSettings.allowNone&&combo.getSelectedId()==1)combo.setSelectedId(2,dontSendNotification);}if(tfSettings.linkPhaseMag)plotSmooth[1].setSelectedId(plotSmooth[0].getSelectedId(),dontSendNotification);}
 void applyTF(TFDisplayState state){if(!state.valid()){status="Rangos o pasos TF no validos";if(graphWindow)graphWindow->repaint();repaint();return;}bool changedLinear=(tfSettings.frequencyGrid==4)!=(state.frequencyGrid==4);bool enableZoom=!tfSettings.linkFrequencyZoom&&state.linkFrequencyZoom;tfSettings=state;reflectTF();if(enableZoom)PlotPresentation::linkFrequency(plotZoomX,plotPanX,activePlot>0?activePlot:1);if(changedLinear){plotPanX[1]=plotPanX[2]=0;plotZoomX[1]=plotZoomX[2]=1;}persistTF();persistGraphChoices();resized();if(graphWindow)graphWindow->repaint();repaint();menuItemsChanged();}
 void tfGraphSettings(int tab=1,bool toggle=true){if(toggle&&graphWindow&&graphWindow->isVisible()&&graphWindow->selectedTab()==tab){graphWindow->setVisible(false);return;}auto oldBounds=graphWindow?graphWindow->getBounds():Rectangle<int>{};bool en=commands.language==MyaCommands::Language::EN;if(graphWindow&&en!=graphWindowEnglish)graphWindow.reset();graphWindowEnglish=en;if(!graphWindow)graphWindow=std::make_unique<GraphSettingsWindow>(commands.language==MyaCommands::Language::EN,[this]{return tfSettings;},[this](TFDisplayState state){applyTF(state);},[this](int id){commands.invoke(id);},[this]{return irScale;});if(!oldBounds.isEmpty())graphWindow->setBounds(oldBounds);graphWindow->setLookAndFeel(&look);graphWindow->toggleMeters=[this]{commands.invoke(203);};graphWindow->select(tab);graphWindow->setVisible(true);graphWindow->toFront(true);}

 std::optional<double> panelCoherence(double u,int panel) const{auto saved=capturedFront(panel);if(saved.isNotEmpty())return savedTraces.coherence(saved,tfSettings.frequency(u));int index=frontTrace(panel);if(index<0||index>=4||u<0||u>1)return std::nullopt;auto p=tfEngines[(size_t)index].at(tfSettings.frequency(u));return p?std::optional<double>(p->coherence):std::nullopt;}
 std::optional<double> savedValue(const String& id,double u,int panel){auto c=savedTraces.coherence(id,tfSettings.frequency(u));if(!tfSettings.passes(c))return std::nullopt;int kind=displayType(panel);auto v=savedTraces.value(id,tfSettings.frequency(u),kind,plotSmooth[(size_t)(panel-1)].getSelectedId(),0);return v?std::optional<double>(tfSettings.normalize(*v*(kind==1?360.:36.),kind)):std::nullopt;}

 CaptureLibrary captureLibrary{projectFile.getParentDirectory().getChildFile("Biblioteca")};
 CaptureLibraryPanel libraryPanel{captureLibrary};
 CapturedTraceDisplay savedTraces;
 std::array<String,3> frontCaptured;std::vector<String> capturedChoices;
 TextButton restoreLibraryButton{">"};
 var libraryDSPBindings{Array<var>{}};
 void refreshLibrary(){
  savedTraces.reload(captureLibrary);
  for(auto& id:frontCaptured){auto* n=savedTraces.find(id);if(!n||!n->visible)id.clear();}
  refreshPlotSources();
  repaint();menuItemsChanged();
 }
 void refreshPlotSources(){auto previous=capturedChoices;std::array<int,2> selected{{plotMic[1].getSelectedId(),plotMic[2].getSelectedId()}};capturedChoices.clear();for(const auto& n:captureLibrary.state().nodes)if(n.kind==CaptureLibrary::Kind::SnapshotTF)capturedChoices.push_back(n.id);for(int panel:{1,2}){auto& menu=plotMic[(size_t)panel];int choice=selected[(size_t)panel-1];String preferred;if(choice>=1000&&(size_t)(choice-1000)<previous.size())preferred=previous[(size_t)choice-1000];menu.clear(dontSendNotification);menu.addItem("TF + capturas visibles",1);for(int i=0;i<4;++i)menu.addItem(micNames[(size_t)i].getText(),i+2);menu.addItem("Sin trazos",6);menu.addItem("Solo TF visibles",7);for(size_t i=0;i<capturedChoices.size();++i){auto* n=captureLibrary.find(capturedChoices[i]);menu.addItem("Captura: "+n->name+(n->method=="synthetic-demo"?" [DEMO]":" [archivo]")+(!n->visible?" [oculta]":""),1000+(int)i);if(n->id==preferred)choice=1000+(int)i;}if(preferred.isNotEmpty()&&std::find(capturedChoices.begin(),capturedChoices.end(),preferred)==capturedChoices.end())choice=6;menu.setSelectedId(menu.indexOfItemId(choice)>=0?choice:1,dontSendNotification);}}
 bool capturedInPanel(const String& id,int panel)const{if(panel==0)return false;auto* n=savedTraces.find(id);if(!n||!n->visible)return false;int choice=plotMic[(size_t)panel].getSelectedId();return choice==1||(choice>=1000&&(size_t)(choice-1000)<capturedChoices.size()&&capturedChoices[(size_t)choice-1000]==id);}
 bool tfInPanel(int index,int panel)const{int choice=plotMic[(size_t)panel].getSelectedId();return traceSwitch[(size_t)index].getToggleState()&&(choice==1||choice==7||choice==index+2);}
 String capturedFront(int panel) const{
  if(panel==0)return {};int choice=plotMic[(size_t)panel].getSelectedId();if(choice>=1000&&(size_t)(choice-1000)<capturedChoices.size()){auto id=capturedChoices[(size_t)choice-1000];return capturedInPanel(id,panel)?id:String{};}if(choice!=1)return {};auto preferred=frontCaptured[(size_t)panel];if(capturedInPanel(preferred,panel))return preferred;
  if(frontTrace(panel)<0){auto ids=savedTraces.visible();if(!ids.empty())return ids.back();}return {};
 }
 String captureLegend(int panel)const{StringArray names;for(const auto& id:savedTraces.visible())if(capturedInPanel(id,panel)){auto* n=savedTraces.find(id);names.add(n->name+(n->method=="synthetic-demo"?" [DEMO]":" [archivo]"));}return names.isEmpty()?String{}:"Captura: "+names.joinIntoString(" / ");}
 std::optional<double> panelValue(double u,int type){
  auto id=capturedFront(type);
  if(type>0&&id.isNotEmpty())return savedValue(id,u,type);
  return plotValue(u,frontTrace(type),type);
 }
 String panelName(int type) const{
  if(auto* n=savedTraces.find(capturedFront(type)))return "Captura: "+n->name+" / "+n->method;
  int index=frontTrace(type);return index>=0&&index<4&&tfEngines[(size_t)index].ready()?micNames[(size_t)index].getText()+" / M/R en directo":String("sin trazo real");
 }
 void drawSaved(Graphics& g,Rectangle<int> p,int type,bool front,double panX,double panY,double zoomX,double zoomY){
  if(type==0)return;
  for(const auto& id:savedTraces.visible()){
   if(!capturedInPanel(id,type)||(id==capturedFront(type))!=front)continue;Path path;bool gap=true;double previous=0;
   int points=jmax(700,p.getWidth()*2);
   for(int i=0;i<=points;++i){double u=PlotPresentation::coordinate(i/(double)points,panX,zoomX);
    auto value=savedValue(id,u,type);
    if(!value){gap=true;continue;}float x=(float)(p.getX()+i*(double)p.getWidth()/points),y=(float)(p.getCentreY()-(*value-panY)*zoomY*p.getHeight());
    if(gap||(displayType(type)==1&&std::abs(*value-previous)>.5))path.startNewSubPath(x,y);else path.lineTo(x,y);gap=false;previous=*value;
   }
   g.setColour(Colour::fromString(savedTraces.find(id)->colour));g.strokePath(path,PathStrokeType((float)(front?tfSettings.foregroundWidth:tfSettings.backgroundWidth)));
  }
 }

 File graphViewFile() const{return projectFile.getParentDirectory().getChildFile("vista-graficos.json");}
 void restoreGraphChoices(const GraphViewState& state){if(state.linkSpecified)linkSmooth.setToggleState(state.linkPhaseMag,dontSendNotification);plotTypes=state.types;irScale=state.irScale;impulseScale.setSelectedId(irScale+1,dontSendNotification);for(size_t i=0;i<2;++i)plotTypeChoice[i].setSelectedId(plotTypes[i],dontSendNotification);}
 void persistGraphChoices(){if(!GraphViewState{plotTypes,irScale,linkSmooth.getToggleState()}.save(graphViewFile()))status="No se pudo guardar la vista de graficos";}
 TextButton* add(String name,std::function<void()> action){auto p=std::make_unique<TextButton>(name);p->onClick=std::move(action);addAndMakeVisible(*p);auto* raw=p.get();buttons.push_back(std::move(p));return raw;}

 void syncNames(){int selectedDevice=source.getSelectedId()-1;for(auto* model:meterModels())model->setDefaultDevice(selectedDevice>=0&&selectedDevice<4?deviceKeys[(size_t)selectedDevice]:String{});reflectInputMeters();for(auto& menu:plotMic)for(int i=0;i<4;++i)menu.changeItemText(i+2,micNames[(size_t)i].getText());int selected=source.getSelectedId();source.clear(dontSendNotification);for(int i=0;i<4;++i){auto k=(size_t)i;auto* d=findEndpoint(audioEndpoints,deviceKeys[k]);String device=d?d->name:(deviceKeys[k].isEmpty()?"Not assigned":deviceKeys[k]+" [Unavailable]");String meas=d&&micInputs[k]>0&&micInputs[k]<=d->inputs.size()?d->inputs[micInputs[k]-1]:(micInputs[k]>0?"Input "+String(micInputs[k])+" [Unavailable]":String("Not assigned"));String ref=d&&refInputs[k]>0&&refInputs[k]<=d->inputs.size()?d->inputs[refInputs[k]-1]:(refInputs[k]>0?"Input "+String(refInputs[k])+" [Unavailable]":String("Not assigned"));String route=device+" | M: "+meas+" / R: "+ref;source.addItem(micNames[k].getText()+" | "+route,i+1);micNames[k].setColour(Label::textColourId,traceSwitch[k].colour);micNames[k].setTooltip(route);}
 source.addItem("AVG",5);source.setSelectedId(selected>0?selected:1,dontSendNotification);
 generatorOutput.clear(dontSendNotification);auto* output=findEndpoint(audioEndpoints,outputKey);if(output){generatorOutput.addItemList(output->outputs,1);generatorOutput.setSelectedId(outputChannel,dontSendNotification);}generatorOutput.setTextWhenNothingSelected("Not assigned");
 meterInput.clear(dontSendNotification);int k=source.getSelectedId()-1;if(k>=0&&k<4){auto* d=findEndpoint(audioEndpoints,deviceKeys[(size_t)k]);if(d){meterInput.addItemList(d->inputs,1);meterInput.setSelectedId(micInputs[(size_t)k],dontSendNotification);}}meterInput.setTextWhenNothingSelected("Not assigned");refreshMeterSources();repaint();}


 void positionDrawer(){if(settingsDrawer){int edge=rightVisible?getWidth()-rightWidth-16:getWidth()-8;int width=jmin(718,edge-16);settingsDrawer->setBounds(edge-width,82,width,getHeight()-115);settingsDrawer->toFront(false);}}
 void showDrawer(String key,String title,std::unique_ptr<Component> body){closeCalibration();if(anchoredOptions)anchoredOptions->setVisible(false);if(settingsDrawer&&settingsDrawer->isVisible()&&drawerKey==key){settingsDrawer->setVisible(false);return;}drawerKey=key;settingsDrawer=std::make_unique<SettingsDrawer>(title,std::move(body));settingsDrawer->requestClose=[this]{commands.invoke(602);};addAndMakeVisible(*settingsDrawer);positionDrawer();}
 void engineSettings(int i){auto k=(size_t)i;auto* endpoint=findEndpoint(audioEndpoints,deviceKeys[k]);Component::SafePointer<Workstation> safe(this);auto panel=std::make_unique<EnginePanel>(micNames[k].getText(),micDelay[k].getValue(),micInputs[k],refInputs[k],traceSwitch[k].colour,endpoint?endpoint->name:String("Not assigned / unavailable"),endpoint?endpoint->inputs:StringArray(),[safe,i](String n,double d,int mic,int ref,Colour c){if(safe){auto j=(size_t)i;auto name=n.trim();safe->micNames[j].setText(name.isEmpty()?"TF "+String(i+1):name,dontSendNotification);safe->micDelay[j].setValue(d,dontSendNotification);bool changed=safe->micInputs[j]!=mic||safe->refInputs[j]!=ref;safe->micInputs[j]=mic;safe->refInputs[j]=ref;if(changed)safe->inputRoutesChanged();safe->traceSwitch[j].colour=c;safe->traceSwitch[j].repaint();if(safe->settingsDrawer)safe->settingsDrawer->setAccent(c);safe->tfContent.repaint();safe->syncNames();}});showDrawer("TF "+String(i+1),"Opciones / "+micNames[k].getText(),std::move(panel));if(settingsDrawer)settingsDrawer->setAccent(traceSwitch[k].colour);}
 void audioSettings(){audioEndpoints=discoverAudioEndpoints();syncNames();std::array<String,4> names;for(size_t i=0;i<4;++i)names[i]=micNames[i].getText();Component::SafePointer<Workstation> safe(this);
 auto panel=std::make_unique<RoutingPanel>(audioEndpoints,names,deviceKeys,micInputs,refInputs,outputKey,outputChannel,[safe](int i){return safe?safe->traceSwitch[(size_t)i].colour:Palette::muted;},[safe](int i,String name,String key,int mic,int ref){if(safe){auto k=(size_t)i;safe->micNames[k].setText(name.trim().isEmpty()?"TF "+String(i+1):name,dontSendNotification);bool changed=safe->deviceKeys[k]!=key||safe->micInputs[k]!=mic||safe->refInputs[k]!=ref;safe->deviceKeys[k]=key;safe->micInputs[k]=mic;safe->refInputs[k]=ref;if(changed)safe->inputRoutesChanged();safe->syncNames();}},[safe](String key,int channel){if(safe){bool changed=safe->outputKey!=key||safe->outputChannel!=channel;if(changed&&(safe->inputEngine.running()||safe->permissionPending))safe->stopInputs("Salida modificada; audio detenido.");safe->outputKey=key;safe->outputChannel=channel;if(changed){safe->inputEngine.setReferenceGainDb(0);safe->loopCalMessage.clear();safe->persistInputs();}safe->syncNames();safe->refreshLoopOptions();}});
 panel->calibrateRequested=[this]{int i=source.getSelectedId()-1;if(i>=0&&i<4)deferredCalibration(deviceKeys[(size_t)i],micInputs[(size_t)i]);else deferredCalibration();};routingStatusPanel=panel.get();refreshRoutingStatus();showDrawer("audio","Measurement Configuration",std::move(panel));}

 void info(String title,String body){AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,title,body,"Cerrar");}
 void showUpdateResult(MyaUpdate::Result result,bool manual){
  if(result.state==MyaUpdate::State::NewVersion){
   auto* box=new AlertWindow("Nueva versión de Mya Acústica Analyzer",
    "Disponible "+result.version+"; instalada "+MyaIdentity::version()+". La instalación es manual.",
    MessageBoxIconType::InfoIcon);
   box->addButton("Abrir descarga",1);box->addButton("Más tarde",0);
   box->enterModalState(true,ModalCallbackFunction::create([url=result.url](int choice){if(choice==1)URL(url).launchInDefaultBrowser();}),true);
  }else if(manual){
   String message;
   switch(result.state){
    case MyaUpdate::State::Current:message=MyaUpdate::currentMessage(MyaIdentity::version());break;
    case MyaUpdate::State::NoRelease:message="Todavía no hay una versión publicada en GitHub.";break;
    case MyaUpdate::State::NetworkError:message="No se pudo consultar GitHub. Comprueba la conexión e inténtalo de nuevo.";break;
    case MyaUpdate::State::InvalidResponse:message="La respuesta de GitHub no contiene una versión válida.";break;
    case MyaUpdate::State::NewVersion:break;
   }
   info("Buscar actualizaciones",message);
  }
 }
 void setup(Slider& s,Label& l,String name,double lo,double hi,double step,double value){s.setRange(lo,hi,step);s.setValue(value);s.setSliderStyle(Slider::LinearHorizontal);s.setTextBoxStyle(Slider::TextBoxRight,false,72,25);s.onValueChange=[this]{repaint();};l.setText(name,dontSendNotification);addAndMakeVisible(s);addAndMakeVisible(l);}
 void select(int n){navigationOverlay.setVisible(false);navigationTab.setButtonText("v");if(anchoredOptions)anchoredOptions->setVisible(false);closeCalibration();if(settingsDrawer)settingsDrawer->setVisible(false);mode=n;measure->setToggleState(n==0,dontSendNotification);dsp->setToggleState(n==1,dontSendNotification);vision->setToggleState(n==2,dontSendNotification);lab->setToggleState(n==3,dontSendNotification);resized();repaint();}
 void save(){DynamicObject::Ptr o=new DynamicObject();o->setProperty("schema",1);o->setProperty("synthetic",false);DynamicObject::Ptr lib=new DynamicObject();lib->setProperty("libraryId",captureLibrary.state().id);lib->setProperty("manifest","Biblioteca/biblioteca.json");lib->setProperty("bindings",libraryDSPBindings);o->setProperty("captureLibrary",var(lib.get()));o->setProperty("gainDB",gain.getValue());o->setProperty("delayMs",delay.getValue());o->setProperty("polarity",polarity);o->setProperty("stacks",stacks.getValue());o->setProperty("boxesPerStack",boxes.getValue());o->setProperty("spacingM",spacing.getValue());o->setProperty("linked",adjustmentLinked);Array<var> micros;for(size_t i=0;i<4;++i){DynamicObject::Ptr m=new DynamicObject();m->setProperty("name",micNames[i].getText());m->setProperty("deviceKey",deviceKeys[i]);m->setProperty("micInput",micInputs[i]);m->setProperty("refInput",refInputs[i]);m->setProperty("visible",traceSwitch[i].getToggleState());m->setProperty("colour",traceSwitch[i].colour.toString());m->setProperty("measurementDelayMs",micDelay[i].getValue());micros.add(var(m.get()));}o->setProperty("micros",micros);
 DynamicObject::Ptr view=new DynamicObject();
 view->setProperty("leftVisible",leftVisible);view->setProperty("rightVisible",rightVisible);
 view->setProperty("irFraction",irFraction);view->setProperty("phaseFraction",phaseFraction);
 view->setProperty("tfDisplay",tfSettings.encode());view->setProperty("graphRepresentations",GraphViewState{plotTypes,irScale,linkSmooth.getToggleState()}.encode());view->setProperty("irScale",irScale);view->setProperty("commandLanguage",commands.language==MyaCommands::Language::ES?"es":"en");view->setProperty("plotChoice",plotChoice.getSelectedId());view->setProperty("measurementView",measurementView);view->setProperty("selectedTF",source.getSelectedId());
 view->setProperty("linkSmoothing",linkSmooth.getToggleState());view->setProperty("coherence",showCoherence.getToggleState());view->setProperty("coherenceThreshold",coherenceThreshold.getValue());
 Array<var> plots;for(size_t i=0;i<3;++i){DynamicObject::Ptr item=new DynamicObject();item->setProperty("trace",plotMic[i].getSelectedId());int chosen=plotMic[i].getSelectedId()-1000;if(chosen>=0&&(size_t)chosen<capturedChoices.size())item->setProperty("captureId",capturedChoices[(size_t)chosen]);if(i>0)item->setProperty("displayType",plotTypes[i-1]);item->setProperty("panX",plotPanX[i]);item->setProperty("panY",plotPanY[i]);item->setProperty("zoomX",plotZoomX[i]);item->setProperty("zoomY",plotZoomY[i]);if(i>0)item->setProperty("smoothing",plotSmooth[i-1].getSelectedId());plots.add(var(item.get()));}view->setProperty("plots",plots);o->setProperty("view",var(view.get()));o->setProperty("outputDeviceKey",outputKey);o->setProperty("outputChannel",outputChannel);if(projectFile.replaceWithText(JSON::toString(var(o.get()))))status="Proyecto guardado";else status="No se pudo guardar el proyecto";repaint();}
 void load(bool writePreferences=true){stopInputs();auto v=JSON::parse(projectFile);if(!v.isObject()||int(v["schema"])!=1){status=String::fromUTF8("No hay un proyecto demo válido guardado");repaint();return;}libraryDSPBindings=Array<var>{};if(v["captureLibrary"]["libraryId"].toString()==captureLibrary.state().id&&v["captureLibrary"]["bindings"].isArray())libraryDSPBindings=v["captureLibrary"]["bindings"];outputKey=v["outputDeviceKey"].toString();outputChannel=int(v["outputChannel"]);gain.setValue(double(v["gainDB"]),dontSendNotification);delay.setValue(double(v["delayMs"]),dontSendNotification);stacks.setValue(double(v["stacks"]),dontSendNotification);boxes.setValue(double(v["boxesPerStack"]),dontSendNotification);spacing.setValue(double(v["spacingM"]),dontSendNotification);polarity=bool(v["polarity"]);adjustmentLinked=bool(v["linked"]);invert->setToggleState(polarity,dontSendNotification);if(auto* ms=v["micros"].getArray()){for(int i=0;i<jmin(4,ms->size());++i){auto m=ms->getReference(i);if(!m.isObject())continue;auto k=(size_t)i;if(m.hasProperty("name")){auto name=m["name"].toString().trim();micNames[k].setText(name.isEmpty()?"TF "+String(i+1):name,dontSendNotification);}deviceKeys[k]=m["deviceKey"].toString();micInputs[k]=deviceKeys[k].isEmpty()?0:jmax(0,int(m["micInput"]));refInputs[k]=deviceKeys[k].isEmpty()?0:jmax(0,int(m["refInput"]));traceSwitch[k].setToggleState(bool(m["visible"]),dontSendNotification);auto col=m["colour"].toString();if(col.length()==8)traceSwitch[k].colour=Colour::fromString(col).withAlpha(1.f);micDelay[k].setValue(double(m["measurementDelayMs"]),dontSendNotification);traceSwitch[k].repaint();}}if(writePreferences)persistInputs();inputEngine.setReferenceGainDb(0);loopCalMessage.clear();auto view=v["view"];if(view.isObject()){
 irScale=jlimit(0,2,int(view["irScale"]));impulseScale.setSelectedId(irScale+1,dontSendNotification);
 leftVisible=bool(view["leftVisible"]);rightVisible=bool(view["rightVisible"]);
 irFraction=jlimit(.1,.6,double(view["irFraction"]));phaseFraction=jlimit(.15,.85,double(view["phaseFraction"]));
 if(view.hasProperty("commandLanguage"))commands.language=view["commandLanguage"].toString()=="en"?MyaCommands::Language::EN:MyaCommands::Language::ES;
 measurementView=jlimit(0,1,int(view["measurementView"]));activePlot=measurementView==1?0:1;plotChoice.setSelectedId(jlimit(1,2,int(view["plotChoice"])),dontSendNotification);source.setSelectedId(jlimit(1,5,int(view["selectedTF"])),dontSendNotification);
 linkSmooth.setToggleState(bool(view["linkSmoothing"]),dontSendNotification);showCoherence.setToggleState(bool(view["coherence"]),dontSendNotification);coherenceThreshold.setValue(jlimit(0.,100.,double(view["coherenceThreshold"])),dontSendNotification);
 if(auto* plots=view["plots"].getArray())for(int i=0;i<jmin(3,plots->size());++i){auto item=plots->getReference(i);if(!item.isObject())continue;auto k=(size_t)i;if(i>0){plotTypes[k-1]=item.hasProperty("displayType")?jlimit(1,2,int(item["displayType"])):i;plotTypeChoice[k-1].setSelectedId(plotTypes[k-1],dontSendNotification);}int chosen=jlimit(1,7,int(item["trace"]));auto capture=item["captureId"].toString();if(i>0&&capture.isNotEmpty()){auto it=std::find(capturedChoices.begin(),capturedChoices.end(),capture);chosen=it==capturedChoices.end()?6:1000+(int)std::distance(capturedChoices.begin(),it);}plotMic[k].setSelectedId(chosen,dontSendNotification);plotPanX[k]=jlimit(-2.,2.,double(item["panX"]));plotPanY[k]=jlimit(-10.,10.,double(item["panY"]));plotZoomX[k]=jlimit(i==0?.25:1.,64.,double(item.hasProperty("zoomX")?item["zoomX"]:item["zoom"]));plotZoomY[k]=jlimit(.25,16.,double(item.hasProperty("zoomY")?item["zoomY"]:item["zoom"]));if(i>0)plotSmooth[k-1].setSelectedId(jlimit(1,7,int(item["smoothing"])),dontSendNotification);}
 if(linkSmooth.getToggleState())plotSmooth[1].setSelectedId(plotSmooth[0].getSelectedId(),dontSendNotification);
 }plotPanX[0]=0;for(int panel:{1,2}){auto k=(size_t)panel;double half=.5/plotZoomX[k];plotPanX[k]=jlimit(half,1.-half,.5+plotPanX[k])-.5;}if(auto graph=GraphViewState::decode(view["graphRepresentations"]))restoreGraphChoices(*graph);if(auto tf=TFDisplayState::decode(view["tfDisplay"])){tfSettings=*tf;reflectTF();if(tfSettings.linkFrequencyZoom)PlotPresentation::linkFrequency(plotZoomX,plotPanX,1);}if(writePreferences){persistTF();persistGraphChoices();}syncNames();resized();tfContent.repaint();status="Proyecto recuperado";repaint();}
 #include "WorkstationCommands.inc"
public:
 void checkForUpdates(bool manual){
  if(!updateChecker){
   Component::SafePointer<Workstation> safe(this);
   updateChecker=std::make_unique<MyaUpdate::Checker>([safe](MyaUpdate::Result result,bool requested){if(safe)safe->showUpdateResult(result,requested);});
  }
  if(!updateChecker->check(MyaIdentity::version(),manual)&&manual)info("Buscar actualizaciones","Ya hay una consulta en curso.");
 }
 StringArray getMenuBarNames() override{return commands.menuNames();}
 PopupMenu getMenuForIndex(int index,const String&) override{return commands.menu(index);}
 void menuItemSelected(int id,int) override{commands.invoke(id);}
 #if defined(MYA_OFFSCREEN_RENDER)
 String previewCaptureId;
 bool previewOrigins(int stage){if(stage==0){auto* data=new DynamicObject();data->setProperty("schema",1);data->setProperty("synthetic",true);data->setProperty("name","ORIGEN GUARDADO DEMO");data->setProperty("colour","ff54ee22");data->setProperty("acquisitionMethod","synthetic-demo");Array<var> points;for(double f:{20.,20000.}){auto* point=new DynamicObject();point->setProperty("frequencyHz",f);point->setProperty("real",2.);point->setProperty("imaginary",0.);points.add(var(point));}data->setProperty("samples",points);previewCaptureId=captureLibrary.capture(var(data),"ORIGEN GUARDADO DEMO","ff54ee22");if(previewCaptureId.isEmpty())return false;for(auto& trace:traceSwitch)trace.setToggleState(false,dontSendNotification);tfSettings.coherenceThreshold=0;refreshLibrary();for(int panel:{1,2})plotMic[(size_t)panel].setSelectedId(1,dontSendNotification);previewCursor(2,.5);return panelValue(.5,1)&&panelValue(.5,2)&&captureLegend(1).contains("ORIGEN GUARDADO DEMO")&&!panelCoherence(.5,2)&&frontTrace(0)<0;}
  if(stage==1){for(int panel:{1,2})plotMic[(size_t)panel].setSelectedId(7,dontSendNotification);}else{if(!captureLibrary.setVisibility({previewCaptureId},false))return false;refreshLibrary();for(int panel:{1,2})plotMic[(size_t)panel].setSelectedId(1,dontSendNotification);}previewCursor(2,.5);return !panelValue(.5,1)&&!panelValue(.5,2)&&!panelCoherence(.5,2)&&capturedFront(1).isEmpty()&&frontTrace(0)<0&&cursorReadout.parts[0].value=="-- Hz"&&cursorReadout.parts[1].value.startsWith("--");}
 bool previewAvgSelection(){auto eventFor=[](Component& c,Point<float> point){auto now=Time::getCurrentTime();return MouseEvent(Desktop::getInstance().getMainMouseSource(),point,ModifierKeys{},1.f,0.f,0.f,0.f,0.f,&c,&c,now,point,now,1,false);};source.setSelectedId(1,sendNotificationSync);mouseDown(eventFor(tfContent,{40.f,(float)CompactLayout::avgTop+12}));if(source.getSelectedId()!=5)return false;source.setSelectedId(1,sendNotificationSync);mouseDown(eventFor(avgReserve,{8.f,8.f}));if(source.getSelectedId()!=5)return false;mouseDown(eventFor(avgColourButton,{8.f,8.f}));commands.refresh();if(avgPlay.isEnabled()||avgColourButton.canToggle||MyaCommands::state(*MyaCommands::find(502),commandContext()).enabled)return false;avgColourButton.mouseDoubleClick(eventFor(avgColourButton,{8.f,8.f}));if(!anchoredOptions||!anchoredOptions->isVisible()||anchoredKey!="avg-colour")return false;anchoredOptions->setVisible(false);return source.getSelectedId()==5;}
 bool previewMeasurementMode(bool ir){auto pan=plotPanX,zoom=plotZoomX;auto types=plotTypes;auto smooth=std::array<int,2>{{plotSmooth[0].getSelectedId(),plotSmooth[1].getSelectedId()}};if(!commands.invoke(ir?981:980))return false;commands.refresh();return measurementButtons[20]->getToggleState()==!ir&&measurementButtons[21]->getToggleState()==ir&&!measurementButtons[0]->getToggleState()&&!measurementButtons[0]->isEnabled()&&impulseScale.isVisible()&&plotTypeChoice[0].isVisible()==!ir&&(ir?plotBounds(0).getHeight()>300:plotBounds(0).getHeight()>0&&plotBounds(1).getHeight()==plotBounds(2).getHeight())&&pan==plotPanX&&zoom==plotZoomX&&types==plotTypes&&smooth==std::array<int,2>{{plotSmooth[0].getSelectedId(),plotSmooth[1].getSelectedId()}};}
 bool previewNavigationChoice(){for(int index:{0,1,2,3}){if(!navigationOverlay.isVisible())navigationTab.onClick();buttons[(size_t)index]->onClick();if(navigationOverlay.isVisible()||mode!=index)return false;if(!commands.invoke(980)||mode!=0||measurementView!=0)return false;}return !navigationOverlay.isVisible();}
 bool previewNavigationOverlay(bool open){auto p1=plotBounds(1),p2=plotBounds(2);if(navigationOverlay.isVisible()!=open)navigationTab.onClick();return navigationOverlay.isVisible()==open&&p1==plotBounds(1)&&p2==plotBounds(2)&&navigationTab.getY()==66&&buttons[0]->getParentComponent()==&navigationOverlay;}
 bool previewSurface(const Component& surface)const{for(int i=0;i<8;++i)if(buttons[(size_t)i]->getParentComponent()!=&navigationOverlay)return false;return getY()==-66&&getHeight()==surface.getHeight()+66&&plotBounds(1).getHeight()==plotBounds(2).getHeight()&&!navigationOverlay.isVisible();}

 bool previewIndependentViews(){auto side=dockMeterState.primaryCard()->id,floating=floatingMeterState.primaryCard()->id;int sChannel=dockMeterState.primaryCard()->channel,fMetric=floatingMeterState.primaryCard()->metric;auto card=*meterState.primaryCard();card.channel=3;card.metric=3;card.weight=2;if(!meterState.update(card)||dockMeterState.primaryCard()->channel!=sChannel||floatingMeterState.primaryCard()->metric!=fMetric||side==card.id||floating==card.id||side==floating)return false;for(auto grid:{std::pair<int,int>{2,2},{1,4},{4,1},{1,1}}){if(!dockMeterState.setGrid(grid.first,grid.second))return false;dockMeters.refreshNow();if(!dockMeters.previewPages())return false;MeterPanel floats(floatingMeterState);floats.setSize(480,340);if(!floatingMeterState.setGrid(grid.first,grid.second)||!floats.previewPages())return false;}return dockMeterState.primaryCard()->id==side&&floatingMeterState.primaryCard()->id==floating;}
 bool previewRoutingStatus(){closeCalibration();if(settingsDrawer)settingsDrawer->setVisible(false);auto keys=deviceKeys;auto m=micInputs,r=refInputs;audioSettings();if(!routingStatusPanel||!routingStatusPanel->previewStatus().contains("Entradas detenidas"))return false;auto* panel=routingStatusPanel.getComponent();panel->setAcquisitionState(true);if(!panel->previewStatus().contains("Entradas dBFS activas"))return false;panel->setAcquisitionState(false,true);if(!panel->previewStatus().contains("Esperando permiso"))return false;stopInputs();if(!panel->previewStatus().contains("Entradas detenidas"))return false;return keys==deviceKeys&&m==micInputs&&r==refInputs&&!inputEngine.running();}
 bool previewAcquisitionGuard(){if(inputEngine.running()||permissionPending)return false;beginInputs(15);if(inputEngine.running()||permissionPending)return false;source.setSelectedId(5,sendNotificationSync);commands.refresh();for(size_t k=0;k<tfActions.size();++k){if(!tfActions[k][3].isEnabled())return false;tfActions[k][3].onClick();if(source.getSelectedId()!=(int)k+1||inputEngine.running()||permissionPending)return false;}stopInputs();return true;}
 bool previewPanelToggle(){closeCalibration();if(settingsDrawer)settingsDrawer->setVisible(false);principalMeterOptions();if(!settingsDrawer||!settingsDrawer->isVisible())return false;principalMeterOptions();return !settingsDrawer->isVisible();}
 bool previewCompactMeters(int rows,int columns){
  auto p1=plotBounds(1),p2=plotBounds(2);auto selection=libraryPanel.selected();dockMeterState.sidebar(false);commands.invoke(961);if(!dockMeters.isVisible()||!libraryPanel.isVisible()||libraryPanel.getHeight()!=24||!previewLeftHeader())return false;commands.invoke(961);if(dockMeters.isVisible()||libraryPanel.selected()!=selection)return false;
  if(!dockMeterState.setGrid(rows,columns))return false;commands.invoke(961);dockMeters.refreshNow();resized();int count=0;std::function<void(Component&)> inspect=[&](Component& c){if(dynamic_cast<CompactMeterTile*>(&c)){if(c.isVisible()&&(c.getWidth()<1||c.getHeight()<112))count=-1000;else ++count;}for(auto* child:c.getChildren())inspect(*child);};inspect(dockMeters);return count==(int)dockMeterState.state().cards.size()&&p1==plotBounds(1)&&p2==plotBounds(2);
 }
 void previewCloseCompactMeters(){dockMeterState.sidebar(false);dockMeters.refreshNow();resized();}
 Image previewFloatingGrid(int rows=2,int columns=2,int width=720,int height=330){floatingMeterState.setGrid(rows,columns);MeterPanel panel(floatingMeterState);panel.setLookAndFeel(&look);panel.setSize(width,height);return panel.createComponentSnapshot(panel.getLocalBounds());}
 void previewCursor(int panel,double u){cursorPlot=panel;cursorU=u;updateCursorReadout();}
 void previewMeters(){for(size_t k=0;k<4;++k){inputMeters[k].channels[0].update(-2.,0,inputMeterSettings);inputMeters[k].channels[0].update(-18.+k*3,.2,inputMeterSettings);inputMeters[k].channels[1].update(-20.+k*3,0,inputMeterSettings);}status=String::fromUTF8("DEMO VISUAL AISLADA · niveles sintéticos · sin audio · AVG pendiente");}
 void previewFolds(bool open){alignmentExpanded=open;resized();}
 bool previewLeftHeader()const{return tracesPage.getWidth()==metersPage.getWidth()&&tracesPage.getHeight()==metersPage.getHeight()&&tracesPage.getX()==libraryPanel.getX()&&metersPage.getX()-libraryPanel.getX()==tracesPage.getWidth()+2&&libraryPanel.previewHeaderMatches(tracesPage.getWidth(),tracesPage.getHeight());}
 bool previewReadoutSources(){auto old1=plotMic[1].getSelectedId(),old2=plotMic[2].getSelectedId();plotMic[1].setSelectedId(2,dontSendNotification);plotMic[2].setSelectedId(4,dontSendNotification);previewCursor(2,.6);auto magnitude=panelValue(cursorU,2),phase=panelValue(cursorU,1);bool ok=cursorReadout.parts.size()==4&&cursorReadout.parts[1].value==(magnitude?String(tfSettings.physical(*magnitude,2),2)+" dB":String("--"))&&cursorReadout.parts[2].value==(phase?String(tfSettings.physical(*phase,1),1)+String::fromUTF8("°"):String("--"))&&cursorReadout.parts[3].label=="Coh P2/P1"&&cursorReadout.getTooltip().contains("P2 TF 3")&&cursorReadout.getTooltip().contains("P1 TF 1");plotMic[1].setSelectedId(old1,dontSendNotification);plotMic[2].setSelectedId(old2,dontSendNotification);updateCursorReadout();return ok;}
 bool previewSignalOptions(int id){auto keys=deviceKeys;auto mic=micInputs,ref=refInputs;auto out=outputKey;int channel=outputChannel;double beforeGain=gain.getValue(),beforeDelay=delay.getValue();auto graph1=plotBounds(1),graph2=plotBounds(2);auto prefs=projectFile.getParentDirectory().getChildFile("vista-trabajo.json");auto beforePrefs=prefs.loadFileAsString(),beforeProject=projectFile.loadFileAsString();Button* button=id==970?static_cast<Button*>(&loopOptions):static_cast<Button*>(measurementButtons[id==517?10:12].get());if(!button->isEnabled()||!button->onClick)return false;button->onClick();if(!anchoredOptions||!anchoredOptions->isVisible()||!optionsAnchor)return false;auto anchor=getLocalArea(optionsAnchor,optionsAnchor->getLocalBounds());if(anchoredOptions->getRight()!=anchor.getX()-6||anchoredOptions->getX()<0||anchoredOptions->getBottom()>getHeight()-33||(principalMeter.isVisible()&&anchoredOptions->getBounds().intersects(principalMeter.getBounds())))return false;button->onClick();if(anchoredOptions->isVisible())return false;button->onClick();if(!anchoredOptions->isVisible())return false;return keys==deviceKeys&&mic==micInputs&&ref==refInputs&&out==outputKey&&channel==outputChannel&&beforeGain==gain.getValue()&&beforeDelay==delay.getValue()&&graph1==plotBounds(1)&&graph2==plotBounds(2)&&beforePrefs==prefs.loadFileAsString()&&beforeProject==projectFile.loadFileAsString()&&measurementButtons[9]->isEnabled()&&measurementButtons[11]->isEnabled()&&measurementButtons[4]->isEnabled()&&!avgPlay.isEnabled()&&avgColourButton.isEnabled();}
 void previewCloseSignalOptions(){if(anchoredOptions)anchoredOptions->setVisible(false);}
 bool previewDoubleClickRouting(){
  auto routes=deviceKeys;auto mic=micInputs,ref=refInputs;auto output=outputKey;int channel=outputChannel;
  auto eventFor=[](Component& component,Point<float> point){auto now=Time::getCurrentTime();return MouseEvent(Desktop::getInstance().getMainMouseSource(),point,ModifierKeys{},1.f,0.f,0.f,0.f,0.f,&component,&component,now,point,now,2,false);};
  for(size_t k=0;k<4;++k){
   for(int ch=0;ch<2;++ch){auto b=CompactLayout::meterBar(inputMeters[k].getWidth(),ch);auto event=eventFor(inputMeters[k],{b.x+b.w*.5f,b.y+b.h*.5f});inputMeters[k].mouseDoubleClick(event);mouseDoubleClick(event);if(drawerKey!="input-meters"||!settingsDrawer||!settingsDrawer->isVisible())return false;settingsDrawer->setVisible(false);}
   auto scale=eventFor(inputMeters[k],{25.f,22.f});inputMeters[k].mouseDoubleClick(scale);if(settingsDrawer->isVisible())return false;
   auto labelEvent=eventFor(micNames[k],{10.f,10.f});mouseDoubleClick(labelEvent);if(drawerKey!="TF "+String((int)k+1)||!settingsDrawer->isVisible())return false;settingsDrawer->setVisible(false);
   traceSwitch[k].mouseDoubleClick(eventFor(traceSwitch[k],{12.f,10.f}));if(drawerKey!="TF "+String((int)k+1)||!settingsDrawer->isVisible())return false;settingsDrawer->setVisible(false);
   colourEdit[k].onClick();if(drawerKey!="TF "+String((int)k+1)||!settingsDrawer->isVisible())return false;settingsDrawer->setVisible(false);
  }
  return deviceKeys==routes&&micInputs==mic&&refInputs==ref&&outputKey==output&&outputChannel==channel;
 }
 void previewOpenMeterOptions(){auto box=CompactLayout::meterBar(inputMeters[0].getWidth(),0);auto now=Time::getCurrentTime();MouseEvent event(Desktop::getInstance().getMainMouseSource(),{(float)box.x+20,(float)box.y+5},ModifierKeys{},1.f,0.f,0.f,0.f,0.f,&inputMeters[0],&inputMeters[0],now,{(float)box.x+20,(float)box.y+5},now,2,false);inputMeters[0].mouseDoubleClick(event);}
 bool previewFits()const{return tfContent.getHeight()<=tfViewport.getHeight()&&inputMeters[3].getBottom()<=CompactLayout::tfCardsHeight&&avgReserve.getBottom()<=tfViewport.getHeight();}
 bool previewListAccessible()const{int maximum=jmax(0,tfContent.getHeight()-tfViewport.getHeight());return avgReserve.getBottom()-maximum<=tfViewport.getHeight()&&tfViewport.getHeight()>=CompactLayout::tfCard;}
 bool previewPrincipal(){auto graph1=plotBounds(1),graph2=plotBounds(2);if(!meterState.state().mainVisible||!principalMeter.isVisible()||principalMeter.getBottom()>measurementButtons[20]->getY())return false;commands.invoke(972);if(!meterState.state().clock)return false;commands.invoke(972);if(meterState.state().clock)return false;commands.invoke(971);if(principalMeter.isVisible())return false;commands.invoke(971);principalMeter.refresh();return principalMeter.isVisible()&&graph1==plotBounds(1)&&graph2==plotBounds(2);}
 void previewAvgColour(Colour colour){avgColour=colour;reflectAvgColour();}
 void previewMeterClock(bool clock){if(meterState.state().clock!=clock)commands.invoke(972);principalMeter.refresh();}
 void fixtureCleanSave(){previewOrigins(0);for(auto& trace:traceSwitch)trace.setToggleState(false,dontSendNotification);source.setSelectedId(1,dontSendNotification);plotMic[0].setSelectedId(1,dontSendNotification);plotMic[1].setSelectedId(2,dontSendNotification);plotMic[2].setSelectedId(2,dontSendNotification);measurementView=0;save();}
 bool fixtureIsClean(){for(const auto& trace:traceSwitch)if(trace.getToggleState())return false;return plotMic[0].getSelectedId()==1&&plotMic[1].getSelectedId()==2&&plotMic[2].getSelectedId()==2&&frontTrace(0)<0&&frontTrace(1)<0&&frontTrace(2)<0&&capturedFront(1).isEmpty()&&capturedFront(2).isEmpty()&&!inputEngine.running()&&!permissionPending&&captureLibrary.state().nodes.size()>0;}
 String fixtureRoutes(){return JSON::toString(routes().encode());}
 void fixtureLoad(){load();}
 bool previewCalibrationDrawer(){if(settingsDrawer)settingsDrawer->setVisible(false);showCalibration();if(drawerKey!="spl-calibration"||!settingsDrawer||!settingsDrawer->isVisible()||inputEngine.running())return false;beginInputs(0,InputLevels::Source{"Core / Card",1});return !inputEngine.running()&&!permissionPending;}
 void previewPrincipalConfig(int tab=0){closeCalibration();if(settingsDrawer)settingsDrawer->setVisible(false);auto panel=std::make_unique<MeterConfiguration>(meterState,[this]{commands.invoke(203);},[this]{deferredCalibration();});panel->selectTab(tab);showDrawer("principal-meter","SPL / configuracion",std::move(panel));}

 #endif
 explicit Workstation(File savedProject=defaultProjectFile()):projectFile(std::move(savedProject)){setLookAndFeel(&look);
 navigationOverlay.draw=[this](Graphics& g){g.setColour(Palette::panel.withAlpha(.98f));g.fillRoundedRectangle(navigationOverlay.getLocalBounds().toFloat(),5);g.setColour(Palette::line);g.drawRoundedRectangle(navigationOverlay.getLocalBounds().toFloat().reduced(.5f),5,1);};addChildComponent(navigationOverlay);addAndMakeVisible(navigationTab);navigationTab.getProperties().set("commandGlyph",true);navigationTab.onClick=[this]{navigationOverlay.setVisible(!navigationOverlay.isVisible());if(navigationOverlay.isVisible()){navigationOverlay.toFront(false);commands.refresh();}navigationTab.setButtonText(navigationOverlay.isVisible()?"^":"v");};navigationMenus.onClick=[this]{PopupMenu menu;auto names=commands.menuNames();for(int i=0;i<names.size();++i)menu.addSubMenu(names[i],commands.menu(i));Component::SafePointer<Workstation> safe(this);menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&navigationMenus),[safe](int id){if(safe&&id){safe->commands.invoke(id);safe->navigationOverlay.setVisible(false);safe->navigationTab.setButtonText("v");}});};navigationOverlay.addAndMakeVisible(navigationMenus);
 coherenceThreshold.setRange(0,100,.01);coherenceThreshold.setValue(0);coherenceThreshold.setSliderStyle(Slider::LinearVertical);coherenceThreshold.setTextBoxStyle(Slider::NoTextBox,false,0,0);coherenceThreshold.setTitle("Coherence Threshold (%)");coherenceThreshold.setTooltip("Umbral de coherencia: oculta muestras medidas por debajo del porcentaje.");coherenceThreshold.onValueChange=[this]{tfSettings.coherenceThreshold=coherenceThreshold.getValue();persistTF();status="Coherence Threshold: "+String(coherenceThreshold.getValue(),2).trimCharactersAtEnd("0").trimCharactersAtEnd(".")+"%";repaint();};addAndMakeVisible(coherenceThreshold);
 showCoherence.setToggleState(true,dontSendNotification);addAndMakeVisible(showCoherence);

 cursorReadout.setText("-- Hz       -- dB       -- deg",dontSendNotification);cursorReadout.setJustificationType(Justification::centred);cursorReadout.setFont(Font(FontOptions(15.f)));cursorReadout.setColour(Label::backgroundColourId,Palette::plot);cursorReadout.setInterceptsMouseClicks(true,false);addAndMakeVisible(cursorReadout);

 for(int i=0;i<2;++i){auto& divider=plotDividers[(size_t)i];addAndMakeVisible(divider);divider.reset=[this]{irFraction=.14;phaseFraction=.5;resized();repaint();};divider.moved=[this,i](int y){int height=getHeight()-183;if(i==0)irFraction=jlimit(60.,(double)jmax(60,height-176),(double)y-118)/height;else{auto p=plotBounds(1);int remaining=118+height-p.getY()-8;phaseFraction=jlimit(80.,(double)jmax(80,remaining-80),(double)y-p.getY())/remaining;}resized();repaint();};}

 impulseScale.addItemList({"Lin","Log","ETC"},1);impulseScale.setSelectedId(1,dontSendNotification);impulseScale.setTitle("IR: escala de amplitud; tiempo lineal");impulseScale.setTooltip(juce::String::fromUTF8("IR calculada desde la TF medida. Lin, Log y ETC cambian la representación de amplitud."));
 impulseScale.onChange=[this]{int id=impulseScale.getSelectedId();if(id<1||id>3||!commands.invoke(929+id))impulseScale.setSelectedId(irScale+1,dontSendNotification);};addAndMakeVisible(impulseScale);
 for(int t=0;t<2;++t){auto& c=plotTypeChoice[(size_t)t];c.addItem("Magnitud / Magnitude",2);c.addItem("Fase / Phase",1);c.addItem(juce::String::fromUTF8("RTA — adquisición pendiente"),3);c.addItem(juce::String::fromUTF8("Espectrograma — pendiente"),4);c.setItemEnabled(3,false);c.setItemEnabled(4,false);c.setSelectedId(t+1,dontSendNotification);
  c.onChange=[this,t]{int kind=plotTypeChoice[(size_t)t].getSelectedId();if((kind!=1&&kind!=2)||!commands.invoke((t==0?940:950)+(kind==1?1:0)))plotTypeChoice[(size_t)t].setSelectedId(plotTypes[(size_t)t],dontSendNotification);};addAndMakeVisible(c);
 }
 for(int t=0;t<3;++t){auto& c=plotMic[(size_t)t];c.addItem("All visible",1);for(int i=0;i<4;++i)c.addItem("TF "+String(i+1),i+2);c.setSelectedId(1);c.setTitle(String(t==0?"Impulse":t==1?"Phase":"Magnitude")+" / Trace");c.onChange=[this,t]{frontCaptured[(size_t)t].clear();repaint();};addAndMakeVisible(c);}
 for(int t=0;t<2;++t){auto& c=plotSmooth[(size_t)t];c.addItemList({"Off","1/48","1/24","1/12","1/6","1/3","1 oct"},1);c.setTooltip("Suavizado visual: Off = sin suavizado; no cambia el promedio");c.setSelectedId(1);c.setTitle(String(t==0?"Phase":"Magnitude")+" / Smoothing");c.onChange=[this,t]{if(linkSmooth.getToggleState())plotSmooth[(size_t)(1-t)].setSelectedId(plotSmooth[(size_t)t].getSelectedId(),dontSendNotification);updateCursorReadout();repaint();};addAndMakeVisible(c);}
 linkSmooth.setToggleState(true,dontSendNotification);

 measure=add(String::fromUTF8("Medición"),[]{});
 dsp=add(String::fromUTF8("DSP / Ajuste"),[]{});
 vision=add(String::fromUTF8("Mya Mapp Vision 3D"),[]{});
 lab=add(String::fromUTF8("Laboratorio"),[]{});
 add(String::fromUTF8("Guardar"),[]{});
 add(String::fromUTF8("Abrir demo"),[]{});
 add(String::fromUTF8("Audio settings"),[]{});
 add(String::fromUTF8("Ayuda"),[]{});
 add(String::fromUTF8("Auto EQ"),[]{});
 add(String::fromUTF8("Auto FIR"),[]{});
 add(String::fromUTF8("Target"),[]{});
 add(String::fromUTF8("Filtros"),[]{});
 invert=add(String::fromUTF8("Ø Polaridad"),[]{});
 bypass=add(String::fromUTF8("Bypass"),[]{});
 add(String::fromUTF8("DSP → 3D"),[]{});
 add(String::fromUTF8("Diseño → DSP"),[]{});
 add(String::fromUTF8("Calibración"),[]{});
 add(String::fromUTF8("Find / Track"),[]{});
 add(String::fromUTF8("Exportar DSP"),[]{});
 for(int i=0;i<8;++i)navigationOverlay.addAndMakeVisible(*buttons[(size_t)i]);
 setup(gain,gainLabel,String::fromUTF8("Ganancia · dB"),-18,12,.1,-2);setup(delay,delayLabel,String::fromUTF8("Delay · ms"),0,30,.01,3.2);
 setup(stacks,stacksLabel,"Stacks",1,24,1,12);setup(boxes,boxesLabel,"Cajas por stack",1,4,1,2);setup(spacing,spacingLabel,String::fromUTF8("Separación · m"),.2,5,.1,2.5);setup(rotation,rotationLabel,String::fromUTF8("Giro de escena · °"),-60,60,1,20);

 const Colour initialColours[]={Palette::green,Colour(0xff4d88ff),Colour(0xfff03baf),Colour(0xffffe33b)};
 for(int i=0;i<4;++i){auto idx=(size_t)i;traceSwitch[idx].openSettings=[this,i]{source.setSelectedId(i+1,sendNotificationSync);commands.invoke(202);};traceSwitch[idx].colour=initialColours[i];micNames[idx].setText("TF "+String(i+1),dontSendNotification);micNames[idx].setEditable(false,false,false);micNames[idx].setInterceptsMouseClicks(true,false);micNames[idx].setTitle("Nombre del motor "+String(i+1));micNames[idx].onTextChange=[this,i]{auto k=(size_t)i;if(micNames[k].getText().trim().isEmpty())micNames[k].setText("TF "+String(i+1),dontSendNotification);syncNames();};addAndMakeVisible(micNames[idx]);traceSwitch[idx].setTitle("Mostrar u ocultar MICRO "+String(i+1));traceSwitch[idx].onClick=[this,i]{status="MICRO "+String(i+1)+(traceSwitch[(size_t)i].getToggleState()?" visible":" oculto")+String::fromUTF8(" · solo visibilidad, no cambia el promedio");repaint();};addAndMakeVisible(traceSwitch[idx]);
 colourEdit[idx].setButtonText("...");colourEdit[idx].setTitle("Opciones TF "+String(i+1));colourEdit[idx].onClick=[this,i]{source.setSelectedId(i+1,sendNotificationSync);commands.invoke(202);};addAndMakeVisible(colourEdit[idx]);
 micDelay[idx].setRange(0,2000,.01);micDelay[idx].setSliderStyle(Slider::IncDecButtons);micDelay[idx].setTextBoxStyle(Slider::TextBoxLeft,false,82,22);micDelay[idx].setTextValueSuffix(" ms");micDelay[idx].setTitle("Delay de medicion MICRO "+String(i+1));micDelay[idx].onValueChange=[this,idx]{if(source.getSelectedId()==int(idx)+1)selectedDelay.setValue(micDelay[idx].getValue(),dontSendNotification);tfEngines[idx].setDelayMs(micDelay[idx].getValue());status=String::fromUTF8("Delay de medición manual aplicado a TF ")+String((int)idx+1);repaint();};addAndMakeVisible(micDelay[idx]);}
 source.addItemList({String::fromUTF8("TF 1 · EMX-7150"),String::fromUTF8("TF 2 · EMX-7150"),String::fromUTF8("TF 3 · EMX-7150"),String::fromUTF8("TF 4 · EMX-7150"),String::fromUTF8("AVG · audiencia")},1);source.setSelectedId(1);source.onChange=[this]{if(pinkLoopActive||permissionPending){stopInputs("TF seleccionada modificada; loop detenido.");inputEngine.setReferenceGainDb(0);loopCalMessage.clear();}int selectedDevice=source.getSelectedId()-1;for(auto* model:meterModels())model->setDefaultDevice(selectedDevice>=0&&selectedDevice<4?deviceKeys[(size_t)selectedDevice]:String{});frontCaptured[1].clear();frontCaptured[2].clear();int i=source.getSelectedId()-1;if(i>=0&&i<4)selectedDelay.setValue(micDelay[(size_t)i].getValue(),dontSendNotification);refreshLoopOptions();tfContent.repaint();repaint();};addAndMakeVisible(source);setWantsKeyboardFocus(true);dockMeters.calibrateRequested=[this](String k,int c){deferredCalibration(k,c);};syncNames();
 for(auto name:{"Spectrum","TF Mode","Decibel","RT60","Int. Loop","Timer","...","Find","...","Pink","...","Sweep","...","+ TF","+ AVG","Phase","...","Restaurar","Polaridad","Track todos","Real Time","Impulse","SPL",">","Stop","Pause","Config","View","On","Captura [Espacio]"})addMeasurementButton(name);
 const char* icons[]={"play","stop","pause"};const char* titles[]={"Iniciar todos","Detener todos","Pausar todos"};
 for(int n=0;n<3;++n){auto& b=*measurementButtons[(size_t)(23+n)];b.getProperties().set("transportIcon",icons[n]);b.setButtonText("");b.setTitle(titles[n]);b.setTooltip(titles[n]);}
 measurementButtons[9]->setButtonText("Pink [G]");measurementButtons[11]->setButtonText("Sweep [S]");
 measurementButtons[20]->setToggleState(true,dontSendNotification);
 for(auto* c:{&meterInput,&meterWeight,&generatorOutput})addAndMakeVisible(c);meterInput.setEnabled(false);meterWeight.setEnabled(false);generatorOutput.setEnabled(true);generatorOutput.onChange=[this]{int channel=generatorOutput.getSelectedId();if(channel<1||channel==outputChannel)return;if(inputEngine.running()||permissionPending)stopInputs("Salida del generador modificada; salida en silencio.");outputChannel=channel;persistInputs();syncNames();};
 
 meterWeight.addItem("dB SPL A Fast",1);meterWeight.setSelectedId(1);meterWeight.setTooltip("Maqueta de medidor. Ponderaciones y calibracion pendientes.");
 generatorLevel.setText("-20 dBFS",dontSendNotification);generatorLevel.setJustificationType(Justification::centred);addAndMakeVisible(generatorLevel);
 measurementButtons[7]->setButtonText("Find L / F");measurementButtons[7]->setTooltip("L: tiempo individual. F: volver a buscar el tiempo de la TF seleccionada.");
 measurementButtons[1]->setToggleState(true,dontSendNotification);
 measurementButtons[6]->setTooltip("Ajustes de Timer");measurementButtons[8]->setTooltip("Advanced Delay Finder");measurementButtons[10]->setTooltip("Pink: Gain / HPF / LPF");measurementButtons[12]->setTooltip("Sweep: Gain / HPF / LPF");measurementButtons[16]->setTooltip("Resultados de Auto Phase");

 plotChoice.addItem("Impulso en TF",1);plotChoice.addItem("Ocultar impulso TF",2);plotChoice.setTitle("Visibilidad del impulso superior en TF");plotChoice.setTooltip("Este ajuste solo muestra/oculta el impulso dentro de TF; no selecciona el modo IR.");plotChoice.setSelectedId(1);addAndMakeVisible(plotChoice);
 plotChoice.onChange=[this]{persistTF();resized();repaint();};
 averageChoice.addItemList({"Prom.1","Prom.2","Prom.4","Prom.5","Prom.8","Prom.16","Prom.32","Prom.64"},1);averageChoice.setSelectedId(4);addAndMakeVisible(averageChoice);
 averageChoice.setTooltip("Profundidad real del promedio temporal TF");averageChoice.onChange=[this]{const int depths[]={1,2,4,5,8,16,32,64};int id=jlimit(1,8,averageChoice.getSelectedId());liveTF.setAveraging(depths[id-1],true);status="Promedio TF exponencial / profundidad "+String(depths[id-1]);repaint();};
 spectrumFFT.addItemList({"FFT 4K","FFT 8K","FFT 16K","FFT 32K","FFT 64K","FFT 128K"},1);spectrumFFT.setSelectedId(4);
 spectrumSpeed.addItemList({"5 FPS","10 FPS","20 FPS","30 FPS","50 FPS"},1);spectrumSpeed.setSelectedId(3);
 spectrumWindow.addItemList({"Hann","Hamming","Blackman-Harris","Flat Top","Rectangular"},1);spectrumWindow.setSelectedId(1);
 spectrumAverage.addItemList({"Sin promedio","Exp 2","Exp 4","Exp 8","Exp 16","FIFO 4","FIFO 8","FIFO 16"},1);spectrumAverage.setSelectedId(3);
 auto configureSpectrum=[this]{SpectrumCore::Settings s;const int sizes[]={4096,8192,16384,32768,65536,131072},fps[]={5,10,20,30,50};s.fftSize=sizes[jlimit(1,6,spectrumFFT.getSelectedId())-1];s.framesPerSecond=fps[jlimit(1,5,spectrumSpeed.getSelectedId())-1];const MyaDSP::Window windows[]={MyaDSP::Window::Hann,MyaDSP::Window::Hamming,MyaDSP::Window::BlackmanHarris,MyaDSP::Window::FlatTop,MyaDSP::Window::Rectangular};s.window=windows[jlimit(1,5,spectrumWindow.getSelectedId())-1];int a=spectrumAverage.getSelectedId();s.average=a==1?SpectrumCore::Average::None:a>=6?SpectrumCore::Average::Fifo:SpectrumCore::Average::Exponential;const int depths[]={1,2,4,8,16,4,8,16};s.depth=depths[jlimit(1,8,a)-1];s.overlap=.75;liveSpectrum.configure(s);status="Espectro: "+String(s.fftSize)+" puntos / "+String(s.framesPerSecond,0)+" FPS";repaint();};
 for(auto* combo:{&spectrumFFT,&spectrumSpeed,&spectrumWindow,&spectrumAverage}){addAndMakeVisible(*combo);combo->onChange=configureSpectrum;}configureSpectrum();
 for(auto* slider:{&selectedDelay,&phasePreview}){slider->setRange(0,2000,.01);slider->setSliderStyle(Slider::IncDecButtons);slider->setTextBoxStyle(Slider::TextBoxLeft,false,86,26);slider->setTextValueSuffix(" ms");addAndMakeVisible(slider);}
 selectedDelay.setTitle("Delay de referencia seleccionado");
 selectedDelay.onValueChange=[this]{int i=source.getSelectedId()-1;if(i>=0&&i<4){micDelay[(size_t)i].setValue(selectedDelay.getValue());if(i==liveTFIndex)liveTF.setDelayMs(selectedDelay.getValue());}};
 phasePreview.setEnabled(false);phasePreview.setTooltip("Auto Phase pendiente");
 tfContent.addMouseListener(this,true);tfViewport.setViewedComponent(&tfContent,false);tfViewport.setScrollBarsShown(true,false);tfViewport.setScrollBarThickness(10);addAndMakeVisible(tfViewport);
 for(int i=0;i<4;++i){auto k=(size_t)i;for(auto* c:std::initializer_list<Component*>{&micNames[k],&traceSwitch[k],&colourEdit[k],&micDelay[k]})tfContent.addAndMakeVisible(c);
 const char* labels[]={"Find","Track","AG",">"};for(int j=0;j<4;++j){auto& b=tfActions[k][(size_t)j];b.setButtonText(labels[j]);if(j==3){b.getProperties().set("transportIcon","play");b.setButtonText("");b.setTitle("Iniciar TF "+String(i+1));b.setTooltip("Iniciar TF "+String(i+1));}tfContent.addAndMakeVisible(b);}}
 for(size_t k=0;k<4;++k){inputMeters[k].openOptions=[this]{inputMeterOptions();};tfContent.addAndMakeVisible(inputMeters[k]);tfActions[k][1].getProperties().set("trackCircle",true);}
 measurementButtons[19]->getProperties().set("trackCircle",true);measurementButtons[19]->getProperties().set("trackColour",Palette::cyan.toString());
 tfContent.draw=[this](Graphics& g){int w=tfContent.getWidth();for(int i=0;i<4;++i){int y=i*CompactLayout::tfRow;bool selected=source.getSelectedId()==i+1;g.setColour(selected?Colour(0xff35434d):Colour(0xff192126));g.fillRoundedRectangle(0,(float)y,(float)w,(float)CompactLayout::tfCard,4);g.setColour(traceSwitch[(size_t)i].colour.withAlpha(selected?1.f:.55f));g.drawRoundedRectangle(1.5f,(float)y+1.5f,(float)w-3.f,(float)CompactLayout::tfCard-3.f,4,selected?3.f:1.f);

 }bool selected=source.getSelectedId()==5;auto row=Rectangle<float>{0,(float)CompactLayout::avgTop,(float)w,(float)CompactLayout::avgHeight};g.setColour(selected?Colour(0xff35434d):Colour(0xff192126));g.fillRoundedRectangle(row,4);g.setColour(avgColour.withAlpha(selected?1.f:.55f));g.drawRoundedRectangle(row.reduced(1.5f),4,selected?3.f:1.f);};

 deviceWatcher.addChangeListener(this);audioEndpoints=discoverAudioEndpoints();syncNames();setupCommands();refreshMeterSources();
 addAndMakeVisible(tracesPage);addAndMakeVisible(metersPage);commands.bind(tracesPage,960);commands.bind(metersPage,961);addAndMakeVisible(dockMeters);dockMeters.toggleRequested=[this]{commands.invoke(203);};meterRevision=meterState.configRevision();meterListener=meterState.listen([this]{if(meterRevision!=meterState.configRevision()){meterRevision=meterState.configRevision();resized();repaint();menuItemsChanged();}});dockRevision=dockMeterState.configRevision();dockListener=dockMeterState.listen([this]{if(dockRevision!=dockMeterState.configRevision()){dockRevision=dockMeterState.configRevision();resized();repaint();menuItemsChanged();}});addAndMakeVisible(principalMeter);principalMeter.command=[this](int id){commands.invoke(id);};for(size_t i=0;i<graphGear.size();++i){auto& gear=graphGear[i];gear.setButtonText("...");gear.getProperties().set("commandGlyph",true);commands.bind(gear,i==0?963:303);addAndMakeVisible(gear);}
 captureLibrary.adoptLegacy(projectFile.getParentDirectory().getChildFile("Capturas"));
 libraryPanel.onCommand=[this](int id){commands.invoke(id);};
 libraryPanel.commandText=[this](int id){auto* c=MyaCommands::find(id);return c?commands.details(*c,MyaCommands::state(*c,commands.current())):String{};};
 libraryPanel.onChange=[this]{refreshLibrary();};libraryPanel.onSelection=[this]{repaint();menuItemsChanged();};
 libraryPanel.onFront=[this](String id){auto* n=captureLibrary.find(id);if(n&&n->visible&&n->kind==CaptureLibrary::Kind::SnapshotTF){frontCaptured[1]=frontCaptured[2]=id;auto it=std::find(capturedChoices.begin(),capturedChoices.end(),id);if(it!=capturedChoices.end())for(int panel:{1,2})plotMic[(size_t)panel].setSelectedId(1000+(int)std::distance(capturedChoices.begin(),it),dontSendNotification);repaint();}};
 libraryPanel.onError=[this](String message){status=message;repaint();};
 addAndMakeVisible(libraryPanel);restoreLibraryButton.getProperties().set("commandGlyph",true);commands.bind(restoreLibraryButton,405);addAndMakeVisible(restoreLibraryButton);
 const int aiIDs[]={814,965,810,812,813,966,967};for(size_t i=0;i<aiButtons.size();++i){commands.bind(aiButtons[i],aiIDs[i],true);addAndMakeVisible(aiButtons[i]);}commands.bind(sumButton,968,true);addAndMakeVisible(sumButton);
 alignmentFold.onClick=[this]{alignmentExpanded=!alignmentExpanded;resized();repaint();};
 avgReserve.setText("AVG",dontSendNotification);avgReserve.setColour(Label::textColourId,Palette::orange);avgReserve.setFont(Font(FontOptions(11.f)));avgReserve.setTooltip("AVG seleccionable; sin datos ni estimador. Doble clic: editar color.");avgReserve.setInterceptsMouseClicks(true,false);
 tfContent.addAndMakeVisible(avgReserve);tfContent.addAndMakeVisible(avgColourButton);tfContent.addAndMakeVisible(avgPlay);avgColourButton.setTitle("Color AVG");avgColourButton.canToggle=false;avgColourButton.setTooltip("Clic: seleccionar AVG sin datos. Doble clic: editar color.");avgColourButton.openSettings=[this]{source.setSelectedId(5,sendNotificationSync);editAvgColour();};avgPlay.getProperties().set("transportIcon","play");commands.bind(avgPlay,969,true);reflectAvgColour();addAndMakeVisible(alignmentFold);
 commands.bind(loopOptions,970,true);loopOptions.getProperties().set("commandGlyph",true);addAndMakeVisible(loopOptions);measurementButtons[5]->getProperties().set("utilityIcon","timer");measurementButtons[5]->getProperties().set("commandGlyph",true);measurementButtons[26]->getProperties().set("utilityIcon","config");measurementButtons[26]->getProperties().set("commandGlyph",true);for(int id:{9,11})measurementButtons[(size_t)id]->getProperties().set("hasOptions",true);for(int id:{10,12})measurementButtons[(size_t)id]->getProperties().set("commandGlyph",true);
 for(int n:{4,7,8,13,14,15,16,17,18})addAndMakeVisible(*measurementButtons[(size_t)n]);addAndMakeVisible(plotChoice);
 for(auto pair:std::initializer_list<std::pair<int,const char*>>{{20,"TF"},{0,"RTA"},{21,"IR"},{26,"Audio / E-S"},{2,"Medidores"}}){measurementButtons[(size_t)pair.first]->getProperties().set("commandGlyph",true);measurementButtons[(size_t)pair.first]->setButtonText(pair.second);}
 commands.bind(closeLeft,405);commands.bind(closeRight,406);commands.bind(restoreRight,406);for(auto* b:{&closeLeft,&closeRight,&restoreRight}){b->getProperties().set("commandGlyph",true);addAndMakeVisible(b);}
 addAndMakeVisible(leftDivider);addAndMakeVisible(rightDivider);leftDivider.moved=[this](int x){leftWidth=jlimit(WorkspaceViewState::leftMin,WorkspaceViewState::leftMax,x-8);resized();repaint();persistWorkspace();};rightDivider.moved=[this](int x){rightWidth=jlimit(WorkspaceViewState::rightMin,WorkspaceViewState::rightMax,getWidth()-x-8);resized();repaint();persistWorkspace();};
 registerKeys(*this);setSize(1440,960);select(0);
 if(projectFile.existsAsFile())load(false);
 if(auto state=WorkspaceViewState::read(projectFile.getParentDirectory().getChildFile("vista-trabajo.json"))){leftWidth=state->leftWidth;rightWidth=state->rightWidth;leftVisible=state->leftVisible;rightVisible=state->rightVisible;inputMeterSettings=state->meters;avgColour=Colour::fromString(state->avgColour);}reflectAvgColour();reflectInputMeters();
 if(auto stored=InputRoutePreferences::read(projectFile.getParentDirectory().getChildFile("entradas-niveles.json"))){for(size_t k=0;k<4;++k){deviceKeys[k]=String::fromUTF8(stored->routes[k].device.c_str());micInputs[k]=stored->routes[k].mic;refInputs[k]=stored->routes[k].ref;}if(stored->outputStored){outputKey=stored->outputDevice;outputChannel=stored->outputChannel;}inputEngine.setPinkLevelDb(stored->pinkDb);inputEngine.setPinkBand(stored->pinkHighPass,stored->pinkLowPass);inputEngine.setSweepLevelDb(stored->sweepDb);inputEngine.setSweep(stored->sweepStart,stored->sweepEnd,stored->sweepSeconds);syncNames();}libraryPanel.refresh();refreshLibrary();if(auto graph=GraphViewState::read(graphViewFile()))restoreGraphChoices(*graph);if(auto tf=TFDisplayState::read(tfSettingsFile())){tfSettings=*tf;reflectTF();}else tfSettings.linkPhaseMag=linkSmooth.getToggleState();resized();repaint();
 }
 void changeListenerCallback(ChangeBroadcaster*) override {audioEndpoints=discoverAudioEndpoints();if(!inputEngine.reconcile(audioEndpoints))stopInputs(inputEngine.lastError());syncNames();refreshMeterSources();if(settingsDrawer&&settingsDrawer->isVisible()){auto key=drawerKey;closeCalibration();settingsDrawer->setVisible(false);if(key=="audio")audioSettings();else if(key.startsWith("TF "))engineSettings(key.substring(3).getIntValue()-1);}if(!inputEngine.running()){inputEngine.setReferenceGainDb(0);loopCalMessage.clear();status="Dispositivos actualizados. Entradas cerradas; pulsa Play para abrirlas.";}repaint();}
 ~Workstation() override {++activation;stopTimer();inputEngine.stop();settingsDrawer.reset();graphWindow.reset();meters.reset();meterState.unlisten(meterListener);dockMeterState.unlisten(dockListener);deviceWatcher.removeChangeListener(this);tfContent.removeMouseListener(this);setLookAndFeel(nullptr);}
 std::array<FractionalSmoothing,4> smoothingCache;
 std::array<double,4> cachedDelay{{-1.e30,-1.e30,-1.e30,-1.e30}};
 std::complex<double> cursorResponse(double f,int index,int type,int panel=-1){
 auto sample=[&](double freq){double u=std::log(freq/20.)/std::log(1000.);return response(freq,false)*std::polar(std::pow(10.,(.65*index*std::sin(u*8+index))/20),.12*index*std::cos(u*7)+2*MathConstants<double>::pi*freq*micDelay[(size_t)index].getValue()/1000.);};
 int id=plotSmooth[(size_t)((panel>0?panel:type)-1)].getSelectedId();if(id<=1)return sample(f);
 auto k=(size_t)index;double currentDelay=micDelay[k].getValue();
 if(std::abs(cachedDelay[k]-currentDelay)>1.e-9){smoothingCache[k].rebuild(sample);cachedDelay[k]=currentDelay;}
 const double divs[]={48,24,12,6,3,1};return smoothingCache[k].at(f,divs[jlimit(0,5,id-2)],type==1);
 }
 // All-visible resolves the selected visible TF, otherwise the last visible TF.
 // An explicitly selected hidden TF has no readable value. AVG is not a TF estimator.
 int frontTrace(int type) const {
  std::array<bool,4> visible{};for(size_t i=0;i<4;++i)visible[i]=traceSwitch[i].getToggleState();
  int choice=plotMic[(size_t)type].getSelectedId();return PlotPresentation::frontTrace(choice==7?1:choice,source.getSelectedId()-1,visible);
 }
 bool validTF(double u,int index) const {
  return PlotPresentation::validTF(u,index,tfSettings.coherence(demoCoherence(std::log(tfSettings.frequency(u)/20.)/std::log(1000.),index))*100,coherenceThreshold.getValue());
 }
 std::optional<double> plotValue(double u,int index,int type){
  if(index<0)return std::nullopt;
  if(index>=4||!tfEngines[(size_t)index].ready())return std::nullopt;auto& engine=tfEngines[(size_t)index];
  if(type==0){double timeMs=-40.+80.*u;auto value=engine.impulseAt(timeMs);if(!value)return {};double magnitude=std::abs(*value);if(irScale==0)return std::clamp(*value*4.,-1.,1.);return std::clamp((20.*std::log10(std::max(magnitude,1e-9))+90.)/90.,0.,1.);}
  int smooth=plotSmooth[(size_t)(type-1)].getSelectedId();const double fractions[]={0.,1./48.,1./24.,1./12.,1./6.,1./3.,1.};auto p=engine.smoothedAt(tfSettings.frequency(u),fractions[jlimit(0,6,smooth-1)]);if(!p||!tfSettings.passes(p->coherence))return std::nullopt;return tfSettings.normalize(displayType(type)==1?p->phaseDegrees:p->magnitudeDb,displayType(type));
 }
 void updateCursorReadout(){
  cursorReadout.parts.clear();auto add=[this](String label,String value,Colour colour){cursorReadout.parts.push_back({label,value,colour});};
  auto pink=Colour(0xfff03baf),red=Colour(0xffff4040);
  if(cursorPlot<0){cursorReadout.setTooltip({});add("Frecuencia","-- Hz",Palette::cyan);add("Magnitud","-- dB",Palette::green);add("Fase",String::fromUTF8("--°"),pink);add("Coherencia","-- %",red);return;}
  if(cursorPlot==0){double timeMs=-40.+80.*cursorU;int selected=frontTrace(0);auto value=selected>=0&&selected<4?tfEngines[(size_t)selected].impulseAt(timeMs):std::nullopt;add("Tiempo",value?String(timeMs,2)+" ms":String("-- ms"),Palette::cyan);add(String("IR ")+String(irScale==0?"Lin":irScale==1?"Log":"ETC"),value?String(irScale==0?*value:20.*std::log10(std::max(std::abs(*value),1e-9)),irScale==0?4:2)+String(irScale==0?" Lin":" dB"):String("--"),Palette::green);add("Fase",String::fromUTF8("--°"),pink);add("Coherencia","-- %",red);cursorReadout.setTooltip("Respuesta al impulso real de la TF seleccionada / referencia temporal aplicada");return;}
  if(frontTrace(1)<0&&frontTrace(2)<0&&capturedFront(1).isEmpty()&&capturedFront(2).isEmpty()){add("Frecuencia","-- Hz",Palette::cyan);add("Magnitud","-- dB",Palette::green);add("Fase",String::fromUTF8("--°"),pink);add("Coherencia","-- %",red);cursorReadout.setTooltip("Sin trazos visibles en los selectores de fase/magnitud");return;}
  double frequency=tfSettings.frequency(cursorU);add("Frecuencia",frequency>=1000?String(frequency/1000.,2)+" kHz":String(frequency,1)+" Hz",Palette::cyan);String full=String(frequency,1)+" Hz";
  std::array<String,3> readings,coherenceReadings;
  for(int panel:{2,1}){int kind=displayType(panel);auto v=panelValue(cursorU,panel);auto coh=panelCoherence(cursorU,panel);auto k=(size_t)panel;readings[k]=v?String(tfSettings.physical(*v,kind),kind==2?2:1)+String(kind==2?String(" dB"):String::fromUTF8("°")):String("--");coherenceReadings[k]=coh?String(tfSettings.coherence(*coh)*100,1)+"%":String("--");full+="\nP"+String(panel)+" "+panelName(panel)+": "+readings[k]+(tfSettings.coherenceSquared?String::fromUTF8(" Coh² "):String(" Coh "))+coherenceReadings[k];}
  auto panelFor=[this](int kind){if(displayType(cursorPlot)==kind)return cursorPlot;return displayType(1)==kind?1:displayType(2)==kind?2:0;};int mag=panelFor(2),phase=panelFor(1);
  add("Magnitud",mag?readings[(size_t)mag]:"-- dB",Palette::green);add("Fase",phase?readings[(size_t)phase]:String::fromUTF8("--°"),pink);
  bool different=capturedFront(1)!=capturedFront(2)||frontTrace(1)!=frontTrace(2)||coherenceReadings[1]!=coherenceReadings[2];
  if(different)add("Coh P2/P1",coherenceReadings[2]+String::fromUTF8(" · ")+coherenceReadings[1],red);else add("Coherencia",coherenceReadings[2],red);
  full+="\nLectura principal: magnitud P"+String(mag)+", fase P"+String(phase)+(different?String("; coherencias P2 / P1"):String("; coherencia comun"));cursorReadout.setTooltip(full);
 }
 void mouseExit(const MouseEvent& event) override {
  auto point=event.getEventRelativeTo(this).getPosition();
  for(int t=0;t<3;++t)if((measurementView==0||t==0)&&(t!=0||plotChoice.getSelectedId()!=2||measurementView==1)&&plotArea(plotBounds(t)).contains(point))return;
  cursorPlot=-1;plotCursorVisible.fill(false);updateCursorReadout();repaint();
 }
 void mouseMove(const MouseEvent& event) override{
 if(mode!=0)return;auto point=event.getEventRelativeTo(this).getPosition();
 for(int t=0;t<3;++t){if(measurementView==1&&t>0)continue;if(t==0&&plotChoice.getSelectedId()==2&&measurementView==0)continue;auto p=plotArea(plotBounds(t));if(!p.contains(point))continue;
 cursorPlot=t;cursorY=((double)point.y-p.getY())/p.getHeight();cursorU=PlotPresentation::coordinate(((double)point.x-p.getX())/p.getWidth(),plotPanX[(size_t)t],plotZoomX[(size_t)t]);
 if(t==0)plotCursorVisible.fill(false);else plotCursorVisible[0]=false;
 plotCursorU[(size_t)t]=cursorU;plotCursorVisible[(size_t)t]=true;
 // TF cursors share frequency, but each horizontal arm follows its own trace value.
 if(t>0){plotCursorU[1]=plotCursorU[2]=cursorU;plotCursorVisible[1]=plotCursorVisible[2]=true;}
 updateCursorReadout();repaint();return;
 }
 if(cursorPlot!=-1){cursorPlot=-1;plotCursorVisible.fill(false);cursorReadout.setText("-- Hz       -- dB       -- deg",dontSendNotification);repaint();}
 }
 void mouseDown(const MouseEvent& event) override {
 if(event.originalComponent==this&&mode!=2){for(int t=0;t<3;++t)if((measurementView==0||t==0)&&(t!=0||plotChoice.getSelectedId()!=2||measurementView==1)&&activeGraphBounds(t).contains(event.getPosition())){activePlot=t;grabKeyboardFocus();status="Grafica seleccionada / flechas izquierda/derecha: zoom X / + y -: zoom Y / doble clic: centrar";repaint();return;}}
 auto* origin=event.originalComponent;
 if(origin!=&tfContent&&!tfContent.isParentOf(origin))return;
 auto point=event.getEventRelativeTo(&tfContent).getPosition();
 if(point.x>=0&&point.x<tfContent.getWidth()&&point.y>=CompactLayout::avgTop&&point.y<CompactLayout::avgTop+24){source.setSelectedId(5,sendNotificationSync);status="AVG seleccionada / sin datos; estimador, Play, Find y Track pendientes";tfContent.repaint();repaint();if(origin==&tfContent&&getPeer()!=nullptr)grabKeyboardFocus();return;}
 int index=point.y/CompactLayout::tfRow;
 if(point.x<0||point.x>=tfContent.getWidth()||point.y<0||index>=4||point.y%CompactLayout::tfRow>=CompactLayout::tfCard)return;
 source.setSelectedId(index+1,sendNotificationSync);
 status=micNames[(size_t)index].getText()+String::fromUTF8(" seleccionada · L / F: tiempo individual · Espacio: captura");
 tfContent.repaint();repaint();
 if(origin==&tfContent&&getPeer()!=nullptr)grabKeyboardFocus();
 }

 void mouseDoubleClick(const MouseEvent& event) override {
 if(event.originalComponent==this&&mode!=2){for(int t=0;t<3;++t)if((measurementView==0||t==0)&&(t!=0||plotChoice.getSelectedId()!=2||measurementView==1)&&activeGraphBounds(t).contains(event.getPosition())){plotPanX[(size_t)t]=plotPanY[(size_t)t]=0;plotZoomX[(size_t)t]=plotZoomY[(size_t)t]=1.;activePlot=t;repaint();return;}}
 auto* origin=event.originalComponent;
 if(origin!=&tfContent&&!tfContent.isParentOf(origin))return;
 auto point=event.getEventRelativeTo(&tfContent).getPosition();
 if(point.x>=0&&point.x<tfContent.getWidth()&&point.y>=CompactLayout::avgTop&&point.y<CompactLayout::avgTop+24){if(origin==&tfContent||origin==&avgReserve){source.setSelectedId(5,sendNotificationSync);editAvgColour();}return;}
 int index=point.y/CompactLayout::tfRow;
 if(point.x<0||point.x>=tfContent.getWidth()||point.y<0||index>=4||point.y%CompactLayout::tfRow>=CompactLayout::tfCard)return;
 // The colour button already handles its own double click; inputs retain normal editing.
 if(origin!=&tfContent&&origin!=&micNames[(size_t)index])return;
 source.setSelectedId(index+1,sendNotificationSync);commands.invoke(202);
 }
 void registerKeys(Component& c){c.addKeyListener(this);for(auto* child:c.getChildren())registerKeys(*child);}
 bool keyPressed(const KeyPress& k,Component*) override{return keyPressed(k);}
 bool keyPressed(const KeyPress& k) override {
 auto mods=k.getModifiers();auto* focus=Component::getCurrentlyFocusedComponent();
 bool command=mods.isCommandDown()||mods.isCtrlDown();
 // Only the plot/workspace owns application shortcuts. Editors, combos and sliders keep theirs.
 if(focus!=this){meterSequence.reset();return false;}
 int key=k.getKeyCode();char c=key>=0&&key<128?(char)CharacterFunctions::toLowerCase((juce_wchar)key):0;
 if(MyaCommands::isPlainMeterKey(key,mods.isCommandDown(),mods.isCtrlDown(),mods.isAltDown(),mods.isShiftDown())){
  int id=meterSequence.press(Time::getMillisecondCounterHiRes(),true);if(id)commands.invoke(id);return true;
 }
 meterSequence.reset();int id=0;
 if(key==KeyPress::escapeKey&&!command&&!mods.isAltDown()&&!mods.isShiftDown())id=602;
 else if(mode!=2&&!command&&!mods.isAltDown()&&!mods.isShiftDown()&&key==KeyPress::leftKey)id=856;
 else if(mode!=2&&!command&&!mods.isAltDown()&&!mods.isShiftDown()&&key==KeyPress::rightKey)id=857;
 else if(mode!=2&&!command&&!mods.isAltDown()&&key==KeyPress::upKey)id=858;
 else if(mode!=2&&!command&&!mods.isAltDown()&&key==KeyPress::downKey)id=859;
 else{auto character=k.getTextCharacter();if(character=='+'||character=='-')c=(char)character;
  id=MyaCommands::shortcut(c,command,mods.isAltDown(),mods.isShiftDown(),mode!=2,false);
 }
 if(!id)return false;
 fineCommandStep=mods.isShiftDown();bool ran=commands.invoke(id);fineCommandStep=false;
 if(!ran){if(auto* definition=MyaCommands::find(id))status=commands.details(*definition,MyaCommands::state(*definition,commands.current()));repaint();}
 return true;
 }
 void resized() override {if(buttons.size()<19)return;int excess=jmax(0,680-centerWidth());if(rightVisible){int reduction=jmin(excess,rightWidth-WorkspaceViewState::rightMin);rightWidth-=reduction;excess-=reduction;}if(leftVisible)leftWidth-=jmin(excess,leftWidth-WorkspaceViewState::leftMin);tracesPage.setVisible(leftVisible);metersPage.setVisible(leftVisible);int lw=leftWidth-8;int column=CompactLayout::libraryColumnWidth(lw),tabHeight=CompactLayout::libraryTabHeight;tracesPage.setBounds(12,91,column,tabHeight);metersPage.setBounds(14+column,91,column,tabHeight);libraryPanel.setVisible(leftVisible);libraryPanel.setHeaderOnly(dockMeterState.state().sidebarMeters);libraryPanel.setBounds(12,118,lw,dockMeterState.state().sidebarMeters?24:getHeight()-290);dockMeters.setVisible(leftVisible&&dockMeterState.state().sidebarMeters);dockMeters.setBounds(12,147,lw,getHeight()-319);restoreLibraryButton.setVisible(!leftVisible);restoreLibraryButton.setBounds(0,92,18,24);
 closeLeft.setVisible(leftVisible);closeLeft.setBounds(leftWidth+4,83,18,22);closeLeft.setButtonText("<");closeRight.setVisible(rightVisible);closeRight.setBounds(getWidth()-rightWidth-24,83,18,22);closeRight.setButtonText(">");restoreRight.setVisible(!rightVisible);restoreRight.setBounds(getWidth()-18,92,18,24);restoreRight.setButtonText("<");leftDivider.setVisible(leftVisible);leftDivider.setBounds(leftWidth+8,112,6,getHeight()-150);rightDivider.setVisible(rightVisible);rightDivider.setBounds(getWidth()-rightWidth-16,112,6,getHeight()-150);
 for(size_t i=0;i<aiButtons.size();++i){aiButtons[i].setVisible(leftVisible);int row=(int)i/2;aiButtons[i].setBounds(12+(int(i)%2)*((lw+2)/2),getHeight()-162+row*28,i==6?lw:(lw-2)/2,24);}sumButton.setVisible(mode==0);sumButton.setBounds(centerX()+centerWidth()-90,getHeight()-61,82,22);
 int navWidth=jmin(660,getWidth()-24),navCell=(navWidth-30)/4;navigationOverlay.setBounds((getWidth()-navWidth)/2,82,navWidth,113);navigationTab.setBounds(getWidth()/2-17,66,34,14);String measurementName=measurementView==1?"Medicion IR":measurementView==2?"RTA":measurementView==3?"Espectrograma":"Medicion TF";navigationTab.setTitle(String("Navegacion / ")+String(mode==0?measurementName:mode==1?"DSP / Ajuste":mode==2?"Mya Vision 3D":"Laboratorio"));navigationTab.setTooltip(navigationTab.getTitle()+" / clic para abrir o cerrar accesos");for(int i=0;i<8;++i)buttons[(size_t)i]->setBounds(6+(i%4)*(navCell+6),6+(i/4)*34,navCell,28);navigationMenus.setBounds(navWidth-216,78,210,26);navigationTab.toFront(false);if(navigationOverlay.isVisible())navigationOverlay.toFront(false);int left=14;for(int i=8;i<12;++i)buttons[(size_t)i]->setBounds(left+(i-8)%2*83,getHeight()-170+(i-8)/2*36,78,30);
 int right=getWidth()-rightWidth;gainLabel.setBounds(right,105,200,22);gain.setBounds(right,128,200,30);delayLabel.setBounds(right,169,200,22);delay.setBounds(right,192,200,30);
 buttons[12]->setBounds(right,234,98,30);buttons[13]->setBounds(right+103,234,97,30);buttons[14]->setBounds(right,276,200,30);buttons[15]->setBounds(right,312,200,30);buttons[16]->setBounds(right,348,200,30);buttons[17]->setBounds(right,384,200,30);buttons[18]->setBounds(right,420,200,30);
 for(int i=8;i<12;++i)buttons[(size_t)i]->setVisible(false);
 for(int i=12;i<19;++i)buttons[(size_t)i]->setVisible(rightVisible);
 gain.setVisible(rightVisible);gainLabel.setVisible(rightVisible);delay.setVisible(rightVisible);delayLabel.setVisible(rightVisible);
 source.setBounds(centerX()+8,91,jmax(120,centerWidth()-250),28);source.setVisible(mode!=0);cursorReadout.setVisible(mode==0);cursorReadout.setBounds(centerX()+8,87,jmax(120,centerWidth()-16),24);
 linkSmooth.setVisible(false);
 for(int i=0;i<2;++i){auto r=plotBounds(i);plotDividers[(size_t)i].setVisible(mode==0&&measurementView==0&&(i!=0||plotChoice.getSelectedId()!=2));plotDividers[(size_t)i].setBounds(r.getX(),r.getBottom(),r.getWidth(),8);}
 for(int t=0;t<3;++t){auto r=plotBounds(t);auto& c=plotMic[(size_t)t];c.setVisible(mode==0&&((measurementView==0&&(t!=0||plotChoice.getSelectedId()!=2))||(measurementView==1&&t==0)));c.setBounds(r.getRight()-115,r.getY()+2,110,18);}
 int magnitudePanel=plotTypes[1]==2?2:plotTypes[0]==2?1:-1;auto magArea=plotBounds(magnitudePanel>0?magnitudePanel:2);
 coherenceThreshold.setVisible(false);coherenceThreshold.setBounds(magArea.getRight()-12,magArea.getY()+26,10,jmax(20,roundToInt((magArea.getHeight()-48)*tfSettings.coherenceHeight)));showCoherence.setVisible(mode==0&&measurementView==0&&magnitudePanel>0);showCoherence.setBounds(magArea.getX()+175,magArea.getY()+2,95,18);
 impulseScale.setVisible(mode==0&&measurementView<=1&&(plotChoice.getSelectedId()!=2||measurementView==1));auto irArea=plotBounds(0);impulseScale.setBounds(irArea.getX()+5,irArea.getY()+2,95,18);
 for(int t=0;t<2;++t){auto area=plotBounds(t+1);for(int kind:{1,2}){auto* c=MyaCommands::find((t==0?940:950)+(kind==1?1:0));plotTypeChoice[(size_t)t].changeItemText(kind,commands.title(*c));}plotTypeChoice[(size_t)t].setVisible(mode==0&&measurementView==0);plotTypeChoice[(size_t)t].setBounds(area.getX()+5,area.getY()+2,140,18);plotSmooth[(size_t)t].setTitle(String(displayType(t+1)==1?"Fase / Phase":"Magnitud / Magnitude")+juce::String::fromUTF8(" · suavizado / smoothing"));}
 for(int t=0;t<3;++t){auto r=activeGraphBounds(t);graphGear[(size_t)t].setVisible(mode!=2&&(mode!=0||(measurementView==0&&(t!=0||plotChoice.getSelectedId()!=2))||(measurementView==1&&t==0)));graphGear[(size_t)t].setBounds(r.getRight()-20,r.getBottom()-18,16,16);}
 {bool visible=mode==0&&measurementView>=2;int x=centerX()+62,y=120;for(auto* combo:{&spectrumFFT,&spectrumSpeed,&spectrumWindow,&spectrumAverage}){combo->setVisible(visible);combo->setBounds(x,y,130,22);x+=136;}}
 for(int t=0;t<2;++t){auto r=plotBounds(t+1);plotSmooth[(size_t)t].setVisible(mode==0&&measurementView==0);auto box=CompactLayout::smoothing(r.getWidth());plotSmooth[(size_t)t].setBounds(r.getX()+box.x,r.getY()+box.y,box.w,box.h);}

 const bool measuring=mode==0&&rightVisible;
 for(auto& button:measurementButtons){button->setVisible(false);button->setTransform({});}
 for(auto* c:std::initializer_list<Component*>{&meterInput,&meterWeight,&selectedDelay,&phasePreview})c->setVisible(false);
 tfViewport.setVisible(measuring);averageChoice.setVisible(measuring);generatorOutput.setVisible(measuring);generatorLevel.setVisible(measuring);
 if(mode==0){for(int i=12;i<19;++i)buttons[(size_t)i]->setVisible(false);gain.setVisible(false);gainLabel.setVisible(false);delay.setVisible(false);delayLabel.setVisible(false);}
 int rx=getWidth()-rightWidth,rw=rightWidth-16;
 auto mb=[&](int id,int bx,int y,int w,int h=26){auto& button=*measurementButtons[(size_t)id];button.setVisible(measuring);button.setBounds(bx,y,w,h);};
 int meterOffset=meterState.state().mainVisible?96:0;principalMeter.setVisible(measuring&&meterState.state().mainVisible);principalMeter.setBounds(rx,88,rw,96);
 int tab=(rw-8)/3;mb(20,rx,94+meterOffset,tab);mb(0,rx+tab+4,94+meterOffset,tab);mb(21,rx+2*(tab+4),94+meterOffset,rw-2*(tab+4));
 averageChoice.setTransform({});averageChoice.setBounds(rx,128+meterOffset,rw,22);averageChoice.setTooltip("Profundidad del promedio temporal TF en directo");
 int transport=(rw-8)/3;mb(23,rx,154+meterOffset,transport);mb(24,rx+transport+4,154+meterOffset,transport);mb(25,rx+2*(transport+4),154+meterOffset,rw-2*(transport+4));
 mb(19,rx,184+meterOffset,28);mb(5,rx+32,184+meterOffset,28);mb(26,rx+64,184+meterOffset,28);mb(4,rx+96,184+meterOffset,rw-128);measurementButtons[4]->getProperties().set("commandGlyph",true);measurementButtons[4]->setButtonText("Int.Loop");loopOptions.setVisible(measuring);loopOptions.setBounds(rx+rw-28,184+meterOffset,28,26);
 int gen=CompactLayout::generatorTop(getHeight());
 int footerTop=CompactLayout::actionTop(getHeight(),alignmentExpanded),alignmentY=footerTop+32;
 tfViewport.setTransform({});tfViewport.setBounds(rx,CompactLayout::tfStart(meterState.state().mainVisible),rw,CompactLayout::tfViewportHeight(getHeight(),alignmentExpanded,meterState.state().mainVisible));
 int contentHeight=CompactLayout::tfContentHeight,cw=rw-(contentHeight>tfViewport.getHeight()?10:0);tfContent.setSize(cw,contentHeight);
 avgColourButton.setBounds(0,CompactLayout::avgTop,24,24);avgReserve.setBounds(26,CompactLayout::avgTop,cw-60,24);avgPlay.setBounds(cw-29,CompactLayout::avgTop,27,24);
 int half=(rw-4)/2;mb(13,rx,footerTop,half);mb(14,rx+half+4,footerTop,rw-half-4);
 alignmentFold.setVisible(measuring);alignmentFold.setBounds(rx,alignmentY,rw,26);alignmentFold.setButtonText(String(alignmentExpanded?"v ":"> ")+String::fromUTF8("Alineación avanzada"));
 const int advanced[]={7,8,15,16,17,18};for(int j=0;j<6;++j){int id=advanced[j];mb(id,rx+(j%2)*(half+4),alignmentY+32+(j/2)*26,half,22);measurementButtons[(size_t)id]->setVisible(measuring&&alignmentExpanded);}
 plotChoice.setTransform({});plotChoice.setBounds(rx+half+4,alignmentY+32+3*26,rw-half-4,22);plotChoice.setVisible(measuring&&measurementView==0&&alignmentExpanded);
 generatorOutput.setTransform({});generatorLevel.setTransform({});
 mb(9,rx,CompactLayout::signalTop(getHeight()),(rw-4)/2,28);mb(11,rx+(rw+4)/2,CompactLayout::signalTop(getHeight()),(rw-4)/2,28);
 mb(10,rx+(rw-4)/2-24,CompactLayout::signalTop(getHeight()),24,28);mb(12,rx+rw-24,CompactLayout::signalTop(getHeight()),24,28);measurementButtons[10]->setButtonText("...");measurementButtons[12]->setButtonText("...");
 generatorOutput.setBounds(rx,gen+20,rw-66,22);generatorLevel.setBounds(rx+rw-62,gen+20,62,22);mb(28,rx,gen+44,rw,22);
 for(int i=0;i<4;++i){auto k=(size_t)i;int y=i*CompactLayout::tfRow,w=cw;micNames[k].setBounds(25,y+1,w-88,21);traceSwitch[k].setBounds(0,y+1,24,22);colourEdit[k].setBounds(w-59,y+2,27,20);tfActions[k][3].setBounds(w-29,y+2,27,20);
 inputMeters[k].setBounds(4,y+24,w-8,48);micDelay[k].setSliderStyle(Slider::LinearHorizontal);micDelay[k].setTextBoxStyle(Slider::TextBoxLeft,false,w-82,18);micDelay[k].setBounds(4,y+76,w-78,18);tfActions[k][0].setBounds(w-70,y+76,38,18);tfActions[k][0].getProperties().set("commandGlyph",true);tfActions[k][0].setButtonText("Find");tfActions[k][1].setVisible(true);tfActions[k][1].setBounds(w-28,y+73,25,24);tfActions[k][2].setVisible(false);}
 auto compactLeft=AffineTransform::scale(.82f,1.f).translated(2.5f,0);for(int i=8;i<12;++i)buttons[(size_t)i]->setTransform(compactLeft);
 const bool sceneMode=mode==2&&rightVisible;stacks.setVisible(sceneMode);boxes.setVisible(sceneMode);spacing.setVisible(sceneMode);rotation.setVisible(sceneMode);stacksLabel.setVisible(sceneMode);boxesLabel.setVisible(sceneMode);spacingLabel.setVisible(sceneMode);rotationLabel.setVisible(sceneMode);
 Slider* ss[]={&stacks,&boxes,&spacing,&rotation};Label* ll[]={&stacksLabel,&boxesLabel,&spacingLabel,&rotationLabel};for(int i=0;i<4;++i){ll[i]->setBounds(right,473+i*64,200,23);ss[i]->setBounds(right,496+i*64,200,28);}positionDrawer();positionAnchored();}
 void text(Graphics& g,String s,Rectangle<int> r,float size=12,Colour c=Palette::ink){g.setColour(c);g.setFont(size);g.drawFittedText(s,r,Justification::centredLeft,1);}
 void panel(Graphics& g,Rectangle<int> r){g.setColour(Palette::panel);g.fillRoundedRectangle(r.toFloat(),6);g.setColour(Palette::line);g.drawRoundedRectangle(r.toFloat(),6,1.5f);}
 std::complex<double> response(double f,bool adjusted){auto h=demoTransfer(f);if(adjusted&&!disabled)h*=std::polar(std::pow(10.,gain.getValue()/20),-2*MathConstants<double>::pi*f*delay.getValue()/1000)*(polarity?-1.:1.);return h;}
 void graph(Graphics& g,Rectangle<int> r,String label,int type){
 g.setColour(Palette::plot);g.fillRect(r);int kind=mode==0?displayType(type):type;int amplitudeMode=irScale;
 double panX=mode!=2?plotPanX[(size_t)type]:0,panY=mode!=2?plotPanY[(size_t)type]:0,zoomX=mode!=2?plotZoomX[(size_t)type]:1.,zoomY=mode!=2?plotZoomY[(size_t)type]:1.;
 if(mode!=2&&type==activePlot){g.setColour(Palette::cyan);g.drawRect(r,1);}auto p=plotArea(r);
 g.setColour(Colour(0xff2c3135));g.drawRect(p,1);
 int divisions=kind==0?6:std::max(1,(int)std::ceil(tfSettings.span(kind)/(kind==1?tfSettings.phaseGrid:tfSettings.magGrid)));
 for(int i=0;i<=divisions;++i){if(kind==0&&amplitudeMode==0&&i%3!=0)continue;double axisValue=kind==0?0:tfSettings.maximum(kind)-i*(kind==1?tfSettings.phaseGrid:tfSettings.magGrid);double fraction=kind==0?i/(double)divisions:(tfSettings.maximum(kind)-axisValue)/tfSettings.span(kind);if(fraction>1.000001)continue;float yy=(float)(p.getY()+fraction*p.getHeight());g.setColour(Colour(0xff2c3135));g.drawHorizontalLine(roundToInt(yy),(float)p.getX(),(float)p.getRight());
  double value=kind==0?(amplitudeMode==0?(1.-i/3.)/zoomY+panY*2.:-60.+(60.-i*20.)/zoomY+panY*120.):tfSettings.physical(tfSettings.normalize(axisValue,kind)/zoomY+panY,kind);
  text(g,String(value,2).trimCharactersAtEnd("0").trimCharactersAtEnd(".")+(kind==1?String::fromUTF8("°"):String()),{r.getX()+2,roundToInt(yy)-6,32,12},10,Palette::blue);
 }
 auto tick=[&](double coordinate,String name){double position=PlotPresentation::screen(coordinate,panX,zoomX);if(position<0||position>1)return;int xx=roundToInt(p.getX()+position*p.getWidth());g.setColour(Colour(0xff2c3135));g.drawVerticalLine(xx,(float)p.getY(),(float)p.getBottom());text(g,name,{jlimit(r.getX()+1,r.getRight()-64,xx-18),p.getBottom()+3,40,14},10,Palette::blue);};
 if(kind==0){double first=ImpulseDisplay::timeMs(PlotPresentation::coordinate(0,panX,zoomX)),last=ImpulseDisplay::timeMs(PlotPresentation::coordinate(1,panX,zoomX));for(int t=(int)std::ceil(first/5.)*5;t<=last;t+=5)tick(t/ImpulseDisplay::spanMs+.5,String(t));}
 else{std::vector<double> ticks;if(tfSettings.frequencyGrid==1)ticks.assign(PlotPresentation::frequencyTicks.begin(),PlotPresentation::frequencyTicks.end());else if(tfSettings.frequencyGrid==2)ticks={20,50,100,200,500,1000,2000,5000,10000,20000};else if(tfSettings.frequencyGrid==3)ticks={20,25,31.5,40,50,63,80,100,125,160,200,250,315,400,500,630,800,1000,1250,1600,2000,2500,3150,4000,5000,6300,8000,10000,12500,16000,20000};else ticks={20,2000,4000,6000,8000,10000,12000,14000,16000,18000,20000};int index=0;for(double f:ticks){String name=f>=1000?String(f/1000.,2).trimCharactersAtEnd("0").trimCharactersAtEnd(".")+"k":std::abs(f-31.5)<.001?String("31,5"):String(f,0);if(tfSettings.frequencyGrid==3&&(index++%3)!=2)name.clear();tick(tfSettings.coordinate(f),name);}}

 g.saveState();g.reduceClipRegion(p);if(mode==0)drawSaved(g,p,type,false,panX,panY,zoomX,zoomY);auto order=PlotPresentation::drawOrder(mode==0?frontTrace(type):-1);for(int layer=0;layer<(mode==0?4:(type==0?1:2));++layer){int s=mode==0?order[(size_t)layer]:layer;if(mode==0&&!tfInPanel(s,type))continue;Path path;double previous=0;bool gap=true;int points=jmax(700,p.getWidth()*2);
 for(int i=0;i<=points;++i){
  double u=PlotPresentation::coordinate(i/(double)points,panX,zoomX),val=0;
  if(mode==0){auto displayed=plotValue(u,s,type);if(!displayed){gap=true;continue;}val=*displayed;}
  else if(type==0){auto value=impulseDisplay.normalized(u,irScale);if(!value){gap=true;continue;}val=*value;}
  else{double f=tfSettings.frequency(u);if(u<0||u>1||!tfSettings.passes(demoCoherence(std::log(f/20.)/std::log(1000.),0))){gap=true;continue;}auto h=response(f,s==1&&(mode==1||mode==3));val=tfSettings.normalize(kind==1?std::arg(h)*180/MathConstants<double>::pi:20*std::log10(std::abs(h)),kind);}
  float xx=(float)(p.getX()+PlotPresentation::screen(u,panX,zoomX)*p.getWidth());
  float yy=(float)(p.getCentreY()-(val-panY)*zoomY*p.getHeight());
  if(gap||(kind==1&&std::abs(val-previous)>.5))path.startNewSubPath(xx,yy);else path.lineTo(xx,yy);
  gap=false;previous=val;
 }g.setColour(mode==0?traceSwitch[(size_t)s].colour:(s==0?Palette::cyan:Palette::green));g.strokePath(path,PathStrokeType((float)((mode==0?(s==frontTrace(type)&&capturedFront(type).isEmpty()):s==(type==0?0:1))?tfSettings.foregroundWidth:tfSettings.backgroundWidth)));}if(mode==0)drawSaved(g,p,type,true,panX,panY,zoomX,zoomY);if(mode!=2&&kind==2&&showCoherence.getToggleState()){
 Path cp;bool gap=true;int index=mode==0?frontTrace(type):0;auto savedFront=mode==0?capturedFront(type):String{};
 for(int i=0;i<=700;++i){double u=PlotPresentation::coordinate(i/700.,panX,zoomX);std::optional<double> coherence;
   if(u>=0&&u<=1){if(savedFront.isNotEmpty())coherence=savedTraces.coherence(savedFront,tfSettings.frequency(u));else if(index>=0&&index<4){auto point=tfEngines[(size_t)index].at(tfSettings.frequency(u));if(point)coherence=point->coherence;}}
  if(!coherence){gap=true;continue;}float x=(float)(p.getX()+i*p.getWidth()/700.),y=(float)(p.getY()+(1-tfSettings.coherence(*coherence))*p.getHeight()*tfSettings.coherenceHeight);
  if(gap)cp.startNewSubPath(x,y);else cp.lineTo(x,y);gap=false;
 }g.setColour(Colour(0xffff4040));g.strokePath(cp,PathStrokeType((float)(tfSettings.coherenceFollowTrace?tfSettings.foregroundWidth:tfSettings.coherenceWidth)));}

 if(mode==0&&plotCursorVisible[(size_t)type]){
 double cu=plotCursorU[(size_t)type];float cx=(float)(p.getX()+((cu-panX-.5)*zoomX+.5)*p.getWidth());
 auto cursorValue=panelValue(cu,type);
 if(cursorValue&&cx>=(float)p.getX()&&cx<=(float)p.getRight()){
 double value=*cursorValue;
 int cy=roundToInt(p.getCentreY()-(value-panY)*zoomY*p.getHeight());
 g.setColour(Palette::ink.withAlpha(.5f));g.drawVerticalLine(roundToInt(cx),(float)p.getY(),(float)p.getBottom());g.drawHorizontalLine(cy,(float)p.getX(),(float)p.getRight());
 }}g.restoreState();if(mode==0&&type>0){auto legend=captureLegend(type);for(int i=0;i<4;++i)if(tfInPanel(i,type)&&tfEngines[(size_t)i].ready())legend+=(legend.isNotEmpty()?" / ":"")+micNames[(size_t)i].getText()+" · M/R EN DIRECTO";if(legend.isNotEmpty())text(g,legend,{r.getX()+230,r.getY()+2,jmax(1,r.getWidth()-360),18},10,Palette::muted);plotMic[(size_t)type].setTooltip(legend+" / solo datos medidos y capturas reales");}if(mode==0&&kind==0){int selected=frontTrace(0);bool ready=selected>=0&&selected<4&&tfEngines[(size_t)selected].ready();text(g,ready?juce::String::fromUTF8("IR REAL · referencia temporal aplicada · ms"):juce::String::fromUTF8("IR · esperando TF real"),{r.getX()+110,r.getY()+2,jmax(1,r.getWidth()-240),20},11,ready?Palette::green:Palette::muted);}else if(mode!=0)text(g,label,{r.getX()+9,r.getY()+2,r.getWidth()-18,20},12,Palette::muted);}
 void spectrumGraph(Graphics& g,Rectangle<int> bounds,bool spectrogram){
  g.setColour(Palette::plot);g.fillRect(bounds);auto p=bounds.reduced(48,28);g.setColour(Colour(0xff26353e));for(double f:{31.5,63.,125.,250.,500.,1000.,2000.,4000.,8000.,16000.}){float x=(float)(p.getX()+std::log(f/20.)/std::log(1000.)*p.getWidth());g.drawVerticalLine(roundToInt(x),(float)p.getY(),(float)p.getBottom());text(g,f>=1000?String(f/1000.,f<10000?1:0)+"k":String(f,1),{roundToInt(x)-24,p.getBottom()+2,48,18},9,Palette::muted);}
  if(spectrogram){const auto& history=liveSpectrum.spectrogram();if(!history.empty()){int rows=std::min((int)history.size(),p.getHeight());for(int y=0;y<rows;++y){const auto& frame=history[history.size()-1-(size_t)y];for(int x=0;x<p.getWidth();++x){double f=20.*std::pow(1000.,x/(double)std::max(1,p.getWidth()-1));int k=(int)std::llround(f*liveSpectrum.configuration().fftSize/frame.sampleRate);double db=k>=0&&(size_t)k<frame.db.size()?frame.db[(size_t)k]:-120.;float v=(float)jlimit(0.,1.,(db+100.)/80.);Colour c=Colour::fromHSV((1.f-v)*.68f,.95f,v*.9f+.05f,1.f);g.setColour(c);g.fillRect(p.getX()+x,p.getBottom()-1-y,1,1);}}}text(g,"ESPECTROGRAMA REAL",{p.getX()+6,p.getY()+4,180,18},11,Palette::cyan);}
  else if(liveSpectrum.ready()){Path line;bool started=false;const auto& frame=liveSpectrum.frame();for(int x=0;x<p.getWidth();++x){double f=20.*std::pow(1000.,x/(double)std::max(1,p.getWidth()-1));int k=(int)std::llround(f*liveSpectrum.configuration().fftSize/frame.sampleRate);if(k<0||(size_t)k>=frame.db.size())continue;double db=jlimit(-120.,20.,frame.db[(size_t)k]);float y=(float)(p.getBottom()-(db+120.)/140.*p.getHeight());if(!started){line.startNewSubPath((float)p.getX()+x,y);started=true;}else line.lineTo((float)p.getX()+x,y);}g.setColour(Palette::green);g.strokePath(line,PathStrokeType(1.5f));text(g,"RTA REAL",{p.getX()+6,p.getY()+4,100,18},11,Palette::green);}else text(g,"Esperando audio de la TF activa",p,13,Palette::muted);
 }
 void scene(Graphics& g,Rectangle<int> r){g.setColour(Palette::plot);g.fillRect(r);auto a=rotation.getValue()*MathConstants<double>::pi/180.;auto proj=[&](double x,double y,double z){return Point<float>((float)(r.getCentreX()+(x*std::cos(a)-y*std::sin(a))*r.getWidth()/85.),(float)(r.getY()+90+(x*std::sin(a)+y*std::cos(a))*6-z*8));};for(int i=-20;i<=20;i+=2){g.setColour(Colour(0xff29414c));g.drawLine({proj(i,0,0),proj(i,40,0)});}for(int j=0;j<=40;j+=2)g.drawLine({proj(-20,j,0),proj(20,j,0)});Path stage;stage.startNewSubPath(proj(-17,-7,0));stage.lineTo(proj(17,-7,0));stage.lineTo(proj(17,0,0));stage.lineTo(proj(-17,0,0));stage.closeSubPath();g.setColour(Colour(0xff233b48));g.fillPath(stage);int n=(int)stacks.getValue(),b=(int)boxes.getValue();for(int i=0;i<n;++i)for(int k=0;k<b;++k){auto p=proj((i-(n-1)/2.)*spacing.getValue(),0,k*1.1);g.setColour(Palette::orange);g.fillRect(p.x-6,p.y-10,12.f,10.f);g.setColour(Palette::plot);g.drawRect(p.x-6,p.y-10,12.f,10.f);}for(int side:{-1,1})for(int k=0;k<8;++k){auto p=proj(side*17,0,7+k*.7);g.setColour(Palette::cyan);g.drawRect(p.x-5,p.y-5,10.f,5.f);}text(g,String::fromUTF8("GEOMETRÍA 3D · ")+String(n)+String::fromUTF8(" stacks × ")+String(b)+" cajas = "+String(n*b)+" recintos",r.removeFromTop(30).reduced(10,0),13,Palette::cyan);text(g,String::fromUTF8("Sin mapa SPL: solver acústico pendiente"),{r.getX()+10,r.getBottom()-32,r.getWidth()-20,24},12,Palette::orange);}
 void paint(Graphics& g) override {commands.refresh();if(mode==0)updateCursorReadout();g.fillAll(Palette::bg);
 if(leftVisible)panel(g,{8,82,leftWidth,getHeight()-115});panel(g,{centerX(),82,centerWidth(),getHeight()-115});if(rightVisible)panel(g,{getWidth()-rightWidth-8,82,rightWidth,getHeight()-115});

 Rectangle<int> center{centerX()+8,118,centerWidth()-16,getHeight()-183};if(mode==2){scene(g,center);if(adjustmentLinked)text(g,"DSP vinculado: "+String(gain.getValue(),1)+" dB / "+String(delay.getValue(),2)+String::fromUTF8(" ms / Ø ")+String(polarity?"ON":"OFF"),{center.getX()+10,center.getY()+35,center.getWidth()-20,25},12,Palette::green);}else if(mode==0){if(measurementView==2||measurementView==3)spectrumGraph(g,center,measurementView==3);else{if(plotChoice.getSelectedId()!=2||measurementView==1)graph(g,plotBounds(0),String::fromUTF8("IMPULSE RESPONSE · ms"),0);if(measurementView==0){graph(g,plotBounds(1),displayName(1),1);graph(g,plotBounds(2),displayName(2),2);}}}else{int usable=center.getHeight()-(mode==1||mode==3?100:0);int irHeight=plotChoice.getSelectedId()==2?0:104;if(irHeight)graph(g,{center.getX(),center.getY(),center.getWidth(),90},String::fromUTF8("IMPULSE RESPONSE · sin datos DSP"),0);int gh=(usable-irHeight)/2;graph(g,{center.getX(),center.getY()+(irHeight?97:0),center.getWidth(),gh},String::fromUTF8("PHASE · grados"),1);graph(g,{center.getX(),center.getY()+irHeight+gh,center.getWidth(),gh},String::fromUTF8("MAGNITUDE · dB / Hz · cian original · verde procesada"),2);if(mode==1||mode==3){int yy=center.getBottom()-85;text(g,String::fromUTF8("CADENA ACTIVA: GANANCIA → DELAY → POLARIDAD"),{center.getX()+8,yy,center.getWidth()-16,26},12,Palette::cyan);text(g,String::fromUTF8("EQ / crossover / all-pass / FIR: implementación pendiente"),{center.getX()+8,yy+29,center.getWidth()-16,25},12,Palette::muted);}}
  if(mode==0&&rightVisible){int x=getWidth()-rightWidth,w=rightWidth-16,gen=CompactLayout::generatorTop(getHeight());g.setColour(Colour(0xff172028));g.fillRoundedRectangle((float)x,(float)gen,(float)w,66.f,4);String generatorState=pinkLoopActive?(internalLoopEnabled?"Rosa "+String(inputEngine.pinkLevel(),1)+" dBFS / ref. interna":"Rosa "+String(inputEngine.pinkLevel(),1)+" dBFS / Output "+String(outputChannel)):(internalLoopEnabled?"Loop interno preparado / Pink detenido":"Generador / detenido");text(g,generatorState,{x+4,gen,w-8,18},10,pinkLoopActive?Palette::green:Palette::muted);}

 text(g,status,{12,getHeight()-28,getWidth()-445,24},12,Palette::muted);text(g,MyaIdentity::version()+String::fromUTF8(" · dBFS / SPL RMS / TF EN DIRECTO"),{getWidth()-425,getHeight()-28,413,24},11,Palette::orange);
 }
};
// Keep the established workspace coordinate system and all hit tests together.
// Its former decorative strip and permanent navigation row are removed from the visible surface. The extra
// height goes to the existing layout, including equal phase/magnitude plots.
class WorkstationSurface : public Component {
public:
 static constexpr int reclaimedHeight=66;
 Workstation station;
 WorkstationSurface(){addAndMakeVisible(station);setSize(1440,960);}
 void resized()override{station.setBounds(0,-reclaimedHeight,getWidth(),getHeight()+reclaimedHeight);}
};
class MainWindow : public DocumentWindow {
 Workstation* workstation=nullptr;
public:
 MainWindow():DocumentWindow(MyaIdentity::name(),Palette::bg,allButtons){
 setUsingNativeTitleBar(true);auto* surface=new WorkstationSurface();workstation=&surface->station;setContentOwned(surface,true);
 #if JUCE_MAC
 MenuBarModel::setMacMainMenu(workstation);
 #else
 // All menus are retained in the chevron overlay on Windows.
 #endif
 setResizable(true,false);setResizeLimits(1120,900,3000,2000);centreWithSize(1440,960);setVisible(true);installMyaNativeTitleBrand(*this);workstation->grabKeyboardFocus();}
 ~MainWindow()override{
 #if JUCE_MAC
 MenuBarModel::setMacMainMenu(nullptr);
 #else
 setMenuBar(nullptr);
 #endif
 }
 void closeButtonPressed()override{JUCEApplication::getInstance()->systemRequestedQuit();}
 void checkForUpdates(){if(workstation)workstation->checkForUpdates(false);}
};
class MyaApp:public JUCEApplication {
 std::unique_ptr<MainWindow> window;std::unique_ptr<MyaStartupWindow> startup;std::shared_ptr<bool> alive=std::make_shared<bool>(true);
public:
 const String getApplicationName()override{return MyaIdentity::name();}const String getApplicationVersion()override{return MyaIdentity::version();}
 void initialise(const String&)override{startup=std::make_unique<MyaStartupWindow>(true);auto token=alive;MessageManager::callAsync([this,token]{if(!*token)return;window=std::make_unique<MainWindow>();startup.reset();window->checkForUpdates();});}
 void shutdown()override{*alive=false;startup.reset();window.reset();}
};

#if !defined(MYA_OFFSCREEN_RENDER)
START_JUCE_APPLICATION(MyaApp)
#endif
