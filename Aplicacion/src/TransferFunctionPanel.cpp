#include "TransferFunctionPanel.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr double minHz=20.0,maxHz=20000.0;
constexpr float minDb=-60.0f,maxDb=30.0f;
const juce::Colour background{0xff080a0c},grid{0xff2c3135},text{0xffa9bac8};

float magnitudeY(double db,juce::Rectangle<float> area){
 return area.getBottom()-juce::jlimit(0.0f,1.0f,static_cast<float>((db-minDb)/(maxDb-minDb)))*area.getHeight();
}
float phaseY(double degrees,juce::Rectangle<float> area){
 return area.getBottom()-juce::jlimit(0.0f,1.0f,static_cast<float>((degrees+180.0)/360.0))*area.getHeight();
}
}

TransferFunctionPanel::TransferFunctionPanel(DataProvider provider):dataProvider(std::move(provider)){}
TransferFunctionPanel::~TransferFunctionPanel(){stopTimer();}

void TransferFunctionPanel::setActive(bool shouldBeActive){
 active=shouldBeActive;
 if(active&&isShowing()){timerCallback();startTimerHz(30);}
 else stopTimer();
}

void TransferFunctionPanel::visibilityChanged(){
 if(!isShowing())stopTimer();
 else if(active){timerCallback();startTimerHz(30);}
}

void TransferFunctionPanel::timerCallback(){
 traces=dataProvider?dataProvider():std::vector<Trace>{};
 repaint();
}

std::optional<float> TransferFunctionPanel::frequencyPosition(double hz,juce::Rectangle<float> area){
 if(!std::isfinite(hz)||hz<minHz||hz>maxHz||area.isEmpty())return {};
 return area.getX()+static_cast<float>(std::log(hz/minHz)/std::log(maxHz/minHz))*area.getWidth();
}

void TransferFunctionPanel::drawGrid(juce::Graphics& g,juce::Rectangle<float> area,bool phase)const{
 g.setColour(grid);g.drawRect(area,1.0f);
 for(double hz:{20.0,50.0,100.0,200.0,500.0,1000.0,2000.0,5000.0,10000.0,20000.0}){
  auto x=frequencyPosition(hz,area);if(!x)continue;
  g.setColour(grid);g.drawVerticalLine(juce::roundToInt(*x),area.getY(),area.getBottom());
  g.setColour(text);g.setFont(10.0f);
  auto label=hz>=1000.0?juce::String(hz/1000.0,0)+"k":juce::String(hz,0);
  g.drawText(label,juce::roundToInt(*x)-22,juce::roundToInt(area.getBottom())+3,44,15,juce::Justification::centred);
 }
 if(phase){
  for(int value=-180;value<=180;value+=90){auto y=phaseY(value,area);g.setColour(grid);g.drawHorizontalLine(juce::roundToInt(y),area.getX(),area.getRight());g.setColour(text);g.drawText(juce::String(value),juce::roundToInt(area.getX())-41,juce::roundToInt(y)-8,36,16,juce::Justification::centredRight);}
 }else{
  for(int value=-60;value<=30;value+=15){auto y=magnitudeY(value,area);g.setColour(grid);g.drawHorizontalLine(juce::roundToInt(y),area.getX(),area.getRight());g.setColour(text);g.drawText(juce::String(value),juce::roundToInt(area.getX())-41,juce::roundToInt(y)-8,36,16,juce::Justification::centredRight);}
 }
}

void TransferFunctionPanel::drawTraces(juce::Graphics& g,juce::Rectangle<float> area,bool phase)const{
 for(const auto& trace:traces){
  juce::Path path;bool started=false;double previousPhase=0.0;
  for(const auto& sample:trace.samples){
   auto x=frequencyPosition(sample.frequencyHz,area);
   double value=phase?sample.phaseDegrees:sample.magnitudeDb;
   if(!sample.valid||!x||!std::isfinite(value)){started=false;continue;}
   auto y=phase?phaseY(value,area):magnitudeY(value,area);
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
   auto x=frequencyPosition(sample.frequencyHz,area);
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
 auto bounds=getLocalBounds().toFloat();
 if(bounds.getWidth()<130.0f||bounds.getHeight()<160.0f)return;
 float paneHeight=(bounds.getHeight()-36.0f)/2.0f;
 auto phaseArea=juce::Rectangle<float>{bounds.getX()+51.0f,bounds.getY()+23.0f,bounds.getWidth()-63.0f,paneHeight-35.0f};
 auto magnitudeArea=juce::Rectangle<float>{bounds.getX()+51.0f,bounds.getY()+paneHeight+23.0f,bounds.getWidth()-63.0f,paneHeight-35.0f};
 if(phaseArea.isEmpty()||magnitudeArea.isEmpty())return;
 g.setColour(text);g.setFont(12.0f);
 g.drawText("FASE / grados",12,2,180,18,juce::Justification::centredLeft);
 g.drawText("MAGNITUD / dB",12,juce::roundToInt(paneHeight)+2,180,18,juce::Justification::centredLeft);
 drawGrid(g,phaseArea,true);drawGrid(g,magnitudeArea,false);
 g.saveState();g.reduceClipRegion(phaseArea.getSmallestIntegerContainer());drawTraces(g,phaseArea,true);g.restoreState();
 g.saveState();g.reduceClipRegion(magnitudeArea.getSmallestIntegerContainer());drawTraces(g,magnitudeArea,false);drawCoherence(g,magnitudeArea);g.restoreState();
 if(traces.empty()){g.setColour(text);g.drawText("FT sin medidas activas",phaseArea.getSmallestIntegerContainer(),juce::Justification::centred);}
}
