#include "TransferFunctionPanel.h"
#include "PlotPresentation.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr double minHz=20.0,maxHz=20000.0;
constexpr float minDb=-60.0f,maxDb=30.0f;
const juce::Colour background{0xff080a0c},grid{0xff2c3135},text{0xffa9bac8};

}

class TransferFunctionPanel::PlotPane : public juce::Component {
public:
 PlotPane(TransferFunctionPanel& panel,bool isPhase):owner(panel),phase(isPhase){setInterceptsMouseClicks(false,false);}
 void paint(juce::Graphics& g) override{owner.paintPane(g,getLocalBounds().toFloat(),phase);}
private:
 TransferFunctionPanel& owner;
 bool phase;
};

TransferFunctionPanel::TransferFunctionPanel(DataProvider provider):dataProvider(std::move(provider)){
 phasePane=std::make_unique<PlotPane>(*this,true);magnitudePane=std::make_unique<PlotPane>(*this,false);
 addAndMakeVisible(*phasePane);addAndMakeVisible(*magnitudePane);
 speed.addItemList({"Fast","Slow","Prom.16"},1);speed.setSelectedId(1);speed.setTooltip("Respuesta temporal de magnitud, fase y coherencia");speed.onChange=[this]{history.clear();lastUpdateMs=0;};addAndMakeVisible(speed);
}
void TransferFunctionPanel::setDisplayOptions(bool multi,int smooth){if(multiWindowMode!=multi){multiWindowMode=multi;if(onResolutionChanged)onResolutionChanged(multi);}smoothingMode=juce::jlimit(1,6,smooth);history.clear();}
TransferFunctionPanel::~TransferFunctionPanel(){stopTimer();}

void TransferFunctionPanel::setActive(bool shouldBeActive){
 active=shouldBeActive;
 if(active&&isShowing()){timerCallback();startTimerHz(30);}
 else {stopTimer();traces.clear();history.clear();lastUpdateMs=0;phasePane->repaint();magnitudePane->repaint();}
}

void TransferFunctionPanel::visibilityChanged(){
 if(!isShowing())stopTimer();
 else if(active){timerCallback();startTimerHz(30);}
}

void TransferFunctionPanel::timerCallback(){
 auto incoming=dataProvider?dataProvider():std::vector<Trace>{};
 double now=juce::Time::getMillisecondCounterHiRes(),dt=lastUpdateMs>0?juce::jlimit(0.0,1.0,(now-lastUpdateMs)/1000.):0.;lastUpdateMs=now;
 double tau=speed.getSelectedId()==2?1.:speed.getSelectedId()==3?5.4:.125;
 double alpha=dt>0?1.-std::exp(-dt/tau):1.;
 std::map<int,Trace> next;
 for(auto& trace:incoming){auto old=history.find(trace.id);if(old!=history.end()&&old->second.samples.size()==trace.samples.size())for(size_t i=0;i<trace.samples.size();++i){auto& current=trace.samples[i];const auto& previous=old->second.samples[i];if(!current.valid||!previous.valid)continue;current.magnitudeDb=previous.magnitudeDb+alpha*(current.magnitudeDb-previous.magnitudeDb);current.phaseDegrees=previous.phaseDegrees+alpha*std::remainder(current.phaseDegrees-previous.phaseDegrees,360.);current.phaseDegrees=std::remainder(current.phaseDegrees,360.);current.coherence=previous.coherence+alpha*(current.coherence-previous.coherence);}next[trace.id]=trace;}
 history=std::move(next);traces=std::move(incoming);
 phasePane->repaint();magnitudePane->repaint();
}

void TransferFunctionPanel::resized(){speed.setBounds(juce::jmax(8,getWidth()-464),2,104,21);}

void TransferFunctionPanel::setPlotLayout(juce::Rectangle<int> phase,juce::Rectangle<int> magnitude){
 phasePane->setBounds(phase);magnitudePane->setBounds(magnitude);speed.toFront(false);
}

void TransferFunctionPanel::setPlotTransforms(const std::array<double,3>& newPanX,const std::array<double,3>& newPanY,
                                             const std::array<double,3>& newZoomX,const std::array<double,3>& newZoomY){
 panX=newPanX;panY=newPanY;zoomX=newZoomX;zoomY=newZoomY;
 phasePane->repaint();magnitudePane->repaint();
}

std::optional<float> TransferFunctionPanel::frequencyPosition(double hz,juce::Rectangle<float> area,bool phase)const{
 if(!std::isfinite(hz)||hz<minHz||hz>maxHz||area.isEmpty())return {};
 auto index=phase?1u:2u;
 double u=std::log(hz/minHz)/std::log(maxHz/minHz);
 return area.getX()+static_cast<float>(PlotPresentation::screen(u,panX[index],zoomX[index]))*area.getWidth();
}

