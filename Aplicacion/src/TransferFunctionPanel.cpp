#include "TransferFunctionPanel.h"
#include "PlotPresentation.h"
#include "CoherenceBlanking.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {
constexpr double minHz=20.0,maxHz=20000.0;
constexpr float minDb=-18.0f,maxDb=15.0f;
const juce::Colour background{0xff080a0c},grid{0xff2c3135},text{0xffa9bac8},axisBlue{0xff55b8f2};

double readableStep(double target){
 if(!std::isfinite(target)||target<=0)return 1.;
 double magnitude=std::pow(10.,std::floor(std::log10(target)));
 double best=magnitude,error=std::numeric_limits<double>::infinity();
 for(double factor:{1.,2.,5.,10.}){double candidate=factor*magnitude;double difference=std::abs(std::log(candidate/target));if(difference<error){best=candidate;error=difference;}}
 return best;
}

juce::String axisNumber(double value,double step){
 int decimals=step<1.?juce::jlimit(0,4,(int)std::ceil(-std::log10(step))):0;
 if(std::abs(value)<step*.0001)value=0;
 return juce::String(value,decimals);
}

juce::String frequencyNumber(double hz,double interval){
 if(hz>=1000.){int decimals=interval<10.?3:interval<100.?2:interval<1000.?1:0;auto label=juce::String(hz/1000.,decimals);if(decimals>0)label=label.trimCharactersAtEnd("0").trimCharactersAtEnd(".");return label+"k";}
 int decimals=interval<1.?2:interval<10.?1:0;
 auto label=juce::String(hz,decimals);return decimals>0?label.trimCharactersAtEnd("0").trimCharactersAtEnd("."):label;
}

}

class TransferFunctionPanel::PlotPane : public juce::Component {
public:
 PlotPane(TransferFunctionPanel& panel,bool isPhase):owner(panel),phase(isPhase){setInterceptsMouseClicks(true,false);}
 void paint(juce::Graphics& g) override{owner.paintPane(g,getLocalBounds().toFloat(),phase);}
 void mouseMove(const juce::MouseEvent& e) override{owner.updatePaneCursor(phase?1:2,e.position,false);}
 void mouseExit(const juce::MouseEvent&) override{owner.updatePaneCursor(phase?1:2,{},true);}
private:
 TransferFunctionPanel& owner;
 bool phase;
};

class TransferFunctionPanel::CoherenceHandle : public juce::Component {
public:
 explicit CoherenceHandle(TransferFunctionPanel& panel):owner(panel){setMouseCursor(juce::MouseCursor::UpDownResizeCursor);}
 void paint(juce::Graphics& g) override{
  g.setColour(juce::Colour(0xffb9c0c5));
  juce::Path tip;tip.startNewSubPath(1.f,(float)getHeight()*.5f);tip.lineTo((float)getWidth()-1.f,1.f);tip.lineTo((float)getWidth()-1.f,(float)getHeight()-1.f);tip.closeSubPath();g.fillPath(tip);
 }
 void mouseDown(const juce::MouseEvent& e) override{owner.dragCoherenceHandle(e.getEventRelativeTo(&owner).position.y);}
 void mouseDrag(const juce::MouseEvent& e) override{owner.dragCoherenceHandle(e.getEventRelativeTo(&owner).position.y);}
 void mouseUp(const juce::MouseEvent&) override{if(owner.onCoherenceThresholdCommitted)owner.onCoherenceThresholdCommitted();}
private:
 TransferFunctionPanel& owner;
};

TransferFunctionPanel::TransferFunctionPanel(DataProvider provider):dataProvider(std::move(provider)){
 phasePane=std::make_unique<PlotPane>(*this,true);magnitudePane=std::make_unique<PlotPane>(*this,false);
 addAndMakeVisible(*phasePane);addAndMakeVisible(*magnitudePane);
 coherenceHandle=std::make_unique<CoherenceHandle>(*this);addAndMakeVisible(*coherenceHandle);
}
void TransferFunctionPanel::setDisplayOptions(bool multi,int smooth){if(multiWindowMode!=multi){multiWindowMode=multi;if(onResolutionChanged)onResolutionChanged(multi);}smoothingMode=juce::jlimit(1,6,smooth);}
TransferFunctionPanel::~TransferFunctionPanel(){stopTimer();}

