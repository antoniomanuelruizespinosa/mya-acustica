#include <juce_gui_extra/juce_gui_extra.h>
#include <complex>
#include <optional>
#include "AudioRouting.h"
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
using namespace juce;
namespace Palette { const Colour bg{0xff222427}, panel{0xff2c3036}, plot{0xff080a0c}, ink{0xffe5edf4}, muted{0xffa9bac8}, cyan{0xff00e5ed}, blue{0xff159fe5}, orange{0xffffac24}, green{0xff54ee22}, line{0xff626e78}; }
class MyaLook : public LookAndFeel_V4 {
public:
 Font getComboBoxFont(ComboBox& box) override{return Font(FontOptions(box.getHeight()<=20?11.f:13.f));}
 MyaLook(){setColour(Label::textColourId,Palette::ink);setColour(Slider::textBoxTextColourId,Palette::cyan);setColour(Slider::textBoxBackgroundColourId,Colour(0xff111920));setColour(Slider::textBoxOutlineColourId,Palette::line);setColour(Slider::trackColourId,Palette::cyan);setColour(ComboBox::backgroundColourId,Colour(0xff18242d));setColour(ComboBox::textColourId,Palette::cyan);setColour(PopupMenu::backgroundColourId,Palette::panel);setColour(PopupMenu::textColourId,Palette::ink);}
 void drawButtonBackground(Graphics& g,Button& b,const Colour&,bool over,bool down) override {auto r=b.getLocalBounds().toFloat().reduced(.5f);if(b.getProperties()["trackCircle"]){float d=jmin(r.getWidth(),r.getHeight())-4;auto c=Rectangle<float>{r.getCentreX()-d/2,r.getCentreY()-d/2,d,d};auto col=Colour::fromString(b.getProperties()["trackColour"].toString());if(b.getToggleState()){g.setColour(col.withAlpha(.3f));g.fillEllipse(c);}g.setColour(b.getToggleState()?col:Palette::muted);g.drawEllipse(c,2);g.drawEllipse(c.reduced(4),1);return;}g.setGradientFill(ColourGradient(Colour(0xff3a4b58),0,0,Colour(0xff172028),0,r.getHeight(),false));g.fillRoundedRectangle(r,4);g.setColour(b.getToggleState()?Palette::cyan:(over?Palette::ink:Palette::line));g.drawRoundedRectangle(r,4,down?2.f:1.f);}
 void drawButtonText(Graphics& g,TextButton& b,bool,bool) override {
 if(b.getProperties()["trackCircle"])return;
 auto icon=b.getProperties()["transportIcon"].toString();
 if(icon.isNotEmpty()){g.setColour(b.isEnabled()?Palette::ink:Palette::muted);float cx=(float)b.getWidth()*.5f,cy=(float)b.getHeight()*.5f;float r=jmin(7.f,(float)b.getHeight()*.27f);if(icon=="play"){Path p;p.addTriangle(cx-r*.7f,cy-r,cx-r*.7f,cy+r,cx+r,cy);g.fillPath(p);}else if(icon=="stop")g.fillRect(cx-r,cy-r,r*2,r*2);else{g.fillRect(cx-r,cy-r,r*.65f,r*2);g.fillRect(cx+r*.35f,cy-r,r*.65f,r*2);}return;}
g.setColour(b.isEnabled()?(b.getToggleState()?Palette::cyan:Palette::ink):Palette::muted.withAlpha(.55f));g.setFont(13.f);g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(4),Justification::centred,1);}
};
class TraceSwitch : public Button, private Timer {
 void timerCallback() override{stopTimer();setToggleState(!getToggleState(),dontSendNotification);if(onClick)onClick();}

public:
 std::function<void()> openSettings;
 Colour colour{Palette::green};
 TraceSwitch():Button("Mostrar trazo") {setClickingTogglesState(true);setToggleState(true,dontSendNotification);}
 void mouseUp(const MouseEvent& e) override {if(e.getNumberOfClicks()==1&&getLocalBounds().contains(e.getPosition()))startTimer(MouseEvent::getDoubleClickTimeout());}
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

class RoutingPanel : public Component {
 std::vector<AudioEndpoint> devices;
 std::array<Label,4> names;
 std::array<ComboBox,4> device,mic,ref;
 ComboBox outputDevice,output;
 Label heading;
 int spectrumRows=0, tfTop=0, avgTop=0, generatorTop=0;
 std::function<Colour(int)> traceColour;
 std::function<void(int,String,String,int,int)> changed;
 std::function<void(String,int)> outputChanged;
 String keyFor(int id)const{return id>0&&id<=(int)devices.size()?devices[(size_t)id-1].key:String();}
 void channels(ComboBox& box,const StringArray& labels,int selected){
  box.clear(dontSendNotification);box.addItemList(labels,1);
  box.setSelectedId(selected>0&&selected<=labels.size()?selected:0,dontSendNotification);
  box.setTextWhenNothingSelected("Not assigned");box.setEnabled(!labels.isEmpty());
 }
 void update(int i){auto k=(size_t)i;changed(i,names[k].getText(),keyFor(device[k].getSelectedId()),mic[k].getSelectedId(),ref[k].getSelectedId());}
 void inputChannels(int i,int m,int r){auto k=(size_t)i;auto* d=findEndpoint(devices,keyFor(device[k].getSelectedId()));channels(mic[k],d?d->inputs:StringArray(),m);channels(ref[k],d?d->inputs:StringArray(),r);}
 void populateDevice(ComboBox& box,const String& key,bool input){
  for(size_t i=0;i<devices.size();++i)if(!(input?devices[i].inputs:devices[i].outputs).isEmpty())box.addItem(devices[i].name+" ["+devices[i].backend+"]",(int)i+1);
  int selected=0;for(size_t i=0;i<devices.size();++i)if(devices[i].key==key)selected=(int)i+1;
  box.setSelectedId(selected,dontSendNotification);
  if(selected==0&&!key.isEmpty())box.setText(key+" [Unavailable]",dontSendNotification);
  box.setTextWhenNothingSelected("Not assigned");
 }
public:
 RoutingPanel(std::vector<AudioEndpoint> catalog,const std::array<String,4>& n,const std::array<String,4>& routes,const std::array<int,4>& m,const std::array<int,4>& r,String outKey,int outChannel,std::function<Colour(int)> colours,std::function<void(int,String,String,int,int)> change,std::function<void(String,int)> outChange):devices(std::move(catalog)),traceColour(std::move(colours)),changed(std::move(change)),outputChanged(std::move(outChange)){
 for(const auto& d:devices)spectrumRows+=d.inputs.size();
 tfTop=116+jmax(1,spectrumRows)*30;avgTop=tfTop+234;generatorTop=avgTop+82;
 heading.setText(String((int)devices.size())+" audio devices detected | Acquisition stopped",dontSendNotification);addAndMakeVisible(heading);
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
 heading.setBounds(12,4,660,32);
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
 label("Input / "+deviceName,16,447,650);mic.addItemList(inputChannels,1);ref.addItemList(inputChannels,1);mic.setTextWhenNothingSelected("Not assigned");ref.setTextWhenNothingSelected("Not assigned");mic.setEnabled(!inputChannels.isEmpty());ref.setEnabled(!inputChannels.isEmpty());mic.setSelectedId(m);ref.setSelectedId(r);mic.setBounds(16,480,310,28);ref.setBounds(350,480,320,28);addAndMakeVisible(mic);addAndMakeVisible(ref);label("Cambios aplicados a la demo. Guardar en la ventana principal conserva el proyecto.",16,529,660);
 name.onTextChange=[this]{update();};delay.onValueChange=[this]{update();};mic.onChange=[this]{update();};ref.onChange=[this]{update();};listener.callback=[this]{update();};
 }
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
class Workstation : public Component, public KeyListener, public MenuBarModel, private ChangeListener {
 MyaLook look;
 AudioDeviceWatcher deviceWatcher;
 std::unique_ptr<SettingsDrawer> settingsDrawer;
 String drawerKey;
 std::vector<std::unique_ptr<TextButton>> buttons;
 TextButton *measure,*dsp,*vision,*lab,*invert,*bypass;
 Slider gain,delay,stacks,boxes,spacing,rotation;
 Label gainLabel,delayLabel,stacksLabel,boxesLabel,spacingLabel,rotationLabel;
 ComboBox source, plotChoice, averageChoice, meterInput, meterWeight, generatorOutput;
 std::array<ComboBox,3> plotMic;
 std::array<ComboBox,2> plotSmooth,plotTypeChoice;
 std::array<TextButton,3> graphGear;
 std::unique_ptr<GraphSettingsWindow> graphWindow;bool graphWindowEnglish=false;
 TFDisplayState tfSettings;
 ComboBox impulseScale;
 std::array<int,2> plotTypes{{1,2}};
 int irScale=0;
 ImpulseDisplay impulseDisplay;
 int displayType(int panel) const{return panel==0?0:plotTypes[(size_t)(panel-1)];}
 String displayName(int panel) const{return displayType(panel)==1?juce::String::fromUTF8("FASE · grados / demo"):juce::String::fromUTF8("MAGNITUD · dB / Hz · demo");}

 Slider coherenceThreshold;
 ToggleButton showCoherence{"Coherence (demo)"};
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
 bool ir=plotChoice.getSelectedId()!=2;int gap=8;
 auto heights=CompactLayout::heights(c.getHeight(),ir,irFraction,phaseFraction);int ih=heights.ir,ph=heights.phase;
 if(type==0)return {c.getX(),c.getY(),c.getWidth(),ih};
 int top=c.getY()+(ir?ih+gap:0);if(type==1)return {c.getX(),top,c.getWidth(),ph};return {c.getX(),top+ph+gap,c.getWidth(),heights.magnitude};
 }