float TransferFunctionPanel::valuePosition(double value,juce::Rectangle<float> area,bool phase)const{
 auto index=phase?1u:2u;
 double minimum=phase?-180.0:minDb,maximum=phase?180.0:maxDb;
 double normalized=(value-(minimum+maximum)*.5)/(maximum-minimum);
 return area.getCentreY()-static_cast<float>((normalized-panY[index])*zoomY[index])*area.getHeight();
}

void TransferFunctionPanel::drawGrid(juce::Graphics& g,juce::Rectangle<float> area,bool phase)const{
 g.setColour(grid);g.drawRect(area,1.0f);
 for(double hz:{20.0,50.0,100.0,200.0,500.0,1000.0,2000.0,5000.0,10000.0,20000.0}){
  auto x=frequencyPosition(hz,area,phase);if(!x||*x<area.getX()||*x>area.getRight())continue;
  g.setColour(grid);g.drawVerticalLine(juce::roundToInt(*x),area.getY(),area.getBottom());
  g.setColour(text);g.setFont(10.0f);
  auto label=hz>=1000.0?juce::String(hz/1000.0,0)+"k":juce::String(hz,0);
  g.drawText(label,juce::roundToInt(*x)-22,juce::roundToInt(area.getBottom())+3,44,15,juce::Justification::centred);
 }
 if(phase){
  for(int value=-180;value<=180;value+=90){auto y=valuePosition(value,area,true);if(y<area.getY()||y>area.getBottom())continue;g.setColour(grid);g.drawHorizontalLine(juce::roundToInt(y),area.getX(),area.getRight());g.setColour(text);g.drawText(juce::String(value),juce::roundToInt(area.getX())-41,juce::roundToInt(y)-8,36,16,juce::Justification::centredRight);}
 }else{
  for(int value=-60;value<=30;value+=15){auto y=valuePosition(value,area,false);if(y<area.getY()||y>area.getBottom())continue;g.setColour(grid);g.drawHorizontalLine(juce::roundToInt(y),area.getX(),area.getRight());g.setColour(text);g.drawText(juce::String(value),juce::roundToInt(area.getX())-41,juce::roundToInt(y)-8,36,16,juce::Justification::centredRight);}
 }
}

void TransferFunctionPanel::drawTraces(juce::Graphics& g,juce::Rectangle<float> area,bool phase)const{
 for(const auto& trace:traces){
  juce::Path path;bool started=false;double previousPhase=0.0;
  for(const auto& sample:trace.samples){
   auto x=frequencyPosition(sample.frequencyHz,area,phase);
   double value=phase?sample.phaseDegrees:sample.magnitudeDb;
   if(!sample.valid||!x||!std::isfinite(value)){started=false;continue;}
   auto y=valuePosition(value,area,phase);
   if(!started||(phase&&std::abs(value-previousPhase)>180.0))path.startNewSubPath(*x,y);
   else path.lineTo(*x,y);
   started=true;previousPhase=value;
  }
  g.setColour(trace.colour);g.strokePath(path,juce::PathStrokeType(trace.selected?2.2f:1.3f));
 }
}

void TransferFunctionPanel::drawCoherence(juce::Graphics& g,juce::Rectangle<float> area)const{
 for(const auto& trace:traces){
  juce::Path path;bool started=false;
  for(const auto& sample:trace.samples){
   auto x=frequencyPosition(sample.frequencyHz,area,false);
   if(!sample.valid||!x||!std::isfinite(sample.coherence)){started=false;continue;}
   float y=area.getBottom()-juce::jlimit(0.0f,1.0f,static_cast<float>(sample.coherence))*area.getHeight();
   if(!started)path.startNewSubPath(*x,y);else path.lineTo(*x,y);
   started=true;
  }
  g.setColour(trace.colour.withAlpha(0.65f));g.strokePath(path,juce::PathStrokeType(trace.selected?1.6f:1.0f));
 }
}

void TransferFunctionPanel::paint(juce::Graphics& g){
 g.fillAll(background);
}

void TransferFunctionPanel::paintPane(juce::Graphics& g,juce::Rectangle<float> bounds,bool phase)const{
 g.fillAll(background);
 if(bounds.getWidth()<130.0f||bounds.getHeight()<80.0f)return;
 auto area=juce::Rectangle<float>{bounds.getX()+51.0f,bounds.getY()+23.0f,bounds.getWidth()-63.0f,bounds.getHeight()-48.0f};
 if(area.isEmpty())return;
 g.setColour(text);g.setFont(12.0f);
 g.drawText(phase?"FASE / grados":"MAGNITUD / dB",12,2,180,18,juce::Justification::centredLeft);
 drawGrid(g,area,phase);
 g.saveState();g.reduceClipRegion(area.getSmallestIntegerContainer());drawTraces(g,area,phase);if(!phase)drawCoherence(g,area);g.restoreState();
 if(phase&&traces.empty()){g.setColour(text);g.drawText("FT sin medidas activas",area.getSmallestIntegerContainer(),juce::Justification::centred);}
}