void TransferFunctionPanel::clearTraces(){traces.clear();history.clear();lastUpdateMs=0;phasePane->repaint();magnitudePane->repaint();}
void TransferFunctionPanel::refreshNow(){if(!frozen)timerCallback();}
void TransferFunctionPanel::setFrozen(bool shouldFreeze){frozen=shouldFreeze;if(frozen)stopTimer();else{suppressedTraces.clear();if(active&&isShowing()){timerCallback();startTimerHz(30);}}}
void TransferFunctionPanel::setTraceEnabled(int traceId,bool enabled){if(enabled)suppressedTraces.erase(traceId);else suppressedTraces.insert(traceId);phasePane->repaint();magnitudePane->repaint();}

void TransferFunctionPanel::setActive(bool shouldBeActive){
 if(frozen){active=shouldBeActive;stopTimer();return;}
 if(active==shouldBeActive){if(!active&&isShowing())timerCallback();return;}
 active=shouldBeActive;
 if(active&&isShowing()){timerCallback();startTimerHz(30);}
 else {stopTimer();timerCallback();}
}

void TransferFunctionPanel::visibilityChanged(){
 if(!isShowing())stopTimer();
 else if(frozen)stopTimer();
 else if(active){timerCallback();startTimerHz(30);}
 else timerCallback();
}

void TransferFunctionPanel::timerCallback(){
 if(frozen)return;
 auto incoming=dataProvider?dataProvider():std::vector<Trace>{};
 double now=juce::Time::getMillisecondCounterHiRes(),dt=lastUpdateMs>0?juce::jlimit(0.0,1.0,(now-lastUpdateMs)/1000.):0.;lastUpdateMs=now;
 double tau=1.;
 double alpha=dt>0?1.-std::exp(-dt/tau):1.;
 std::map<int,Trace> next;
 for(auto& trace:incoming){auto old=history.find(trace.id);if(!trace.frozen&&old!=history.end()&&old->second.samples.size()==trace.samples.size())for(size_t i=0;i<trace.samples.size();++i){auto& current=trace.samples[i];const auto& previous=old->second.samples[i];if(!current.valid||!previous.valid)continue;current.magnitudeDb=previous.magnitudeDb+alpha*(current.magnitudeDb-previous.magnitudeDb);current.phaseDegrees=previous.phaseDegrees+alpha*std::remainder(current.phaseDegrees-previous.phaseDegrees,360.);current.phaseDegrees=std::remainder(current.phaseDegrees,360.);current.coherence=previous.coherence+alpha*(current.coherence-previous.coherence);}next[trace.id]=trace;}
 history=std::move(next);traces=std::move(incoming);
 phasePane->repaint();magnitudePane->repaint();
}

void TransferFunctionPanel::setLineThickness(float pixels){lineThickness=juce::jlimit(1.f,4.f,pixels);phasePane->repaint();magnitudePane->repaint();}

std::optional<TransferFunctionPanel::Sample> TransferFunctionPanel::sampleAt(int traceId,double hz,bool phase) const{
 if(!std::isfinite(hz))return {};
 for(const auto& trace:traces){
  if(trace.id!=traceId||suppressedTraces.count(traceId)||(phase?!trace.showPhase:!trace.showMagnitude))continue;
  const auto& samples=trace.samples;
  auto valid=[this,phase](const Sample& s){return s.valid&&(phase?s.validPhase:s.validMagnitude)&&std::isfinite(phase?s.phaseDegrees:s.magnitudeDb)&&(s.bypassBlanking||(std::isfinite(s.coherence)&&CoherenceBlanking::passes(s.coherence,coherenceThresholdPercent,coherenceSquared)));};
  auto upper=std::lower_bound(samples.begin(),samples.end(),hz,[](const Sample& s,double value){return s.frequencyHz<value;});
  if(upper!=samples.end()&&upper->frequencyHz==hz)return valid(*upper)?std::optional<Sample>(*upper):std::nullopt;
  if(upper==samples.begin()||upper==samples.end())return {};
  const auto& lo=*(upper-1);const auto& hi=*upper;
  if(!valid(lo)||!valid(hi)||hi.frequencyHz<=lo.frequencyHz)return {};
  double t=(hz-lo.frequencyHz)/(hi.frequencyHz-lo.frequencyHz);
  Sample result=lo;result.frequencyHz=hz;result.magnitudeDb=lo.magnitudeDb+t*(hi.magnitudeDb-lo.magnitudeDb);result.coherence=lo.coherence+t*(hi.coherence-lo.coherence);result.phaseDegrees=std::remainder(lo.phaseDegrees+t*std::remainder(hi.phaseDegrees-lo.phaseDegrees,360.),360.);return result;
 }
 return {};
}

