#include "RTAPanel.h"
#include "PlotPresentation.h"
#include "MultiWindowSpectrum.h"
#include "GraphCursor.h"
#include <cmath>
#include <limits>

namespace {
constexpr size_t spectrogramRows=256;
juce::Colour heat(float db,double lower,double upper){
 if(!std::isfinite(db))return juce::Colours::black;
 float t=(float)juce::jlimit(0.,1.,(db-lower)/(upper-lower));
 if(t<.25f)return juce::Colours::black.interpolatedWith(juce::Colour(0xff123f9c),t*4.f);
 if(t<.5f)return juce::Colour(0xff123f9c).interpolatedWith(juce::Colours::cyan,(t-.25f)*4.f);
 if(t<.75f)return juce::Colours::cyan.interpolatedWith(juce::Colours::yellow,(t-.5f)*4.f);
 return juce::Colours::yellow.interpolatedWith(juce::Colours::red,(t-.75f)*4.f);
}
}

RTAPanel::RTAPanel(DataProvider provider):dataProvider(std::move(provider)){
 speed.addItemList({"Fast","Slow","Prom.16"},1);speed.setSelectedId(1);speed.setTooltip("Respuesta temporal de la curva RTA");speed.onChange=[this]{history.clear();lastUpdateMs=0;};addAndMakeVisible(speed);
 windowCount.addItem("1 Ventana",1);windowCount.addItem("2 Ventanas",2);windowCount.setSelectedId(1);windowCount.onChange=[this]{resized();repaint();};addAndMakeVisible(windowCount);
 for(auto* slider:{&upperLimit,&lowerLimit}){slider->setSliderStyle(juce::Slider::LinearVertical);slider->setTextBoxStyle(juce::Slider::TextBoxBelow,false,48,18);addAndMakeVisible(*slider);}upperLimit.setRange(-119.,0.,1.);lowerLimit.setRange(-120.,-1.,1.);
 upperLimit.setValue(-10.);lowerLimit.setValue(-90.);upperLimit.setTitle(juce::String::fromUTF8("Límite superior del espectrograma en dBFS"));lowerLimit.setTitle(juce::String::fromUTF8("Límite inferior del espectrograma en dBFS"));upperLimit.onValueChange=[this]{if(upperLimit.getValue()<=lowerLimit.getValue())lowerLimit.setValue(upperLimit.getValue()-1.,juce::dontSendNotification);repaint();};lowerLimit.onValueChange=[this]{if(lowerLimit.getValue()>=upperLimit.getValue())upperLimit.setValue(lowerLimit.getValue()+1.,juce::dontSendNotification);repaint();};
 peakTracker.setTooltip("Seguir el pico de nivel entre 20 Hz y Nyquist de la traza seleccionada");peakTracker.onClick=[this]{if(cursorVisible&&peakTracker.getToggleState())if(auto* trace=selectedTrace()){int index=peakIndex(*trace);if(index>=0)updateCursor(*trace,plotArea(),index);}repaint();};addAndMakeVisible(peakTracker);resized();
}