 class Readout : public Label {
 public:
 std::vector<std::pair<String,Colour>> parts;
 void paint(Graphics& g) override{g.setColour(findColour(Label::backgroundColourId));g.fillRect(getLocalBounds());
  if(parts.empty())parts={{getText(),Palette::muted}};
  float total=0;Font font(FontOptions(12.f));for(auto& part:parts)total+=GlyphArrangement::getStringWidth(font,part.first)+16;
  float scale=jmin(1.f,(float)(getWidth()-8)/jmax(1.f,total));g.saveState();g.addTransform(AffineTransform::scale(scale,1.f));float x=4;
  for(auto& part:parts){float w=GlyphArrangement::getStringWidth(font,part.first);g.setFont(font);g.setColour(part.second);g.drawText(part.first,Rectangle<float>{x,0,w,(float)getHeight()},Justification::centredLeft);x+=w+16;}g.restoreState();}
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
 std::array<TextButton,7> aiButtons;TextButton sumButton,peakSettingsButton;
 Slider selectedDelay, phasePreview;
 void captureTrace(){
 int index=source.getSelectedId()-1;if(index<0||index>=4){info("Captura",String::fromUTF8("Selecciona una TF. El cálculo de promedios aún está pendiente."));return;}
 auto k=(size_t)index;auto stamp=Time::getCurrentTime();auto name=micNames[k].getText()+" / "+stamp.formatted("%H:%M:%S");
 DynamicObject::Ptr data=new DynamicObject();data->setProperty("schema",1);data->setProperty("synthetic",true);data->setProperty("name",name);data->setProperty("deviceProfile",deviceKeys[k]);data->setProperty("micInput",micInputs[k]);data->setProperty("refInput",refInputs[k]);data->setProperty("measurementDelayMs",micDelay[k].getValue());data->setProperty("colour",traceSwitch[k].colour.toString());Array<var> samples;
 for(int n=0;n<=700;++n){double u=n/700.,f=20*std::pow(1000.,u);auto h=response(f,false)*std::polar(std::pow(10.,(.65*index*std::sin(u*8+index))/20),.12*index*std::cos(u*7)+2*MathConstants<double>::pi*f*micDelay[k].getValue()/1000.);DynamicObject::Ptr point=new DynamicObject();point->setProperty("frequencyHz",f);point->setProperty("real",h.real());point->setProperty("imaginary",h.imag());point->setProperty("coherence",demoCoherence(u,index));samples.add(var(point.get()));}
 data->setProperty("createdAtUnixMs",stamp.toMilliseconds());data->setProperty("samples",samples);data->setProperty("dataKind","snapshot-tf");data->setProperty("acquisitionMethod","synthetic-demo");data->setProperty("sampleRateHz",var());data->setProperty("timeReference","synthetic-demo / measurementDelayMs");data->setProperty("windowRecipe","unknown");data->setProperty("captureDSPState","unknown");data->setProperty("coherenceConvention","synthetic gammaSquared power ratio model; not a measured estimator");
 auto id=captureLibrary.capture(var(data.get()),name+juce::String::fromUTF8(" · DEMO TF"),traceSwitch[k].colour.toString());
 if(id.isNotEmpty()){status=juce::String::fromUTF8("Captura TF DEMO guardada → ")+captureLibrary.folderPath(captureLibrary.state().active);libraryPanel.refresh();refreshLibrary();}
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
 void persistWorkspace(){WorkspaceViewState state{leftWidth,rightWidth,leftVisible,rightVisible,inputMeterSettings};if(!state.save(projectFile.getParentDirectory().getChildFile("vista-trabajo.json")))status="No se pudo guardar la vista de trabajo";}
 void reflectInputMeters(){for(size_t k=0;k<4;++k){inputMeters[k].colour=traceSwitch[k].colour;inputMeters[k].settings=inputMeterSettings;inputMeters[k].repaint();tfActions[k][1].getProperties().set("trackColour",traceSwitch[k].colour.toString());tfActions[k][1].repaint();}}
 void inputMeterOptions(){showDrawer("input-meters","M/R / Pico retenido",std::make_unique<TFMeterSettingsPanel>(inputMeterSettings,[this](TFMeterSettings state){inputMeterSettings=state;reflectInputMeters();persistWorkspace();},[this]{for(auto& meter:inputMeters)meter.resetPeaks();}));}


 int mode=0; bool polarity=false,disabled=false,adjustmentLinked=false;
 String status=String::fromUTF8("Prototipo 0.3 · datos sintéticos · audio desactivado");
 File projectFile{File(String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("proyecto-demo.json")};
 MeterState meterState{projectFile.getParentDirectory().getChildFile("medidores.json")};
 std::unique_ptr<MeterWindow> meters;
 MeterPanel dockMeters{meterState,true};
 TextButton tracesPage{"Trazos"},metersPage{"dB Meter"};
 int meterListener=0;
 void refreshMeterSources(){std::vector<MeterState::Source> sources;for(const auto& device:audioEndpoints)for(int i=0;i<device.inputs.size();++i){String label=device.name+" / "+device.inputs[i];for(size_t k=0;k<4;++k)if(deviceKeys[k]==device.key&&micInputs[k]==i+1)label+=" / "+micNames[k].getText();sources.push_back({device.key,label,i+1});}meterState.setSources(std::move(sources));}
 void showMeters(){if(meters&&meters->isVisible()){meters->closeButtonPressed();return;}refreshMeterSources();if(!meters){meters=std::make_unique<MeterWindow>(meterState);meters->setLookAndFeel(&look);meters->panel->toggleRequested=[this]{commands.invoke(203);};}meters->setVisible(true);meters->toFront(true);}
 Rectangle<int> plotArea(Rectangle<int> r) const{auto p=CompactLayout::plot(r.getWidth(),r.getHeight());return {r.getX()+p.x,r.getY()+p.y,p.w,p.h};}
 Rectangle<int> activeGraphBounds(int type) const{if(mode==0)return plotBounds(type);Rectangle<int> center{centerX()+8,118,centerWidth()-16,getHeight()-183};int usable=center.getHeight()-100,irHeight=plotChoice.getSelectedId()==2?0:104,gh=(usable-irHeight)/2;if(type==0)return {center.getX(),center.getY(),center.getWidth(),90};if(type==1)return {center.getX(),center.getY()+(irHeight?97:0),center.getWidth(),gh};return {center.getX(),center.getY()+irHeight+gh,center.getWidth(),gh};}
 File tfSettingsFile() const{return projectFile.getParentDirectory().getChildFile("tf-graficos.json");}
 void persistTF(){tfSettings.linkPhaseMag=linkSmooth.getToggleState();tfSettings.showIR=plotChoice.getSelectedId()==1;tfSettings.coherenceOn=showCoherence.getToggleState();tfSettings.coherenceThreshold=coherenceThreshold.getValue();if(!tfSettings.save(tfSettingsFile()))status="No se pudieron guardar ajustes TF";}
 void reflectTF(){linkSmooth.setToggleState(tfSettings.linkPhaseMag,dontSendNotification);showCoherence.setToggleState(tfSettings.coherenceOn,dontSendNotification);coherenceThreshold.setValue(tfSettings.coherenceThreshold,dontSendNotification);plotChoice.setSelectedId(tfSettings.showIR?1:2,dontSendNotification);for(auto& combo:plotSmooth){combo.setItemEnabled(1,tfSettings.allowNone);if(!tfSettings.allowNone&&combo.getSelectedId()==1)combo.setSelectedId(2,dontSendNotification);}if(tfSettings.linkPhaseMag)plotSmooth[1].setSelectedId(plotSmooth[0].getSelectedId(),dontSendNotification);}
 void applyTF(TFDisplayState state){if(!state.valid()){status="Rangos o pasos TF no validos";if(graphWindow)graphWindow->repaint();repaint();return;}bool changedLinear=(tfSettings.frequencyGrid==4)!=(state.frequencyGrid==4);bool enableZoom=!tfSettings.linkFrequencyZoom&&state.linkFrequencyZoom;tfSettings=state;reflectTF();if(enableZoom)PlotPresentation::linkFrequency(plotZoomX,plotPanX,activePlot>0?activePlot:1);if(changedLinear){plotPanX[1]=plotPanX[2]=0;plotZoomX[1]=plotZoomX[2]=1;}persistTF();persistGraphChoices();resized();if(graphWindow)graphWindow->repaint();repaint();menuItemsChanged();}
 void tfGraphSettings(int tab=1){auto oldBounds=graphWindow?graphWindow->getBounds():Rectangle<int>{};bool en=commands.language==MyaCommands::Language::EN;if(graphWindow&&en!=graphWindowEnglish)graphWindow.reset();graphWindowEnglish=en;if(!graphWindow)graphWindow=std::make_unique<GraphSettingsWindow>(commands.language==MyaCommands::Language::EN,[this]{return tfSettings;},[this](TFDisplayState state){applyTF(state);},[this](int id){commands.invoke(id);},[this]{return irScale;});if(!oldBounds.isEmpty())graphWindow->setBounds(oldBounds);graphWindow->setLookAndFeel(&look);graphWindow->toggleMeters=[this]{commands.invoke(203);};graphWindow->select(tab);graphWindow->setVisible(true);graphWindow->toFront(true);}

 std::optional<double> panelCoherence(double u,int panel) const{auto saved=capturedFront(panel);if(saved.isNotEmpty())return savedTraces.coherence(saved,tfSettings.frequency(u));int index=frontTrace(panel);if(index<0||u<0||u>1)return std::nullopt;double sourceU=std::log(tfSettings.frequency(u)/20.)/std::log(1000.);return demoCoherence(sourceU,index);}
 std::optional<double> savedValue(const String& id,double u,int panel){auto c=savedTraces.coherence(id,tfSettings.frequency(u));if(!tfSettings.passes(c))return std::nullopt;int kind=displayType(panel);auto v=savedTraces.value(id,tfSettings.frequency(u),kind,plotSmooth[(size_t)(panel-1)].getSelectedId(),0);return v?std::optional<double>(tfSettings.normalize(*v*(kind==1?360.:36.),kind)):std::nullopt;}

 CaptureLibrary captureLibrary{projectFile.getParentDirectory().getChildFile("Biblioteca")};
 CaptureLibraryPanel libraryPanel{captureLibrary};
 CapturedTraceDisplay savedTraces;
 std::array<String,3> frontCaptured;
 TextButton restoreLibraryButton{">"};
 var libraryDSPBindings{Array<var>{}};
 void refreshLibrary(){
  savedTraces.reload(captureLibrary);
  for(auto& id:frontCaptured){auto* n=savedTraces.find(id);if(!n||!n->visible)id.clear();}
  auto visible=savedTraces.visible();for(int panel:{1,2})if(frontCaptured[(size_t)panel].isEmpty()&&frontTrace(panel)<0&&!visible.empty())frontCaptured[(size_t)panel]=visible.back();
  repaint();menuItemsChanged();
 }
 String capturedFront(int panel) const{
  if(panel==0)return {};auto preferred=frontCaptured[(size_t)panel];auto* n=savedTraces.find(preferred);if(n&&n->visible)return preferred;
  if(frontTrace(panel)<0){auto ids=savedTraces.visible();if(!ids.empty())return ids.back();}return {};
 }
 std::optional<double> panelValue(double u,int type){
  auto id=capturedFront(type);
  if(type>0&&id.isNotEmpty())return savedValue(id,u,type);
  return plotValue(u,frontTrace(type),type);
 }
 String panelName(int type) const{
  if(auto* n=savedTraces.find(capturedFront(type)))return n->name;
  int index=frontTrace(type);return index>=0?micNames[(size_t)index].getText():String("sin trazo");
 }
 void drawSaved(Graphics& g,Rectangle<int> p,int type,bool front,double panX,double panY,double zoomX,double zoomY){
  if(type==0)return;
  for(const auto& id:savedTraces.visible()){
   if((id==capturedFront(type))!=front)continue;Path path;bool gap=true;double previous=0;
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

 void syncNames(){reflectInputMeters();for(auto& menu:plotMic)for(int i=0;i<4;++i)menu.changeItemText(i+2,micNames[(size_t)i].getText());int selected=source.getSelectedId();source.clear(dontSendNotification);for(int i=0;i<4;++i){auto k=(size_t)i;auto* d=findEndpoint(audioEndpoints,deviceKeys[k]);String device=d?d->name:(deviceKeys[k].isEmpty()?"Not assigned":deviceKeys[k]+" [Unavailable]");String meas=d&&micInputs[k]>0&&micInputs[k]<=d->inputs.size()?d->inputs[micInputs[k]-1]:String("Not assigned");String ref=d&&refInputs[k]>0&&refInputs[k]<=d->inputs.size()?d->inputs[refInputs[k]-1]:String("Not assigned");String route=device+" | M: "+meas+" / R: "+ref;source.addItem(micNames[k].getText()+" | "+route,i+1);micNames[k].setColour(Label::textColourId,traceSwitch[k].colour);micNames[k].setTooltip(route);}
 source.addItem("AVG",5);source.setSelectedId(selected>0?selected:1,dontSendNotification);
 generatorOutput.clear(dontSendNotification);auto* output=findEndpoint(audioEndpoints,outputKey);if(output){generatorOutput.addItemList(output->outputs,1);generatorOutput.setSelectedId(outputChannel,dontSendNotification);}generatorOutput.setTextWhenNothingSelected("Not assigned");
 meterInput.clear(dontSendNotification);int k=source.getSelectedId()-1;if(k>=0&&k<4){auto* d=findEndpoint(audioEndpoints,deviceKeys[(size_t)k]);if(d){meterInput.addItemList(d->inputs,1);meterInput.setSelectedId(micInputs[(size_t)k],dontSendNotification);}}meterInput.setTextWhenNothingSelected("Not assigned");refreshMeterSources();repaint();}


 void positionDrawer(){if(settingsDrawer){int edge=rightVisible?getWidth()-rightWidth-16:getWidth()-8;int width=jmin(718,edge-16);settingsDrawer->setBounds(edge-width,82,width,getHeight()-115);settingsDrawer->toFront(false);}}
 void showDrawer(String key,String title,std::unique_ptr<Component> body){if(settingsDrawer&&settingsDrawer->isVisible()&&drawerKey==key){if(key=="audio")settingsDrawer->setVisible(false);return;}drawerKey=key;settingsDrawer=std::make_unique<SettingsDrawer>(title,std::move(body));settingsDrawer->requestClose=[this]{commands.invoke(602);};addAndMakeVisible(*settingsDrawer);positionDrawer();}
 void engineSettings(int i){auto k=(size_t)i;auto* endpoint=findEndpoint(audioEndpoints,deviceKeys[k]);Component::SafePointer<Workstation> safe(this);auto panel=std::make_unique<EnginePanel>(micNames[k].getText(),micDelay[k].getValue(),micInputs[k],refInputs[k],traceSwitch[k].colour,endpoint?endpoint->name:String("Not assigned / unavailable"),endpoint?endpoint->inputs:StringArray(),[safe,i](String n,double d,int mic,int ref,Colour c){if(safe){auto j=(size_t)i;auto name=n.trim();safe->micNames[j].setText(name.isEmpty()?"TF "+String(i+1):name,dontSendNotification);safe->micDelay[j].setValue(d,dontSendNotification);safe->micInputs[j]=mic;safe->refInputs[j]=ref;safe->traceSwitch[j].colour=c;safe->traceSwitch[j].repaint();if(safe->settingsDrawer)safe->settingsDrawer->setAccent(c);safe->tfContent.repaint();safe->syncNames();}});showDrawer("TF "+String(i+1),"Opciones / "+micNames[k].getText(),std::move(panel));if(settingsDrawer)settingsDrawer->setAccent(traceSwitch[k].colour);}
 void audioSettings(){audioEndpoints=discoverAudioEndpoints();syncNames();std::array<String,4> names;for(size_t i=0;i<4;++i)names[i]=micNames[i].getText();Component::SafePointer<Workstation> safe(this);
 auto panel=std::make_unique<RoutingPanel>(audioEndpoints,names,deviceKeys,micInputs,refInputs,outputKey,outputChannel,[safe](int i){return safe?safe->traceSwitch[(size_t)i].colour:Palette::muted;},[safe](int i,String name,String key,int mic,int ref){if(safe){auto k=(size_t)i;safe->micNames[k].setText(name.trim().isEmpty()?"TF "+String(i+1):name,dontSendNotification);safe->deviceKeys[k]=key;safe->micInputs[k]=mic;safe->refInputs[k]=ref;safe->syncNames();}},[safe](String key,int channel){if(safe){safe->outputKey=key;safe->outputChannel=channel;safe->syncNames();}});
 showDrawer("audio","Measurement Configuration",std::move(panel));}

 void info(String title,String body){AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,title,body,"Cerrar");}
 void setup(Slider& s,Label& l,String name,double lo,double hi,double step,double value){s.setRange(lo,hi,step);s.setValue(value);s.setSliderStyle(Slider::LinearHorizontal);s.setTextBoxStyle(Slider::TextBoxRight,false,72,25);s.onValueChange=[this]{repaint();};l.setText(name,dontSendNotification);addAndMakeVisible(s);addAndMakeVisible(l);}
 void select(int n){if(settingsDrawer)settingsDrawer->setVisible(false);mode=n;measure->setToggleState(n==0,dontSendNotification);dsp->setToggleState(n==1,dontSendNotification);vision->setToggleState(n==2,dontSendNotification);lab->setToggleState(n==3,dontSendNotification);resized();repaint();}
 void save(){DynamicObject::Ptr o=new DynamicObject();o->setProperty("schema",1);o->setProperty("synthetic",true);DynamicObject::Ptr lib=new DynamicObject();lib->setProperty("libraryId",captureLibrary.state().id);lib->setProperty("manifest","Biblioteca/biblioteca.json");lib->setProperty("bindings",libraryDSPBindings);o->setProperty("captureLibrary",var(lib.get()));o->setProperty("gainDB",gain.getValue());o->setProperty("delayMs",delay.getValue());o->setProperty("polarity",polarity);o->setProperty("stacks",stacks.getValue());o->setProperty("boxesPerStack",boxes.getValue());o->setProperty("spacingM",spacing.getValue());o->setProperty("linked",adjustmentLinked);Array<var> micros;for(size_t i=0;i<4;++i){DynamicObject::Ptr m=new DynamicObject();m->setProperty("name",micNames[i].getText());m->setProperty("deviceKey",deviceKeys[i]);m->setProperty("micInput",micInputs[i]);m->setProperty("refInput",refInputs[i]);m->setProperty("visible",traceSwitch[i].getToggleState());m->setProperty("colour",traceSwitch[i].colour.toString());m->setProperty("measurementDelayMs",micDelay[i].getValue());micros.add(var(m.get()));}o->setProperty("micros",micros);
 DynamicObject::Ptr view=new DynamicObject();
 view->setProperty("leftVisible",leftVisible);view->setProperty("rightVisible",rightVisible);
 view->setProperty("irFraction",irFraction);view->setProperty("phaseFraction",phaseFraction);
 view->setProperty("tfDisplay",tfSettings.encode());view->setProperty("graphRepresentations",GraphViewState{plotTypes,irScale,linkSmooth.getToggleState()}.encode());view->setProperty("irScale",irScale);view->setProperty("commandLanguage",commands.language==MyaCommands::Language::ES?"es":"en");view->setProperty("plotChoice",plotChoice.getSelectedId());view->setProperty("selectedTF",source.getSelectedId());
 view->setProperty("linkSmoothing",linkSmooth.getToggleState());view->setProperty("coherence",showCoherence.getToggleState());view->setProperty("coherenceThreshold",coherenceThreshold.getValue());
 Array<var> plots;for(size_t i=0;i<3;++i){DynamicObject::Ptr item=new DynamicObject();item->setProperty("trace",plotMic[i].getSelectedId());if(i>0)item->setProperty("displayType",plotTypes[i-1]);item->setProperty("panX",plotPanX[i]);item->setProperty("panY",plotPanY[i]);item->setProperty("zoomX",plotZoomX[i]);item->setProperty("zoomY",plotZoomY[i]);if(i>0)item->setProperty("smoothing",plotSmooth[i-1].getSelectedId());plots.add(var(item.get()));}view->setProperty("plots",plots);o->setProperty("view",var(view.get()));o->setProperty("outputDeviceKey",outputKey);o->setProperty("outputChannel",outputChannel);if(projectFile.replaceWithText(JSON::toString(var(o.get()))))status=String::fromUTF8("Proyecto demo guardado · datos sintéticos");else status="No se pudo guardar el proyecto";repaint();}
 void load(){auto v=JSON::parse(projectFile);if(!v.isObject()||int(v["schema"])!=1){status=String::fromUTF8("No hay un proyecto demo válido guardado");repaint();return;}libraryDSPBindings=Array<var>{};if(v["captureLibrary"]["libraryId"].toString()==captureLibrary.state().id&&v["captureLibrary"]["bindings"].isArray())libraryDSPBindings=v["captureLibrary"]["bindings"];outputKey=v["outputDeviceKey"].toString();outputChannel=int(v["outputChannel"]);gain.setValue(double(v["gainDB"]));delay.setValue(double(v["delayMs"]));stacks.setValue(double(v["stacks"]));boxes.setValue(double(v["boxesPerStack"]));spacing.setValue(double(v["spacingM"]));polarity=bool(v["polarity"]);adjustmentLinked=bool(v["linked"]);invert->setToggleState(polarity,dontSendNotification);if(auto* ms=v["micros"].getArray()){for(int i=0;i<jmin(4,ms->size());++i){auto m=ms->getReference(i);if(!m.isObject())continue;auto k=(size_t)i;if(m.hasProperty("name")){auto name=m["name"].toString().trim();micNames[k].setText(name.isEmpty()?"TF "+String(i+1):name,dontSendNotification);}deviceKeys[k]=m["deviceKey"].toString();micInputs[k]=deviceKeys[k].isEmpty()?0:jmax(0,int(m["micInput"]));refInputs[k]=deviceKeys[k].isEmpty()?0:jmax(0,int(m["refInput"]));traceSwitch[k].setToggleState(bool(m["visible"]),dontSendNotification);auto col=m["colour"].toString();if(col.length()==8)traceSwitch[k].colour=Colour::fromString(col).withAlpha(1.f);micDelay[k].setValue(double(m["measurementDelayMs"]),dontSendNotification);traceSwitch[k].repaint();}}syncNames();auto view=v["view"];if(view.isObject()){
 irScale=jlimit(0,2,int(view["irScale"]));impulseScale.setSelectedId(irScale+1,dontSendNotification);
 leftVisible=bool(view["leftVisible"]);rightVisible=bool(view["rightVisible"]);
 irFraction=jlimit(.1,.6,double(view["irFraction"]));phaseFraction=jlimit(.15,.85,double(view["phaseFraction"]));
 if(view.hasProperty("commandLanguage"))commands.language=view["commandLanguage"].toString()=="en"?MyaCommands::Language::EN:MyaCommands::Language::ES;
 plotChoice.setSelectedId(jlimit(1,2,int(view["plotChoice"])),dontSendNotification);source.setSelectedId(jlimit(1,5,int(view["selectedTF"])),dontSendNotification);
 linkSmooth.setToggleState(bool(view["linkSmoothing"]),dontSendNotification);showCoherence.setToggleState(bool(view["coherence"]),dontSendNotification);coherenceThreshold.setValue(jlimit(0.,100.,double(view["coherenceThreshold"])),dontSendNotification);
 if(auto* plots=view["plots"].getArray())for(int i=0;i<jmin(3,plots->size());++i){auto item=plots->getReference(i);if(!item.isObject())continue;auto k=(size_t)i;if(i>0){plotTypes[k-1]=item.hasProperty("displayType")?jlimit(1,2,int(item["displayType"])):i;plotTypeChoice[k-1].setSelectedId(plotTypes[k-1],dontSendNotification);}plotMic[k].setSelectedId(jlimit(1,5,int(item["trace"])),dontSendNotification);plotPanX[k]=jlimit(-2.,2.,double(item["panX"]));plotPanY[k]=jlimit(-10.,10.,double(item["panY"]));plotZoomX[k]=jlimit(i==0?.25:1.,64.,double(item.hasProperty("zoomX")?item["zoomX"]:item["zoom"]));plotZoomY[k]=jlimit(.25,16.,double(item.hasProperty("zoomY")?item["zoomY"]:item["zoom"]));if(i>0)plotSmooth[k-1].setSelectedId(jlimit(1,7,int(item["smoothing"])),dontSendNotification);}
 if(linkSmooth.getToggleState())plotSmooth[1].setSelectedId(plotSmooth[0].getSelectedId(),dontSendNotification);
 }plotPanX[0]=0;for(int panel:{1,2}){auto k=(size_t)panel;double half=.5/plotZoomX[k];plotPanX[k]=jlimit(half,1.-half,.5+plotPanX[k])-.5;}if(auto graph=GraphViewState::decode(view["graphRepresentations"]))restoreGraphChoices(*graph);if(auto tf=TFDisplayState::decode(view["tfDisplay"])){tfSettings=*tf;reflectTF();if(tfSettings.linkFrequencyZoom)PlotPresentation::linkFrequency(plotZoomX,plotPanX,1);}persistTF();persistGraphChoices();resized();tfContent.repaint();status="Proyecto demo recuperado";repaint();}
 #include "WorkstationCommands.inc"
public:
 StringArray getMenuBarNames() override{return commands.menuNames();}
 PopupMenu getMenuForIndex(int index,const String&) override{return commands.menu(index);}
 void menuItemSelected(int id,int) override{commands.invoke(id);}
 #if defined(MYA_OFFSCREEN_RENDER)
 void previewCursor(int panel,double u){cursorPlot=panel;cursorU=u;updateCursorReadout();}
 #endif
 Workstation(){setLookAndFeel(&look);
 coherenceThreshold.setRange(0,100,.01);coherenceThreshold.setValue(0);coherenceThreshold.setSliderStyle(Slider::LinearVertical);coherenceThreshold.setTextBoxStyle(Slider::NoTextBox,false,0,0);coherenceThreshold.setTitle("Coherence Threshold (%)");coherenceThreshold.setTooltip("Umbral de coherencia: oculta muestras por debajo del porcentaje. Datos simulados.");coherenceThreshold.onValueChange=[this]{tfSettings.coherenceThreshold=coherenceThreshold.getValue();persistTF();status="Coherence Threshold: "+String(coherenceThreshold.getValue(),2).trimCharactersAtEnd("0").trimCharactersAtEnd(".")+"% / demo";repaint();};addAndMakeVisible(coherenceThreshold);
 showCoherence.setToggleState(true,dontSendNotification);addAndMakeVisible(showCoherence);

 cursorReadout.setText("-- Hz       -- dB       -- deg",dontSendNotification);cursorReadout.setJustificationType(Justification::centred);cursorReadout.setFont(Font(FontOptions(15.f)));cursorReadout.setColour(Label::backgroundColourId,Palette::plot);cursorReadout.setInterceptsMouseClicks(true,false);addAndMakeVisible(cursorReadout);

 for(int i=0;i<2;++i){auto& divider=plotDividers[(size_t)i];addAndMakeVisible(divider);divider.reset=[this]{irFraction=.14;phaseFraction=.5;resized();repaint();};divider.moved=[this,i](int y){int height=getHeight()-183;if(i==0)irFraction=jlimit(60.,(double)jmax(60,height-176),(double)y-118)/height;else{auto p=plotBounds(1);int remaining=118+height-p.getY()-8;phaseFraction=jlimit(80.,(double)jmax(80,remaining-80),(double)y-p.getY())/remaining;}resized();repaint();};}

 impulseScale.addItemList({"Lin","Log","ETC"},1);impulseScale.setSelectedId(1,dontSendNotification);impulseScale.setTitle("IR: escala de amplitud; tiempo lineal");impulseScale.setTooltip(juce::String::fromUTF8("IR sintética demo. No procede de TF, sweep ni medición real. ETC: envolvente analítica; tiempo lineal."));
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
 setup(gain,gainLabel,String::fromUTF8("Ganancia · dB"),-18,12,.1,-2);setup(delay,delayLabel,String::fromUTF8("Delay · ms"),0,30,.01,3.2);
 setup(stacks,stacksLabel,"Stacks",1,24,1,12);setup(boxes,boxesLabel,"Cajas por stack",1,4,1,2);setup(spacing,spacingLabel,String::fromUTF8("Separación · m"),.2,5,.1,2.5);setup(rotation,rotationLabel,String::fromUTF8("Giro de escena · °"),-60,60,1,20);

 const Colour initialColours[]={Palette::green,Colour(0xff4d88ff),Colour(0xfff03baf),Colour(0xffffe33b)};
 for(int i=0;i<4;++i){auto idx=(size_t)i;traceSwitch[idx].openSettings=[this,i]{source.setSelectedId(i+1,sendNotificationSync);commands.invoke(202);};traceSwitch[idx].colour=initialColours[i];micNames[idx].setText("TF "+String(i+1),dontSendNotification);micNames[idx].setEditable(false,false,false);micNames[idx].setInterceptsMouseClicks(true,false);micNames[idx].setTitle("Nombre del motor "+String(i+1));micNames[idx].onTextChange=[this,i]{auto k=(size_t)i;if(micNames[k].getText().trim().isEmpty())micNames[k].setText("TF "+String(i+1),dontSendNotification);syncNames();};addAndMakeVisible(micNames[idx]);traceSwitch[idx].setTitle("Mostrar u ocultar MICRO "+String(i+1));traceSwitch[idx].onClick=[this,i]{status="MICRO "+String(i+1)+(traceSwitch[(size_t)i].getToggleState()?" visible":" oculto")+String::fromUTF8(" · solo visibilidad, no cambia el promedio");repaint();};addAndMakeVisible(traceSwitch[idx]);
 colourEdit[idx].setButtonText("...");colourEdit[idx].setTitle("Opciones TF "+String(i+1));colourEdit[idx].onClick=[this,i]{source.setSelectedId(i+1,sendNotificationSync);commands.invoke(202);};addAndMakeVisible(colourEdit[idx]);
 micDelay[idx].setRange(0,2000,.01);micDelay[idx].setSliderStyle(Slider::IncDecButtons);micDelay[idx].setTextBoxStyle(Slider::TextBoxLeft,false,82,22);micDelay[idx].setTextValueSuffix(" ms");micDelay[idx].setTitle("Delay de medicion MICRO "+String(i+1));micDelay[idx].onValueChange=[this,idx]{if(source.getSelectedId()==int(idx)+1)selectedDelay.setValue(micDelay[idx].getValue(),dontSendNotification);status=String::fromUTF8("Delay de medición manual · demostración · no obtenido con Find");repaint();};addAndMakeVisible(micDelay[idx]);}
 source.addItemList({String::fromUTF8("TF 1 · EMX-7150"),String::fromUTF8("TF 2 · EMX-7150"),String::fromUTF8("TF 3 · EMX-7150"),String::fromUTF8("TF 4 · EMX-7150"),String::fromUTF8("AVG · audiencia")},1);source.setSelectedId(1);source.onChange=[this]{frontCaptured[1].clear();frontCaptured[2].clear();int i=source.getSelectedId()-1;if(i>=0&&i<4)selectedDelay.setValue(micDelay[(size_t)i].getValue(),dontSendNotification);tfContent.repaint();repaint();};addAndMakeVisible(source);setWantsKeyboardFocus(true);syncNames();
 for(auto name:{"Spectrum","TF Mode","Decibel","RT60","Int. Loop","Timer","...","Find","...","Pink","...","Sweep","...","+ TF","+ AVG","Phase","...","Restaurar","Polaridad","Track todos","Real Time","Impulse","SPL",">","Stop","Pause","Config","View","On","Captura [Espacio]"})addMeasurementButton(name);
 const char* icons[]={"play","stop","pause"};const char* titles[]={"Iniciar todos","Detener todos","Pausar todos"};
 for(int n=0;n<3;++n){auto& b=*measurementButtons[(size_t)(23+n)];b.getProperties().set("transportIcon",icons[n]);b.setButtonText("");b.setTitle(titles[n]);b.setTooltip(titles[n]);}
 measurementButtons[9]->setButtonText("Pink [G]");measurementButtons[11]->setButtonText("Sweep [S]");
 measurementButtons[20]->setToggleState(true,dontSendNotification);
 for(auto* c:{&meterInput,&meterWeight,&generatorOutput}){addAndMakeVisible(c);c->setEnabled(false);}
 
 meterWeight.addItem("dB SPL A Fast",1);meterWeight.setSelectedId(1);meterWeight.setTooltip("Maqueta de medidor. Ponderaciones y calibracion pendientes.");
 generatorLevel.setText("-20 dBFS",dontSendNotification);generatorLevel.setJustificationType(Justification::centred);addAndMakeVisible(generatorLevel);
 measurementButtons[7]->setButtonText("Find L / F");measurementButtons[7]->setTooltip("L: tiempo individual. F: volver a buscar el tiempo de la TF seleccionada.");
 measurementButtons[1]->setToggleState(true,dontSendNotification);
 measurementButtons[6]->setTooltip("Ajustes de Timer");measurementButtons[8]->setTooltip("Advanced Delay Finder");measurementButtons[10]->setTooltip("Pink: Gain / HPF / LPF");measurementButtons[12]->setTooltip("Sweep: Gain / HPF / LPF");measurementButtons[16]->setTooltip("Resultados de Auto Phase");

 plotChoice.addItem("All",1);plotChoice.addItem("2 Plots",2);plotChoice.setSelectedId(1);addAndMakeVisible(plotChoice);
 plotChoice.onChange=[this]{persistTF();resized();repaint();};
 averageChoice.addItemList({"1","2","4","5","8","16","32","64"},1);averageChoice.setSelectedId(4);addAndMakeVisible(averageChoice);
 averageChoice.setTooltip("Profundidad de promedio solicitada. Motor pendiente.");
 for(auto* slider:{&selectedDelay,&phasePreview}){slider->setRange(0,2000,.01);slider->setSliderStyle(Slider::IncDecButtons);slider->setTextBoxStyle(Slider::TextBoxLeft,false,86,26);slider->setTextValueSuffix(" ms");addAndMakeVisible(slider);}
 selectedDelay.setTitle("Delay de referencia seleccionado");
 selectedDelay.onValueChange=[this]{int i=source.getSelectedId()-1;if(i>=0&&i<4)micDelay[(size_t)i].setValue(selectedDelay.getValue());};
 phasePreview.setEnabled(false);phasePreview.setTooltip("Auto Phase pendiente");
 tfContent.addMouseListener(this,true);tfViewport.setViewedComponent(&tfContent,false);tfViewport.setScrollBarsShown(true,false);tfViewport.setScrollBarThickness(10);addAndMakeVisible(tfViewport);
 for(int i=0;i<4;++i){auto k=(size_t)i;for(auto* c:std::initializer_list<Component*>{&micNames[k],&traceSwitch[k],&colourEdit[k],&micDelay[k]})tfContent.addAndMakeVisible(c);
 const char* labels[]={"Find","Track","AG",">"};for(int j=0;j<4;++j){auto& b=tfActions[k][(size_t)j];b.setButtonText(labels[j]);if(j==3){b.getProperties().set("transportIcon","play");b.setButtonText("");b.setTitle("Iniciar TF "+String(i+1));b.setTooltip("Iniciar TF "+String(i+1));}tfContent.addAndMakeVisible(b);}}
 for(size_t k=0;k<4;++k){tfContent.addAndMakeVisible(inputMeters[k]);tfActions[k][1].getProperties().set("trackCircle",true);}
 measurementButtons[19]->getProperties().set("trackCircle",true);measurementButtons[19]->getProperties().set("trackColour",Palette::cyan.toString());
 tfContent.draw=[this](Graphics& g){int w=tfContent.getWidth();for(int i=0;i<4;++i){int y=i*CompactLayout::tfRow;bool selected=source.getSelectedId()==i+1;g.setColour(selected?Colour(0xff35434d):Colour(0xff192126));g.fillRoundedRectangle(0,(float)y,(float)w,(float)CompactLayout::tfCard,4);g.setColour(traceSwitch[(size_t)i].colour.withAlpha(selected?1.f:.55f));g.drawRoundedRectangle(1.5f,(float)y+1.5f,(float)w-3.f,(float)CompactLayout::tfCard-3.f,4,selected?3.f:1.f);

 }};

 deviceWatcher.addChangeListener(this);audioEndpoints=discoverAudioEndpoints();syncNames();setupCommands();refreshMeterSources();
 addAndMakeVisible(tracesPage);addAndMakeVisible(metersPage);commands.bind(tracesPage,960);commands.bind(metersPage,961);addAndMakeVisible(dockMeters);dockMeters.toggleRequested=[this]{commands.invoke(203);};meterListener=meterState.listen([this]{repaint();menuItemsChanged();});for(size_t i=0;i<graphGear.size();++i){auto& gear=graphGear[i];gear.setButtonText("...");gear.getProperties().set("commandGlyph",true);commands.bind(gear,i==0?963:303);addAndMakeVisible(gear);}
 captureLibrary.adoptLegacy(projectFile.getParentDirectory().getChildFile("Capturas"));
 libraryPanel.onCommand=[this](int id){commands.invoke(id);};
 libraryPanel.commandText=[this](int id){auto* c=MyaCommands::find(id);return c?commands.details(*c,MyaCommands::state(*c,commands.current())):String{};};
 libraryPanel.onChange=[this]{refreshLibrary();};libraryPanel.onSelection=[this]{repaint();menuItemsChanged();};
 libraryPanel.onFront=[this](String id){auto* n=captureLibrary.find(id);if(n&&n->visible&&n->kind==CaptureLibrary::Kind::SnapshotTF){frontCaptured[1]=frontCaptured[2]=id;repaint();}};
 libraryPanel.onError=[this](String message){status=message;repaint();};
 addAndMakeVisible(libraryPanel);restoreLibraryButton.getProperties().set("commandGlyph",true);commands.bind(restoreLibraryButton,405);addAndMakeVisible(restoreLibraryButton);
 const int aiIDs[]={814,965,810,812,813,966,967};for(size_t i=0;i<aiButtons.size();++i){commands.bind(aiButtons[i],aiIDs[i],true);addAndMakeVisible(aiButtons[i]);}commands.bind(sumButton,968,true);addAndMakeVisible(sumButton);
 peakSettingsButton.setButtonText("Pico retenido / ajustes");peakSettingsButton.onClick=[this]{inputMeterOptions();};addAndMakeVisible(peakSettingsButton);
 commands.bind(closeLeft,405);commands.bind(closeRight,406);commands.bind(restoreRight,406);for(auto* b:{&closeLeft,&closeRight,&restoreRight}){b->getProperties().set("commandGlyph",true);addAndMakeVisible(b);}
 addAndMakeVisible(leftDivider);addAndMakeVisible(rightDivider);leftDivider.moved=[this](int x){leftWidth=jlimit(WorkspaceViewState::leftMin,WorkspaceViewState::leftMax,x-8);resized();repaint();persistWorkspace();};rightDivider.moved=[this](int x){rightWidth=jlimit(WorkspaceViewState::rightMin,WorkspaceViewState::rightMax,getWidth()-x-8);resized();repaint();persistWorkspace();};
 if(auto state=WorkspaceViewState::read(projectFile.getParentDirectory().getChildFile("vista-trabajo.json"))){leftWidth=state->leftWidth;rightWidth=state->rightWidth;leftVisible=state->leftVisible;rightVisible=state->rightVisible;inputMeterSettings=state->meters;}reflectInputMeters();
 libraryPanel.refresh();refreshLibrary();if(auto graph=GraphViewState::read(graphViewFile()))restoreGraphChoices(*graph);if(auto tf=TFDisplayState::read(tfSettingsFile())){tfSettings=*tf;reflectTF();}else tfSettings.linkPhaseMag=linkSmooth.getToggleState();registerKeys(*this);setSize(1440,960);select(0);
 }
 void changeListenerCallback(ChangeBroadcaster*) override {audioEndpoints=discoverAudioEndpoints();syncNames();refreshMeterSources();if(settingsDrawer&&settingsDrawer->isVisible()){auto key=drawerKey;settingsDrawer->setVisible(false);if(key=="audio")audioSettings();else if(key.startsWith("TF "))engineSettings(key.substring(3).getIntValue()-1);}status=String::fromUTF8("Dispositivos de audio actualizados · adquisición desactivada");repaint();}
 ~Workstation() override {graphWindow.reset();meters.reset();meterState.unlisten(meterListener);deviceWatcher.removeChangeListener(this);tfContent.removeMouseListener(this);setLookAndFeel(nullptr);}
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
  return PlotPresentation::frontTrace(plotMic[(size_t)type].getSelectedId(),source.getSelectedId()-1,visible);
 }
 bool validTF(double u,int index) const {
  return PlotPresentation::validTF(u,index,tfSettings.coherence(demoCoherence(std::log(tfSettings.frequency(u)/20.)/std::log(1000.),index))*100,coherenceThreshold.getValue());
 }
 std::optional<double> plotValue(double u,int index,int type){
  if(index<0)return std::nullopt;
  if(type==0)return impulseDisplay.normalized(u,irScale);
  if(!validTF(u,index))return std::nullopt;
  int kind=displayType(type);auto h=cursorResponse(tfSettings.frequency(u),index,kind,type);
  if(!std::isfinite(h.real())||!std::isfinite(h.imag())||std::abs(h)<=1.e-15)return std::nullopt;
  return tfSettings.normalize(kind==1?std::arg(h)*180/MathConstants<double>::pi:20*std::log10(std::abs(h)),kind);
 }
 void updateCursorReadout(){
  cursorReadout.parts.clear();auto add=[this](String value,Colour colour){cursorReadout.parts.emplace_back(value,colour);};
  if(cursorPlot<0){cursorReadout.setTooltip({});add("Frecuencia -- Hz",Palette::cyan);add("Magnitud -- dB",Palette::green);add(String::fromUTF8("Fase --°"),Colour(0xfff03baf));add("Coh -- %",Colour(0xffff4040));return;}
  if(cursorPlot==0){auto value=frontTrace(0)>=0?impulseDisplay.physical(cursorU,irScale):std::nullopt;add(String(ImpulseDisplay::timeMs(cursorU),2)+" ms",Palette::cyan);add(String("IR DEMO ")+String(irScale==0?"Lin":irScale==1?"Log":"ETC")+" "+(value?String(*value,irScale==0?3:2)+String(irScale==0?" Lin":" dB ref 1"):String("--")),Palette::orange);cursorReadout.setTooltip("IR sintetica demo; tiempo lineal");return;}
  String full=String(tfSettings.frequency(cursorU),1)+" Hz";add(full,Palette::cyan);String cohText="Coh";
  if(tfSettings.coherenceSquared)cohText+=String::fromUTF8("²");
  for(int panel:{2,1}){int kind=displayType(panel);auto v=panelValue(cursorU,panel);auto coh=panelCoherence(cursorU,panel);String name=panelName(panel),shortName=name.length()>10?name.substring(0,9)+String::fromUTF8("…"):name;String unit=kind==2?" dB":String::fromUTF8("°");String reading=(v?String(tfSettings.physical(*v,kind),kind==2?2:1)+unit:String("--"));
   add(String(kind==2?"Mag ":"Fase ")+"P"+String(panel)+" "+shortName+" "+reading,kind==2?Palette::green:Colour(0xfff03baf));
   auto c=coh?String(tfSettings.coherence(*coh)*100,1)+"%":String("--");cohText+="  P"+String(panel)+" "+c;full+="\nP"+String(panel)+" "+name+": "+reading+" Coh "+c;
  }add(cohText,Colour(0xffff4040));cursorReadout.setTooltip(full);
 }
 void mouseExit(const MouseEvent& event) override {
  auto point=event.getEventRelativeTo(this).getPosition();
  for(int t=0;t<3;++t)if((t!=0||plotChoice.getSelectedId()!=2)&&plotArea(plotBounds(t)).contains(point))return;
  cursorPlot=-1;plotCursorVisible.fill(false);updateCursorReadout();repaint();
 }
 void mouseMove(const MouseEvent& event) override{
 if(mode!=0)return;auto point=event.getEventRelativeTo(this).getPosition();
 for(int t=0;t<3;++t){if(t==0&&plotChoice.getSelectedId()==2)continue;auto p=plotArea(plotBounds(t));if(!p.contains(point))continue;
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
 if(event.originalComponent==this&&mode!=2){for(int t=0;t<3;++t)if((t!=0||plotChoice.getSelectedId()!=2)&&activeGraphBounds(t).contains(event.getPosition())){activePlot=t;grabKeyboardFocus();status="Grafica seleccionada / flechas izquierda/derecha: zoom X / + y -: zoom Y / doble clic: centrar";repaint();return;}}
 auto* origin=event.originalComponent;
 if(origin!=&tfContent&&!tfContent.isParentOf(origin))return;
 auto point=event.getEventRelativeTo(&tfContent).getPosition();
 int index=point.y/CompactLayout::tfRow;
 if(point.x<0||point.x>=tfContent.getWidth()||point.y<0||index>=4||point.y%CompactLayout::tfRow>=CompactLayout::tfCard)return;
 source.setSelectedId(index+1,sendNotificationSync);
 status=micNames[(size_t)index].getText()+String::fromUTF8(" seleccionada · L / F: tiempo individual · Espacio: captura");
 tfContent.repaint();repaint();
 if(origin==&tfContent)grabKeyboardFocus();
 }

 void mouseDoubleClick(const MouseEvent& event) override {
 if(event.originalComponent==this&&mode!=2){for(int t=0;t<3;++t)if((t!=0||plotChoice.getSelectedId()!=2)&&activeGraphBounds(t).contains(event.getPosition())){plotPanX[(size_t)t]=plotPanY[(size_t)t]=0;plotZoomX[(size_t)t]=plotZoomY[(size_t)t]=1.;activePlot=t;repaint();return;}}
 auto* origin=event.originalComponent;
 if(origin!=&tfContent&&!tfContent.isParentOf(origin))return;
 auto point=event.getEventRelativeTo(&tfContent).getPosition();
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
 if(c=='e'&&!command&&!mods.isAltDown()){
  int id=meterSequence.press(Time::getMillisecondCounterHiRes(),true);if(id)commands.invoke(id);return true;
 }
 meterSequence.reset();int id=0;
 if(key==KeyPress::escapeKey&&!command&&!mods.isAltDown())id=602;
 else if(mode!=2&&!command&&!mods.isAltDown()&&key==KeyPress::leftKey)id=856;
 else if(mode!=2&&!command&&!mods.isAltDown()&&key==KeyPress::rightKey)id=857;
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
 void resized() override {if(buttons.size()<19)return;int excess=jmax(0,680-centerWidth());if(rightVisible){int reduction=jmin(excess,rightWidth-WorkspaceViewState::rightMin);rightWidth-=reduction;excess-=reduction;}if(leftVisible)leftWidth-=jmin(excess,leftWidth-WorkspaceViewState::leftMin);tracesPage.setVisible(leftVisible);metersPage.setVisible(leftVisible);int lw=leftWidth-8;tracesPage.setBounds(12,91,(lw-2)/2,24);metersPage.setBounds(14+(lw-2)/2,91,(lw-2)/2,24);libraryPanel.setVisible(leftVisible&&!meterState.state().sidebarMeters);libraryPanel.setBounds(12,118,lw,getHeight()-290);dockMeters.setVisible(leftVisible&&meterState.state().sidebarMeters);dockMeters.setBounds(12,118,lw,getHeight()-290);restoreLibraryButton.setVisible(!leftVisible);restoreLibraryButton.setBounds(0,92,18,24);
 closeLeft.setVisible(leftVisible);closeLeft.setBounds(leftWidth+4,83,18,22);closeLeft.setButtonText("<");closeRight.setVisible(rightVisible);closeRight.setBounds(getWidth()-rightWidth-24,83,18,22);closeRight.setButtonText(">");restoreRight.setVisible(!rightVisible);restoreRight.setBounds(getWidth()-18,92,18,24);restoreRight.setButtonText("<");leftDivider.setVisible(leftVisible);leftDivider.setBounds(leftWidth+8,112,6,getHeight()-150);rightDivider.setVisible(rightVisible);rightDivider.setBounds(getWidth()-rightWidth-16,112,6,getHeight()-150);
 for(size_t i=0;i<aiButtons.size();++i){aiButtons[i].setVisible(leftVisible);int row=(int)i/2;aiButtons[i].setBounds(12+(int(i)%2)*((lw+2)/2),getHeight()-162+row*28,i==6?lw:(lw-2)/2,24);}sumButton.setVisible(mode==0);sumButton.setBounds(centerX()+centerWidth()-90,getHeight()-61,82,22);
 int x=12;for(int i=0;i<8;++i){int w=i==2?175:(i==6?115:95);buttons[(size_t)i]->setBounds(x,43,w,29);x+=w+6;}int left=14;for(int i=8;i<12;++i)buttons[(size_t)i]->setBounds(left+(i-8)%2*83,getHeight()-170+(i-8)/2*36,78,30);
 int right=getWidth()-rightWidth;gainLabel.setBounds(right,105,200,22);gain.setBounds(right,128,200,30);delayLabel.setBounds(right,169,200,22);delay.setBounds(right,192,200,30);
 buttons[12]->setBounds(right,234,98,30);buttons[13]->setBounds(right+103,234,97,30);buttons[14]->setBounds(right,276,200,30);buttons[15]->setBounds(right,312,200,30);buttons[16]->setBounds(right,348,200,30);buttons[17]->setBounds(right,384,200,30);buttons[18]->setBounds(right,420,200,30);
 for(int i=8;i<12;++i)buttons[(size_t)i]->setVisible(false);
 for(int i=12;i<19;++i)buttons[(size_t)i]->setVisible(rightVisible);
 gain.setVisible(rightVisible);gainLabel.setVisible(rightVisible);delay.setVisible(rightVisible);delayLabel.setVisible(rightVisible);
 source.setBounds(centerX()+8,91,jmax(120,centerWidth()-250),28);source.setVisible(mode!=0);cursorReadout.setVisible(mode==0);cursorReadout.setBounds(centerX()+8,87,jmax(120,centerWidth()-16),24);
 linkSmooth.setVisible(false);
 for(int i=0;i<2;++i){auto r=plotBounds(i);plotDividers[(size_t)i].setVisible(mode==0&&(i!=0||plotChoice.getSelectedId()!=2));plotDividers[(size_t)i].setBounds(r.getX(),r.getBottom(),r.getWidth(),8);}
 for(int t=0;t<3;++t){auto r=plotBounds(t);auto& c=plotMic[(size_t)t];c.setVisible(mode==0&&(t!=0||plotChoice.getSelectedId()!=2));c.setBounds(r.getRight()-115,r.getY()+2,110,18);}
 int magnitudePanel=plotTypes[1]==2?2:plotTypes[0]==2?1:-1;auto magArea=plotBounds(magnitudePanel>0?magnitudePanel:2);
 coherenceThreshold.setVisible(false);coherenceThreshold.setBounds(magArea.getRight()-12,magArea.getY()+26,10,jmax(20,roundToInt((magArea.getHeight()-48)*tfSettings.coherenceHeight)));showCoherence.setVisible(mode==0&&magnitudePanel>0);showCoherence.setBounds(magArea.getX()+175,magArea.getY()+2,95,18);
 impulseScale.setVisible(mode==0&&plotChoice.getSelectedId()!=2);auto irArea=plotBounds(0);impulseScale.setBounds(irArea.getX()+5,irArea.getY()+2,95,18);
 for(int t=0;t<2;++t){auto area=plotBounds(t+1);for(int kind:{1,2}){auto* c=MyaCommands::find((t==0?940:950)+(kind==1?1:0));plotTypeChoice[(size_t)t].changeItemText(kind,commands.title(*c));}plotTypeChoice[(size_t)t].setVisible(mode==0);plotTypeChoice[(size_t)t].setBounds(area.getX()+5,area.getY()+2,140,18);plotSmooth[(size_t)t].setTitle(String(displayType(t+1)==1?"Fase / Phase":"Magnitud / Magnitude")+juce::String::fromUTF8(" · suavizado / smoothing"));}
 for(int t=0;t<3;++t){auto r=activeGraphBounds(t);graphGear[(size_t)t].setVisible(mode!=2&&(t!=0||plotChoice.getSelectedId()!=2));graphGear[(size_t)t].setBounds(r.getRight()-20,r.getBottom()-18,16,16);}
 for(int t=0;t<2;++t){auto r=plotBounds(t+1);plotSmooth[(size_t)t].setVisible(mode==0);auto box=CompactLayout::smoothing(r.getWidth());plotSmooth[(size_t)t].setBounds(r.getX()+box.x,r.getY()+box.y,box.w,box.h);}

 const bool measuring=mode==0&&rightVisible;
 for(auto& b:measurementButtons)b->setVisible(measuring);
 plotChoice.setVisible(measuring);averageChoice.setVisible(measuring);selectedDelay.setVisible(measuring);phasePreview.setVisible(measuring);tfViewport.setVisible(measuring);
 if(mode==0){for(int i=12;i<19;++i)buttons[(size_t)i]->setVisible(false);gain.setVisible(false);gainLabel.setVisible(false);delay.setVisible(false);delayLabel.setVisible(false);}
 measurementButtons[29]->setVisible(false);
 int rx=getWidth()-290;
 auto mb=[&](int i,int bx,int y,int w,int h=27){measurementButtons[(size_t)i]->setBounds(bx,y,w,h);};
 for(auto* c:std::initializer_list<Component*>{&meterInput,&meterWeight,&generatorOutput,&generatorLevel})c->setVisible(measuring);
 mb(20,rx,94,98);mb(21,rx+102,94,82);mb(22,rx+188,94,76);
 meterInput.setBounds(rx+134,133,130,22);meterWeight.setBounds(rx+98,159,166,22);
 mb(1,rx,195,176);mb(0,rx+180,195,84);
 averageChoice.setBounds(rx+196,230,68,25);
 mb(23,rx,266,56);mb(24,rx+62,266,56);mb(25,rx+124,266,56);mb(19,rx+186,266,30);mb(26,rx+222,266,42);measurementButtons[5]->setVisible(false);
 int listEnd=CompactLayout::tfBottom(getHeight());
 tfViewport.setTransform({});tfViewport.setBounds(getWidth()-rightWidth+0,CompactLayout::tfTop,rightWidth-16,CompactLayout::tfViewportHeight(getHeight()));tfContent.setSize(rightWidth-28,CompactLayout::tfContentHeight);
 mb(13,rx,listEnd+6,174);mb(14,rx+180,listEnd+6,84);
 mb(27,rx,listEnd+40,82);mb(6,rx+88,listEnd+40,30);mb(3,rx+124,listEnd+40,68);mb(2,rx+198,listEnd+40,66);
 peakSettingsButton.setVisible(measuring);peakSettingsButton.setTransform(rightTransform());peakSettingsButton.setBounds(rx,listEnd+73,264,22);
 int extras=getHeight()-192;
 mb(7,rx,extras,82);mb(8,rx+86,extras,28);mb(15,rx+120,extras,70);mb(16,rx+194,extras,28);mb(18,rx+226,extras,38);
 measurementButtons[18]->setButtonText(String::fromUTF8("Ø"));
 mb(17,rx,extras+33,82);mb(4,rx+88,extras+33,88);plotChoice.setBounds(rx+182,extras+33,82,27);
 selectedDelay.setVisible(false);phasePreview.setVisible(false);
 int gen=getHeight()-104;mb(9,rx,gen,82);mb(11,rx+88,gen,82);mb(28,rx+214,gen,50);
 mb(10,rx+174,gen,30);mb(12,rx+230,gen+33,30);generatorOutput.setBounds(rx,gen+33,124,27);generatorLevel.setBounds(rx+128,gen+33,96,27);
 for(int i=0;i<4;++i){auto k=(size_t)i;int y=i*CompactLayout::tfRow,w=tfContent.getWidth();micNames[k].setBounds(27,y+2,w-92,22);traceSwitch[k].setBounds(1,y+2,25,23);colourEdit[k].setBounds(w-61,y+3,27,21);tfActions[k][3].setBounds(w-31,y+3,28,21);
 inputMeters[k].setBounds(5,y+28,w-10,56);micDelay[k].setSliderStyle(Slider::LinearHorizontal);micDelay[k].setTextBoxStyle(Slider::TextBoxLeft,false,82,20);micDelay[k].setBounds(6,y+86,w-81,20);tfActions[k][0].setBounds(w-72,y+86,39,20);tfActions[k][0].getProperties().set("commandGlyph",true);tfActions[k][0].setButtonText("Find");tfActions[k][1].setVisible(true);tfActions[k][1].setBounds(w-30,y+84,26,26);tfActions[k][2].setVisible(false);}
 auto compactRight=rightTransform();
 for(size_t i=0;i<measurementButtons.size();++i)if(i!=29)measurementButtons[i]->setTransform(compactRight);
 for(auto* c:std::initializer_list<Component*>{&meterInput,&meterWeight,&generatorOutput,&generatorLevel,&averageChoice,&plotChoice})c->setTransform(compactRight);
 auto compactLeft=AffineTransform::scale(.82f,1.f).translated(2.5f,0);for(int i=8;i<12;++i)buttons[(size_t)i]->setTransform(compactLeft);
 const bool sceneMode=mode==2&&rightVisible;stacks.setVisible(sceneMode);boxes.setVisible(sceneMode);spacing.setVisible(sceneMode);rotation.setVisible(sceneMode);stacksLabel.setVisible(sceneMode);boxesLabel.setVisible(sceneMode);spacingLabel.setVisible(sceneMode);rotationLabel.setVisible(sceneMode);
 Slider* ss[]={&stacks,&boxes,&spacing,&rotation};Label* ll[]={&stacksLabel,&boxesLabel,&spacingLabel,&rotationLabel};for(int i=0;i<4;++i){ll[i]->setBounds(right,473+i*64,200,23);ss[i]->setBounds(right,496+i*64,200,28);}positionDrawer();}
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

 g.saveState();g.reduceClipRegion(p);if(mode==0)drawSaved(g,p,type,false,panX,panY,zoomX,zoomY);auto order=PlotPresentation::drawOrder(mode==0?frontTrace(type):-1);for(int layer=0;layer<(mode==0?4:(type==0?1:2));++layer){int s=mode==0?order[(size_t)layer]:layer;if(mode==0&&(!traceSwitch[(size_t)s].getToggleState()||(plotMic[(size_t)type].getSelectedId()>1&&plotMic[(size_t)type].getSelectedId()!=s+2)))continue;Path path;double previous=0;bool gap=true;int points=jmax(700,p.getWidth()*2);
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
  if(u>=0&&u<=1){if(savedFront.isNotEmpty())coherence=savedTraces.coherence(savedFront,tfSettings.frequency(u));else if(index>=0)coherence=demoCoherence(std::log(tfSettings.frequency(u)/20.)/std::log(1000.),index);}
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
 }}g.restoreState();if(mode==0&&kind==0)text(g,juce::String::fromUTF8("IR SINTÉTICA · ms"),{r.getX()+110,r.getY()+2,jmax(1,r.getWidth()-240),20},11,Palette::orange);else if(mode!=0)text(g,label,{r.getX()+9,r.getY()+2,r.getWidth()-18,20},12,Palette::muted);}
 void scene(Graphics& g,Rectangle<int> r){g.setColour(Palette::plot);g.fillRect(r);auto a=rotation.getValue()*MathConstants<double>::pi/180.;auto proj=[&](double x,double y,double z){return Point<float>((float)(r.getCentreX()+(x*std::cos(a)-y*std::sin(a))*r.getWidth()/85.),(float)(r.getY()+90+(x*std::sin(a)+y*std::cos(a))*6-z*8));};for(int i=-20;i<=20;i+=2){g.setColour(Colour(0xff29414c));g.drawLine({proj(i,0,0),proj(i,40,0)});}for(int j=0;j<=40;j+=2)g.drawLine({proj(-20,j,0),proj(20,j,0)});Path stage;stage.startNewSubPath(proj(-17,-7,0));stage.lineTo(proj(17,-7,0));stage.lineTo(proj(17,0,0));stage.lineTo(proj(-17,0,0));stage.closeSubPath();g.setColour(Colour(0xff233b48));g.fillPath(stage);int n=(int)stacks.getValue(),b=(int)boxes.getValue();for(int i=0;i<n;++i)for(int k=0;k<b;++k){auto p=proj((i-(n-1)/2.)*spacing.getValue(),0,k*1.1);g.setColour(Palette::orange);g.fillRect(p.x-6,p.y-10,12.f,10.f);g.setColour(Palette::plot);g.drawRect(p.x-6,p.y-10,12.f,10.f);}for(int side:{-1,1})for(int k=0;k<8;++k){auto p=proj(side*17,0,7+k*.7);g.setColour(Palette::cyan);g.drawRect(p.x-5,p.y-5,10.f,5.f);}text(g,String::fromUTF8("GEOMETRÍA 3D · ")+String(n)+String::fromUTF8(" stacks × ")+String(b)+" cajas = "+String(n*b)+" recintos",r.removeFromTop(30).reduced(10,0),13,Palette::cyan);text(g,String::fromUTF8("Sin mapa SPL: solver acústico pendiente"),{r.getX()+10,r.getBottom()-32,r.getWidth()-20,24},12,Palette::orange);}
 void paint(Graphics& g) override {commands.refresh();if(mode==0)updateCursorReadout();g.fillAll(Palette::bg);g.setGradientFill(ColourGradient(Colour(0xff43505c),0,0,Colour(0xff141a20),0,37,false));g.fillRect(0,0,getWidth(),37);text(g,"MYA ACUSTICA",{15,2,230,32},19);text(g,String::fromUTF8("0.3 · MYA-005 · PROTOTIPO SIN AUDIO"),{getWidth()-370,2,355,32},12,Palette::orange);
 if(leftVisible)panel(g,{8,82,leftWidth,getHeight()-115});panel(g,{centerX(),82,centerWidth(),getHeight()-115});if(rightVisible)panel(g,{getWidth()-rightWidth-8,82,rightWidth,getHeight()-115});

 Rectangle<int> center{centerX()+8,118,centerWidth()-16,getHeight()-183};if(mode==2){scene(g,center);if(adjustmentLinked)text(g,"DSP vinculado: "+String(gain.getValue(),1)+" dB / "+String(delay.getValue(),2)+String::fromUTF8(" ms / Ø ")+String(polarity?"ON":"OFF"),{center.getX()+10,center.getY()+35,center.getWidth()-20,25},12,Palette::green);}else if(mode==0){if(plotChoice.getSelectedId()!=2)graph(g,plotBounds(0),String::fromUTF8("IMPULSE RESPONSE · sintética · ms"),0);graph(g,plotBounds(1),displayName(1),1);graph(g,plotBounds(2),displayName(2),2);}else{int usable=center.getHeight()-(mode==1||mode==3?100:0);int irHeight=plotChoice.getSelectedId()==2?0:104;if(irHeight)graph(g,{center.getX(),center.getY(),center.getWidth(),90},String::fromUTF8("IMPULSE RESPONSE · sintética · ms"),0);int gh=(usable-irHeight)/2;graph(g,{center.getX(),center.getY()+(irHeight?97:0),center.getWidth(),gh},String::fromUTF8("PHASE · grados"),1);graph(g,{center.getX(),center.getY()+irHeight+gh,center.getWidth(),gh},(mode==0?String::fromUTF8("MAGNITUDE · dB / Hz · demo"):String::fromUTF8("MAGNITUDE · dB / Hz · cian original · verde procesada")),2);if(mode==1||mode==3){int yy=center.getBottom()-85;text(g,String::fromUTF8("CADENA ACTIVA: GANANCIA → DELAY → POLARIDAD"),{center.getX()+8,yy,center.getWidth()-16,26},12,Palette::cyan);text(g,String::fromUTF8("EQ / crossover / all-pass / FIR: implementación pendiente"),{center.getX()+8,yy+29,center.getWidth()-16,25},12,Palette::muted);}}
 if(mode==0&&rightVisible){g.saveState();g.addTransform(rightTransform());int x=getWidth()-290;panel(g,{x-6,127,276,60});text(g,"--",{x+10,137,80,28},24,Palette::ink);text(g,"Sin audio / sin calib.",{x+8,169,86,14},9,Palette::muted);
 panel(g,{x-6,191,276,getHeight()-420});text(g,"Averaging Depth",{x+8,232,180,23},12);
 int extras=getHeight()-192;text(g,"ALINEAMIENTO / MYA",{x+4,extras-22,260,19},11,Palette::cyan);
 int gen=getHeight()-104;panel(g,{x-6,gen-32,276,98});text(g,"SIGNAL GENERATOR / sin audio",{x+4,gen-28,256,22},12,Palette::cyan);g.restoreState();
 }
 text(g,status,{12,getHeight()-28,getWidth()-24,24},12,Palette::muted);
 }
};
class MainWindow : public DocumentWindow {
public:
 MainWindow():DocumentWindow(String::fromUTF8("Mya Acustica · prototipo 0.3 · MYA-005"),Palette::bg,allButtons){
 setUsingNativeTitleBar(true);auto* workstation=new Workstation();setContentOwned(workstation,true);
 #if JUCE_MAC
 MenuBarModel::setMacMainMenu(workstation);
 #else
 setMenuBar(workstation);
 #endif
 setResizable(true,false);setResizeLimits(1120,900,3000,2000);centreWithSize(1440,960);setVisible(true);getContentComponent()->grabKeyboardFocus();}
 ~MainWindow()override{
 #if JUCE_MAC
 MenuBarModel::setMacMainMenu(nullptr);
 #else
 setMenuBar(nullptr);
 #endif
 }
 void closeButtonPressed()override{JUCEApplication::getInstance()->systemRequestedQuit();}
};
class MyaApp:public JUCEApplication {std::unique_ptr<MainWindow> window;public:const String getApplicationName()override{return "Mya Acustica";}const String getApplicationVersion()override{return "0.3.0";}void initialise(const String&)override{window=std::make_unique<MainWindow>();}void shutdown()override{window.reset();}};
#if !defined(MYA_OFFSCREEN_RENDER)
START_JUCE_APPLICATION(MyaApp)
#endif