void TransferFunctionPanel::resized(){}

void TransferFunctionPanel::setPlotLayout(juce::Rectangle<int> phase,juce::Rectangle<int> magnitude){
 phasePane->setBounds(phase);magnitudePane->setBounds(magnitude);updateCoherenceHandle();
}

void TransferFunctionPanel::setCoherenceDisplay(double thresholdPercent,bool squared,bool visible,double height){
 coherenceThresholdPercent=juce::jlimit(0.,100.,thresholdPercent);coherenceSquared=squared;coherenceVisible=visible;coherenceHeight=juce::jlimit(0.05,1.,height);updateCoherenceHandle();phasePane->repaint();magnitudePane->repaint();
}

float TransferFunctionPanel::coherenceLineY(juce::Rectangle<float> area) const{return area.getBottom()-static_cast<float>(coherenceThresholdPercent/100.)*area.getHeight();}

void TransferFunctionPanel::updateCoherenceHandle(){
 if(!coherenceHandle||!magnitudePane)return;
 auto bounds=magnitudePane->getBounds();auto area=juce::Rectangle<float>{51.f,23.f,(float)bounds.getWidth()-63.f,(float)bounds.getHeight()-48.f};
 coherenceHandle->setVisible(bounds.getWidth()>=130&&bounds.getHeight()>=80);
 coherenceHandle->setBounds(bounds.getX()+juce::roundToInt(area.getRight())+1,bounds.getY()+juce::roundToInt(coherenceLineY(area))-5,9,10);
 coherenceHandle->toFront(false);
}

void TransferFunctionPanel::dragCoherenceHandle(float panelY){
 auto bounds=magnitudePane->getBounds();auto top=(float)bounds.getY()+23.f;auto height=(float)bounds.getHeight()-48.f;if(height<=0)return;
 double percent=juce::jlimit(0.,100.,100.*((top+height-panelY)/height));
 if(std::abs(percent-coherenceThresholdPercent)<.01)return;
 coherenceThresholdPercent=percent;updateCoherenceHandle();phasePane->repaint();magnitudePane->repaint();if(onCoherenceThresholdChanged)onCoherenceThresholdChanged(percent);
}

void TransferFunctionPanel::setPlotTransforms(const std::array<double,3>& newPanX,const std::array<double,3>& newPanY,
                                             const std::array<double,3>& newZoomX,const std::array<double,3>& newZoomY){
 panX=newPanX;panY=newPanY;zoomX=newZoomX;zoomY=newZoomY;
 phasePane->repaint();magnitudePane->repaint();
}

void TransferFunctionPanel::setSelectedPlot(int plot){
 if(plot!=1&&plot!=2)return;
 if(selectedPlot==plot)return;
 selectedPlot=plot;phasePane->repaint();magnitudePane->repaint();
}

void TransferFunctionPanel::setPlotCursor(int plot,std::optional<juce::Point<float>> position){
 if(plot!=1&&plot!=2)return;
 plotCursors[(size_t)plot]=position;
 (plot==1?phasePane:magnitudePane)->repaint();
}