void RTAPanel::setDisplayOptions(bool multi,int smooth){if(multiWindowMode!=multi){multiWindowMode=multi;if(onResolutionChanged)onResolutionChanged(multi);}smoothingMode=juce::jlimit(1,6,smooth);}
void RTAPanel::setWindowCount(int count){if(count==1||count==2)windowCount.setSelectedId(count,juce::sendNotificationSync);}
void RTAPanel::updateComponentLanguage(bool spanish){spanishLanguage=spanish;windowCount.changeItemText(1,spanish?"1 Ventana":"1 Window");windowCount.changeItemText(2,spanish?"2 Ventanas":"2 Windows");speed.changeItemText(1,spanish?juce::String::fromUTF8("Rápido"):"Fast");speed.changeItemText(2,spanish?"Lento":"Slow");peakTracker.setButtonText(spanish?"Seguir pico":"Peak Tracker");repaint();}
void RTAPanel::setLineThickness(float pixels){lineThickness=juce::jlimit(1.f,4.f,pixels);repaint();}
void RTAPanel::setFrozen(bool shouldFreeze){frozen=shouldFreeze;if(frozen)stopTimer();}
void RTAPanel::clearTraces(){traces.clear();history.clear();spectrogramHistory.clear();spectrogramHead=0;spectrogramTraceId=-1;lastUpdateMs=0;cursorVisible=false;cursorReading.clear();publishCursor();repaint();}
void RTAPanel::setActive(bool active){if(frozen){stopTimer();return;}if(active&&!isTimerRunning()){timerCallback();startTimerHz(30);}else if(!active){stopTimer();clearTraces();}}
void RTAPanel::visibilityChanged(){if(!isShowing())stopTimer();}
juce::Rectangle<int> RTAPanel::plotArea()const{auto bounds=getLocalBounds().reduced(55,42);if(windowCount.getSelectedId()==2)bounds.setHeight(juce::jmax(1,(bounds.getHeight()-8)/2));return bounds;}
juce::Rectangle<int> RTAPanel::spectrogramArea()const{if(windowCount.getSelectedId()!=2)return {};auto r=plotArea();return {r.getX(),r.getBottom()+8,r.getWidth(),r.getHeight()};}
const RTAPanel::Trace* RTAPanel::selectedTrace()const{for(const auto& trace:traces)if(trace.selected)return &trace;return traces.empty()?nullptr:&traces.front();}
int RTAPanel::peakIndex(const Trace& trace)const{float maximum=-std::numeric_limits<float>::infinity();int best=-1;for(size_t i=0;i<trace.dbfs.size();++i){double hz=MultiWindowSpectrum::gridFrequency((int)i);if(hz<20.||hz>trace.nyquistHz)continue;if(std::isfinite(trace.dbfs[i])&&trace.dbfs[i]>maximum){maximum=trace.dbfs[i];best=(int)i;}}return best;}
void RTAPanel::publishCursor(){if(onCursorReadout)onCursorReadout(cursorVisible?cursorReading:juce::String{});}
void RTAPanel::updateCursor(const Trace& trace,juce::Rectangle<int> area,double index){
 if(trace.dbfs.size()<2||area.isEmpty()||!std::isfinite(index)||index<0||index>(double)(trace.dbfs.size()-1))return;
 const auto lo=(size_t)std::floor(index),hi=juce::jmin(lo+1,trace.dbfs.size()-1);
 const double a=trace.dbfs[lo],b=trace.dbfs[hi],fraction=index-(double)lo;
 if(!std::isfinite(a)||(fraction>0.&&!std::isfinite(b))){cursorVisible=false;cursorReading.clear();publishCursor();return;}
 auto db=fraction==0.?std::optional<double>(a):GraphCursor::interpolate(a,b,fraction);
 auto hz=GraphCursor::interpolateLogFrequency(MultiWindowSpectrum::gridFrequency((int)lo),MultiWindowSpectrum::gridFrequency((int)hi),fraction);
 if(!db||!hz||*hz<20.||*hz>trace.nyquistHz){cursorVisible=false;cursorReading.clear();publishCursor();return;}
 cursorX=(float)area.getX()+(float)(index/(trace.dbfs.size()-1))*area.getWidth();
 cursorY=(float)area.getBottom()-juce::jlimit(0.f,120.f,(float)*db+120.f)/120.f*area.getHeight();
 cursorReading=juce::String(*hz,1)+" Hz / "+juce::String(*db,1)+" dBFS";publishCursor();
}
void RTAPanel::timerCallback(){
 if(frozen)return;
 auto incoming=dataProvider?dataProvider():std::vector<Trace>{};if(incoming.empty()){traces.clear();history.clear();spectrogramHistory.clear();spectrogramHead=0;spectrogramTraceId=-1;cursorReading.clear();publishCursor();repaint();return;}
 double now=juce::Time::getMillisecondCounterHiRes(),dt=lastUpdateMs>0?juce::jlimit(0.0,1.0,(now-lastUpdateMs)/1000.):0.;lastUpdateMs=now;double tau=speed.getSelectedId()==2?1.:speed.getSelectedId()==3?5.4:.125;double alpha=dt>0?1.-std::exp(-dt/tau):1.;std::map<int,std::vector<float>> next;
 for(auto& trace:incoming){auto& values=next[trace.id];values=trace.dbfs;auto old=history.find(trace.id);if(old!=history.end()&&old->second.size()==values.size())for(size_t i=0;i<values.size();++i)if(std::isfinite(values[i])&&std::isfinite(old->second[i]))values[i]=(float)(old->second[i]+alpha*(values[i]-old->second[i]));trace.dbfs=values;}
 history=std::move(next);traces=std::move(incoming);
 if(const auto* chosen=selectedTrace()){if(spectrogramTraceId!=chosen->id){spectrogramHistory.clear();spectrogramHead=0;spectrogramTraceId=chosen->id;}if(!chosen->dbfs.empty()){if(spectrogramHistory.size()<spectrogramRows)spectrogramHistory.push_back(chosen->dbfs);else{spectrogramHistory[spectrogramHead]=chosen->dbfs;spectrogramHead=(spectrogramHead+1)%spectrogramRows;}}if(cursorVisible&&peakTracker.getToggleState()){int index=peakIndex(*chosen);if(index>=0)updateCursor(*chosen,plotArea(),index);}}
 repaint();
}
void RTAPanel::resized(){speed.setBounds(juce::jmax(8,getWidth()-120),8,108,23);peakTracker.setBounds(juce::jmax(8,getWidth()-255),8,130,23);windowCount.setBounds(juce::jmax(8,getWidth()-392),8,128,23);bool two=windowCount.getSelectedId()==2;upperLimit.setVisible(two);lowerLimit.setVisible(two);if(two){auto area=spectrogramArea();upperLimit.setBounds(area.getRight()+5,area.getY(),47,juce::jmax(24,area.getHeight()/2-3));lowerLimit.setBounds(area.getRight()+5,area.getCentreY()+3,47,juce::jmax(24,area.getHeight()/2-3));}}
void RTAPanel::mouseMove(const juce::MouseEvent& event){auto p=plotArea();if(!p.contains(event.getPosition())){mouseExit(event);return;}const auto* chosen=selectedTrace();cursorVisible=false;cursorReading.clear();if(chosen&&chosen->dbfs.size()>1){double index=juce::jlimit(0.0,(double)chosen->dbfs.size()-1.0,(double)(event.position.x-p.getX())*(double)(chosen->dbfs.size()-1)/(double)p.getWidth());if(peakTracker.getToggleState())index=peakIndex(*chosen);if(index>=0){cursorVisible=true;updateCursor(*chosen,p,index);}}else publishCursor();repaint();}
void RTAPanel::mouseExit(const juce::MouseEvent&){cursorVisible=false;cursorReading.clear();publishCursor();repaint();}
void RTAPanel::paint(juce::Graphics& g){
 using namespace juce;const Colour plot(0xff080a0c),line(0xff626e78),muted(0xffa9bac8),green(0xff54ee22),orange(0xffffac24);auto bounds=getLocalBounds();g.fillAll(plot);auto p=plotArea();if(p.isEmpty())return;g.setColour(line);g.drawRect(p);
 for(int db=-120;db<=0;db+=20){float y=(float)(p.getBottom()-(db+120.)/120.*p.getHeight());g.setColour(line.withAlpha(.55f));g.drawHorizontalLine((int)y,(float)p.getX(),(float)p.getRight());g.setColour(muted);g.setFont(10.f);g.drawText(String(db),5,(int)y-9,43,18,Justification::centredLeft);}
 for(double hz:PlotPresentation::frequencyTicks){float x=(float)(p.getX()+std::log(hz/20.)/std::log(1000.)*p.getWidth());g.setColour(line.withAlpha(.45f));g.drawVerticalLine((int)x,(float)p.getY(),(float)p.getBottom());g.setColour(muted);g.setFont(10.f);g.drawText(hz>=1000?String(hz/1000.,hz==1000?0:1)+"k":String(hz,0),(int)x-24,p.getBottom()+5,48,18,Justification::centred);}
 for(const auto& trace:traces){if(trace.dbfs.size()<2)continue;Path path;bool started=false;for(size_t i=0;i<trace.dbfs.size();++i){if(!std::isfinite(trace.dbfs[i])){started=false;continue;}float x=(float)p.getX()+(float)i/(float)(trace.dbfs.size()-1)*(float)p.getWidth();float y=(float)(p.getBottom()-jlimit(0.f,120.f,trace.dbfs[i]+120.f)/120.f*p.getHeight());if(!started){path.startNewSubPath(x,y);started=true;}else path.lineTo(x,y);}g.setColour(trace.colour);g.strokePath(path,PathStrokeType(lineThickness));}
 if(cursorVisible){g.setColour(Colour(0xff00cdda).withAlpha(.7f));g.drawVerticalLine(roundToInt(cursorX),(float)p.getY(),(float)p.getBottom());g.drawHorizontalLine(roundToInt(cursorY),(float)p.getX(),(float)p.getRight());}
 auto s=spectrogramArea();if(!s.isEmpty()){g.setColour(line);g.drawRect(s);const auto count=spectrogramHistory.size();if(count){Image image(Image::RGB,s.getWidth(),s.getHeight(),true);for(int y=0;y<s.getHeight();++y){size_t age=(size_t)((double)y/s.getHeight()*count);size_t row=(spectrogramHead+spectrogramRows-1-age)%spectrogramRows;if(count<spectrogramRows)row=count-1-juce::jmin(age,count-1);const auto& spectrum=spectrogramHistory[row];for(int x=0;x<s.getWidth();++x){size_t bin=(size_t)((double)x/juce::jmax(1,s.getWidth()-1)*juce::jmax<size_t>(1,spectrum.size()-1));if(bin<spectrum.size())image.setPixelAt(x,y,heat(spectrum[bin],lowerLimit.getValue(),upperLimit.getValue()));}}g.drawImageAt(image,s.getX(),s.getY());}}
 g.setColour(traces.empty()?orange:green);g.setFont(13.f);g.drawText(cursorReading.isNotEmpty()?cursorReading:traces.empty()?(spanishLanguage?juce::String::fromUTF8("RTA sin señal: pulsa Play en una TF configurada"):"RTA without signal: press Play on a configured TF"):(spanishLanguage?"RTA en directo":"Live RTA"),10,7,jmax(0,bounds.getWidth()-400),23,Justification::centredLeft);
}