void TransferFunctionPanel::updatePaneCursor(int plot,juce::Point<float> position,bool leaving){
 auto& pane=plot==1?*phasePane:*magnitudePane;
 auto area=pane.getLocalBounds().toFloat().reduced(0.f);
 area={area.getX()+44.f,area.getY()+23.f,area.getWidth()-52.f,area.getHeight()-48.f};
 if(leaving||area.isEmpty()||!area.contains(position)){setPlotCursor(plot,{});if(onCursorMoved)onCursorMoved(plot,0.,false);return;}
 const auto index=(size_t)plot;
 const double screen=(position.x-area.getX())/area.getWidth();
 const double u=PlotPresentation::coordinate(screen,panX[index],zoomX[index]);
 const double hz=minHz*std::pow(maxHz/minHz,u);
 const Trace* focus=nullptr;
 for(const auto& trace:traces)if(!suppressedTraces.count(trace.id)&&(plot==1?trace.showPhase:trace.showMagnitude)){
  if(!focus||trace.selected)focus=&trace;
  if(trace.selected)break;
 }
 const auto sample=focus?sampleAt(focus->id,hz,plot==1):std::optional<Sample>{};
 if(sample){const float y=valuePosition(plot==1?sample->phaseDegrees:sample->magnitudeDb,area,plot==1);setPlotCursor(plot,juce::Point<float>{(float)screen,(y-area.getY())/area.getHeight()});}
 else setPlotCursor(plot,{});
 if(onCursorMoved)onCursorMoved(plot,u,sample.has_value());
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
 auto index=phase?1u:2u;
 double visibleOctaves=std::log2(maxHz/minHz)/zoomX[index];
 int divisions=1;
 for(int candidate:{1,2,3,6,12,24,48,96,192})if(area.getWidth()/(visibleOctaves*candidate)>=72.)divisions=candidate;
 double firstU=PlotPresentation::coordinate(0.,panX[index],zoomX[index]);
 double lastU=PlotPresentation::coordinate(1.,panX[index],zoomX[index]);
 double firstOctave=std::log2(minHz/1000.)+firstU*std::log2(maxHz/minHz);
 double lastOctave=std::log2(minHz/1000.)+lastU*std::log2(maxHz/minHz);
 int first=(int)std::ceil(firstOctave*divisions-1.e-9),last=(int)std::floor(lastOctave*divisions+1.e-9);
 for(int n=first;n<=last&&n<first+512;++n){
  double hz=1000.*std::exp2((double)n/divisions);
  if(hz<minHz||hz>maxHz)continue;
  auto x=frequencyPosition(hz,area,phase);if(!x||*x<area.getX()||*x>area.getRight())continue;
  g.setColour(grid);g.drawVerticalLine(juce::roundToInt(*x),area.getY(),area.getBottom());
  double interval=hz*(std::exp2(1./(double)divisions)-1.);
  g.setColour(axisBlue);g.setFont(10.0f);
  g.drawText(frequencyNumber(hz,interval),juce::roundToInt(*x)-27,juce::roundToInt(area.getBottom())+3,54,15,juce::Justification::centred);
 }
 double minimum=phase?-180.:minDb,maximum=phase?180.:maxDb,span=maximum-minimum,centre=(minimum+maximum)*.5;
 double lower=centre+span*(panY[index]-.5/zoomY[index]),upper=centre+span*(panY[index]+.5/zoomY[index]);
 double step=readableStep((upper-lower)*50./area.getHeight());
 int start=(int)std::ceil(lower/step-1.e-9),end=(int)std::floor(upper/step+1.e-9);
 for(int tick=start;tick<=end&&tick<start+512;++tick){
  double value=tick*step;float y=valuePosition(value,area,phase);
  if(y<area.getY()||y>area.getBottom())continue;
  g.setColour(grid);g.drawHorizontalLine(juce::roundToInt(y),area.getX(),area.getRight());
  g.setColour(axisBlue);g.setFont(10.0f);
  g.drawText(axisNumber(value,step)+(phase?juce::String::fromUTF8("°"):juce::String{}),
             juce::roundToInt(area.getX())-48,juce::roundToInt(y)-8,43,16,juce::Justification::centredRight);
 }
}

void TransferFunctionPanel::drawTraces(juce::Graphics& g,juce::Rectangle<float> area,bool phase)const{
 for(int layer=0;layer<2;++layer)for(const auto& trace:traces){
  if(suppressedTraces.count(trace.id))continue;
  if(trace.selected!=(layer==1))continue;
  if(phase?!trace.showPhase:!trace.showMagnitude)continue;
  juce::Path path;bool started=false;double previousPhase=0.0;
  for(const auto& sample:trace.samples){
   auto x=frequencyPosition(sample.frequencyHz,area,phase);
   double value=phase?sample.phaseDegrees:sample.magnitudeDb;
   if(!sample.valid||(phase?!sample.validPhase:!sample.validMagnitude)||!x||!std::isfinite(value)||(!sample.bypassBlanking&&!CoherenceBlanking::passes(sample.coherence,coherenceThresholdPercent,coherenceSquared))){started=false;continue;}
   auto y=valuePosition(value,area,phase);
   if(!started||(phase&&std::abs(value-previousPhase)>180.0))path.startNewSubPath(*x,y);
   else path.lineTo(*x,y);
   started=true;previousPhase=value;
  }
  g.setColour(trace.colour);g.strokePath(path,juce::PathStrokeType(lineThickness));
 }
}

void TransferFunctionPanel::drawCoherence(juce::Graphics& g,juce::Rectangle<float> area)const{
 if(!coherenceVisible)return;
 for(int layer=0;layer<2;++layer)for(const auto& trace:traces){
  if(suppressedTraces.count(trace.id))continue;
  if(trace.selected!=(layer==1))continue;
  if(!trace.showMagnitude)continue;
  juce::Path path;bool started=false;
  for(const auto& sample:trace.samples){
   auto x=frequencyPosition(sample.frequencyHz,area,false);
   if(!sample.valid||!x||!std::isfinite(sample.coherence)){started=false;continue;}
   float y=area.getY()+static_cast<float>(1.-CoherenceBlanking::displayed(sample.coherence,coherenceSquared))*area.getHeight()*static_cast<float>(coherenceHeight);
   if(!started)path.startNewSubPath(*x,y);else path.lineTo(*x,y);
   started=true;
  }
  g.setColour(juce::Colour(0xffff3030));g.strokePath(path,juce::PathStrokeType(lineThickness));
 }
}

void TransferFunctionPanel::paint(juce::Graphics& g){
 g.fillAll(background);
}

void TransferFunctionPanel::paintPane(juce::Graphics& g,juce::Rectangle<float> bounds,bool phase)const{
 g.fillAll(background);
 if(bounds.getWidth()<130.0f||bounds.getHeight()<80.0f)return;
 auto area=juce::Rectangle<float>{bounds.getX()+44.0f,bounds.getY()+23.0f,bounds.getWidth()-52.0f,bounds.getHeight()-48.0f};
 if(area.isEmpty())return;
 g.setColour(text);g.setFont(12.0f);
 g.drawText(phase?"FASE / grados":"MAGNITUD / dB",12,2,180,18,juce::Justification::centredLeft);
 drawGrid(g,area,phase);
 g.saveState();g.reduceClipRegion(area.getSmallestIntegerContainer());drawTraces(g,area,phase);if(!phase)drawCoherence(g,area);g.restoreState();
 if(!phase){g.setColour(juce::Colour(0xff9ca0a4).withAlpha(.55f));g.drawHorizontalLine(juce::roundToInt(coherenceLineY(area)),area.getX(),area.getRight());}
 if(phase&&traces.empty()){g.setColour(text);g.drawText("FT sin medidas activas",area.getSmallestIntegerContainer(),juce::Justification::centred);}
 if(selectedPlot==(phase?1:2)){
  g.setColour(juce::Colour(0xff43cce8).withAlpha(.8f));
  g.drawRect(bounds.reduced(1.0f),1.25f);
 }
 if(auto cursor=plotCursors[(size_t)(phase?1:2)]){
  const float x=area.getX()+cursor->x*area.getWidth(),y=area.getY()+cursor->y*area.getHeight();
  if(area.contains(x,y)){
   g.setColour(juce::Colour(0xff00e5ed).withAlpha(.9f));
   g.drawVerticalLine(juce::roundToInt(x),area.getY(),area.getBottom());
   g.drawHorizontalLine(juce::roundToInt(y),area.getX(),area.getRight());
  }
 }
}
