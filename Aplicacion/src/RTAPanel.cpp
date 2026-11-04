#include "RTAPanel.h"
#include "PlotPresentation.h"
#include "MultiWindowSpectrum.h"
#include "GraphCursor.h"
#include "MyaBrandAssets.h"
#include <cmath>
#include <limits>

namespace {
constexpr size_t spectrogramRows=256;
int bandDivisions(int mode){return mode==7?1:mode==6?3:mode==5?6:mode==4?12:mode==3?24:mode==2?48:0;}
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
 windowCount.addItem("1 Ventana",1);windowCount.addItem("2 Ventanas",2);windowCount.setSelectedId(1);windowCount.onChange=[this]{resized();repaint();};addAndMakeVisible(windowCount);
 bandedStyle.addItem("Barras",1);bandedStyle.addItem(juce::String::fromUTF8("Líneas"),2);bandedStyle.addItem("Ambas",3);bandedStyle.addItem("3D",4);bandedStyle.setSelectedId(2);bandedStyle.onChange=[this]{repaint();};addAndMakeVisible(bandedStyle);
 smoothingChoiceBox.addItem("Ninguno",1);smoothingChoiceBox.addItem("1/48 Oct",2);smoothingChoiceBox.addItem("1/24 Oct",3);smoothingChoiceBox.addItem("1/12 Oct",4);smoothingChoiceBox.addItem("1/6 Oct",5);smoothingChoiceBox.addItem("1/3 Oct",6);smoothingChoiceBox.addItem("Octave",7);smoothingChoiceBox.setSelectedId(2);smoothingChoiceBox.onChange=[this]{int id=smoothingChoiceBox.getSelectedId();if(id>=1&&id<=7){smoothingMode=id;if(onSmoothingChanged)onSmoothingChanged(smoothingMode);repaint();}};addAndMakeVisible(smoothingChoiceBox);
 microphoneChoice.setTextWhenNothingSelected("Micro");microphoneChoice.onChange=[this]{spectrogramTraceId=-1;spectrogramHistory.clear();spectrogramImage={};spectrogramDirty=true;++spectrogramGeneration;cursorVisible=false;cursorReading.clear();publishCursor();repaint();};addAndMakeVisible(microphoneChoice);
 peakTracker.setTooltip("Seguir el pico de nivel entre 20 Hz y Nyquist de la traza seleccionada");peakTracker.onClick=[this]{if(cursorVisible&&peakTracker.getToggleState())if(auto* trace=selectedTrace()){int index=peakIndex(*trace);if(index>=0)updateCursor(*trace,plotArea(),index);}repaint();};addAndMakeVisible(peakTracker);vblank=juce::VBlankAttachment(this,[this](double timestamp){presentFrame(timestamp);});resized();
}
RTAPanel::~RTAPanel(){stopTimer();vblank={};if(pendingSpectrogram.valid())pendingSpectrogram.wait();}

void RTAPanel::setDisplayOptions(bool multi,int smooth){if(multiWindowMode!=multi){multiWindowMode=multi;if(onResolutionChanged)onResolutionChanged(multi);}smoothingMode=juce::jlimit(1,7,smooth);smoothingChoiceBox.setSelectedId(smoothingMode,juce::dontSendNotification);repaint();}
void RTAPanel::setWindowCount(int count){if(count==1||count==2)windowCount.setSelectedId(count,juce::sendNotificationSync);}
void RTAPanel::setTemporalModeChoice(int choice){if(choice<1||choice>9||choice==temporalMode)return;temporalMode=choice;if(onTemporalModeChanged)onTemporalModeChanged(choice);}
void RTAPanel::updateComponentLanguage(bool spanish){spanishLanguage=spanish;windowCount.changeItemText(1,spanish?"1 Ventana":"1 Window");windowCount.changeItemText(2,spanish?"2 Ventanas":"2 Windows");bandedStyle.changeItemText(1,spanish?"Barras":"Bar");bandedStyle.changeItemText(2,spanish?juce::String::fromUTF8("Líneas"):"Line");bandedStyle.changeItemText(3,spanish?"Ambas":"Both");bandedStyle.changeItemText(4,"3D");smoothingChoiceBox.changeItemText(1,spanish?"Ninguno":"No Banding");smoothingChoiceBox.changeItemText(7,spanish?"Octava":"Octave");microphoneChoice.setTextWhenNothingSelected(spanish?juce::String::fromUTF8("Micrófono"):"Microphone");peakTracker.setButtonText(spanish?"Seguir pico":"Peak Tracker");refreshMicrophoneChoices();repaint();}
void RTAPanel::setLineThickness(float pixels){lineThickness=juce::jlimit(1.f,4.f,pixels);repaint();}
void RTAPanel::setFrozen(bool shouldFreeze){frozen=shouldFreeze;if(frozen)stopTimer();}
void RTAPanel::clearTraces(){traces.clear();spectrogramHistory.clear();spectrogramImage={};spectrogramDirty=true;++spectrogramGeneration;spectrogramHead=0;spectrogramTraceId=-1;lastUpdateMs=0;cursorVisible=false;cursorReading.clear();microphoneSignature.clear();microphoneChoice.clear(juce::dontSendNotification);publishCursor();repaint();}
void RTAPanel::setActive(bool active){if(frozen){stopTimer();return;}if(active&&!isTimerRunning()){timerCallback();startTimerHz(30);}else if(!active){stopTimer();clearTraces();}}
void RTAPanel::visibilityChanged(){if(!isShowing())stopTimer();}
juce::Rectangle<int> RTAPanel::plotArea()const{auto bounds=getLocalBounds().withTrimmedLeft(44).withTrimmedRight(8).withTrimmedTop(42).withTrimmedBottom(42);if(windowCount.getSelectedId()==2)bounds.setHeight(juce::jmax(1,(bounds.getHeight()-8)/2));return bounds;}
juce::Rectangle<int> RTAPanel::spectrogramArea()const{if(windowCount.getSelectedId()!=2)return {};auto r=plotArea();return {r.getX(),r.getBottom()+8,r.getWidth(),r.getHeight()};}
const RTAPanel::Trace* RTAPanel::selectedTrace()const{for(const auto& trace:traces)if(trace.id==microphoneChoice.getSelectedId()-1)return &trace;for(const auto& trace:traces)if(trace.selected)return &trace;return traces.empty()?nullptr:&traces.front();}
void RTAPanel::refreshMicrophoneChoices(){
 juce::String signature;for(const auto& trace:traces)signature+=juce::String(trace.id)+":"+trace.name+"\n";
 if(signature==microphoneSignature)return;
 int previous=microphoneChoice.getSelectedId();microphoneSignature=signature;
 microphoneChoice.clear(juce::dontSendNotification);
 int fallback=0;for(const auto& trace:traces){int id=trace.id+1;microphoneChoice.addItem(trace.name.isNotEmpty()?trace.name:juce::String(spanishLanguage?"Entrada ":"Input ")+juce::String(id),id);if(trace.selected)fallback=id;}
 if(previous>0&&microphoneChoice.indexOfItemId(previous)>=0)fallback=previous;
 if(fallback==0&&!traces.empty())fallback=traces.front().id+1;
 microphoneChoice.setSelectedId(fallback,juce::dontSendNotification);
}
int RTAPanel::peakIndex(const Trace& trace)const{float maximum=-std::numeric_limits<float>::infinity();int best=-1;int divisions=bandDivisions(smoothingMode);int count=divisions>0?(int)std::ceil(divisions*std::log2(std::max(20.,trace.nyquistHz)/20.))+1:(int)trace.dbfs.size();for(int i=0;i<count;++i){int index=divisions>0?juce::roundToInt(MultiWindowSpectrum::pointsPerOctave*i/(double)divisions):i;if(index<0||index>=(int)trace.dbfs.size())break;double hz=MultiWindowSpectrum::gridFrequency(index);if(hz<20.||hz>trace.nyquistHz)continue;if(std::isfinite(trace.dbfs[(size_t)index])&&trace.dbfs[(size_t)index]>maximum){maximum=trace.dbfs[(size_t)index];best=index;}}return best;}
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
 cursorVisible=true;cursorReading=juce::String(*hz,1)+" Hz / "+juce::String(*db,1)+" dBFS";publishCursor();
}
void RTAPanel::timerCallback(){
 if(frozen)return;
 auto incoming=dataProvider?dataProvider():std::vector<Trace>{};if(incoming.empty()){if(!traces.empty()||!spectrogramHistory.empty())clearTraces();return;}
 const double now=juce::Time::getMillisecondCounterHiRes();
 const double dt=lastUpdateMs>0.?juce::jlimit(0.,.25,(now-lastUpdateMs)/1000.):0.;
 lastUpdateMs=now;
 const auto rawSpectrum=incoming.front().dbfs;
 if(temporalMode==2||temporalMode==3){
  const double tau=temporalMode==2?.11:.38;
  const double retention=std::exp(-dt/tau);
  for(auto& current:incoming)for(const auto& previous:traces)if(previous.id==current.id&&previous.dbfs.size()==current.dbfs.size()){
   for(size_t i=0;i<current.dbfs.size();++i){auto& value=current.dbfs[i];const float old=previous.dbfs[i];if(std::isfinite(value)&&std::isfinite(old)&&value<old)value=(float)(value+(old-value)*retention);}
   break;
  }
 }
 traces=std::move(incoming);refreshMicrophoneChoices();
 if(const auto* chosen=selectedTrace()){if(spectrogramTraceId!=chosen->id){spectrogramHistory.clear();spectrogramHead=0;spectrogramTraceId=chosen->id;++spectrogramGeneration;}if(!rawSpectrum.empty()){if(spectrogramHistory.size()<spectrogramRows)spectrogramHistory.push_back(rawSpectrum);else{spectrogramHistory[spectrogramHead]=rawSpectrum;spectrogramHead=(spectrogramHead+1)%spectrogramRows;}spectrogramDirty=true;updateSpectrogramImage();}if(cursorVisible&&peakTracker.getToggleState()){int index=peakIndex(*chosen);if(index>=0)updateCursor(*chosen,plotArea(),index);}}
}
void RTAPanel::resized(){peakTracker.setBounds(juce::jmax(8,getWidth()-114),8,104,23);windowCount.setBounds(juce::jmax(8,getWidth()-236),8,114,23);bandedStyle.setBounds(juce::jmax(8,getWidth()-342),8,100,23);microphoneChoice.setBounds(juce::jmax(8,getWidth()-480),8,130,23);auto plot=plotArea();smoothingChoiceBox.setBounds(plot.getRight()-112,plot.getBottom()-26,108,22);spectrogramDirty=true;++spectrogramGeneration;updateSpectrogramImage();repaint();}
void RTAPanel::mouseMove(const juce::MouseEvent& event){auto p=plotArea();if(!p.contains(event.getPosition())){mouseExit(event);return;}const auto* chosen=selectedTrace();cursorVisible=false;cursorReading.clear();if(chosen&&chosen->dbfs.size()>1){double index=juce::jlimit(0.0,(double)chosen->dbfs.size()-1.0,(double)(event.position.x-p.getX())*(double)(chosen->dbfs.size()-1)/(double)p.getWidth());if(peakTracker.getToggleState())index=peakIndex(*chosen);if(index>=0){cursorVisible=true;updateCursor(*chosen,p,index);}}else publishCursor();repaint();}
void RTAPanel::mouseExit(const juce::MouseEvent&){cursorVisible=false;cursorReading.clear();publishCursor();repaint();}
void RTAPanel::mouseDown(const juce::MouseEvent& event){
 auto area=spectrogramArea();if(area.isEmpty()||event.position.x<area.getX()-16||event.position.x>area.getX()+2)return;
 auto yFor=[&](double db){return (float)area.getY()+(float)(-db/120.)*(float)area.getHeight();};
 float upperDistance=std::abs(event.position.y-yFor(upperLimit)),lowerDistance=std::abs(event.position.y-yFor(lowerLimit));
 if(juce::jmin(upperDistance,lowerDistance)>12.f)return;
 draggedLimit=upperDistance<=lowerDistance?DragLimit::Upper:DragLimit::Lower;
 dragLimitTo(event.position.y);
}
void RTAPanel::mouseDrag(const juce::MouseEvent& event){if(draggedLimit!=DragLimit::None)dragLimitTo(event.position.y);else mouseMove(event);}
void RTAPanel::mouseUp(const juce::MouseEvent&){draggedLimit=DragLimit::None;}
void RTAPanel::dragLimitTo(float y){
 auto area=spectrogramArea();if(area.isEmpty())return;
 double value=std::round(-120.*juce::jlimit(0.f,1.f,(y-area.getY())/(float)area.getHeight()));
 if(draggedLimit==DragLimit::Upper)upperLimit=juce::jlimit(lowerLimit+1.,0.,value);
 else if(draggedLimit==DragLimit::Lower)lowerLimit=juce::jlimit(-120.,upperLimit-1.,value);
 spectrogramDirty=true;repaint();
}
void RTAPanel::updateSpectrogramImage(){
 if(!spectrogramDirty||pendingSpectrogram.valid())return;
 if(spectrogramHistory.empty()||spectrogramHistory.front().empty()){spectrogramImage={};spectrogramDirty=false;return;}
 const auto viewport=spectrogramArea();if(viewport.isEmpty())return;
 auto history=spectrogramHistory;
 const auto head=spectrogramHead;
 const auto width=juce::jlimit(1,2048,viewport.getWidth()),height=juce::jlimit(1,1024,viewport.getHeight());
 const double lower=lowerLimit,upper=upperLimit;
 const unsigned token=spectrogramGeneration;
 spectrogramDirty=false;
 pendingSpectrogram=std::async(std::launch::async,[history=std::move(history),head,width,height,lower,upper,token]() mutable {
  const auto count=history.size();
  juce::Image image(juce::Image::RGB,width,height,true);
  juce::Image::BitmapData pixels(image,juce::Image::BitmapData::writeOnly);
  const auto orderedRow=[&](size_t age)->const std::vector<float>&{age=juce::jmin(age,count-1);return history[count<spectrogramRows?count-1-age:(head+spectrogramRows-1-age)%spectrogramRows];};
  for(int y=0;y<height;++y){
   const double age=(double)y/(double)juce::jmax(1,height-1)*(double)(count-1);
   const size_t row0=(size_t)age,row1=juce::jmin(row0+1,count-1);
   const auto& a=orderedRow(row0);const auto& b=orderedRow(row1);const double fy=age-(double)row0;
   const size_t bins=juce::jmin(a.size(),b.size());if(bins==0)continue;
   for(int x=0;x<width;++x){
    const double position=(double)x/(double)juce::jmax(1,width-1)*(double)(bins-1);
    const size_t bin0=(size_t)position,bin1=juce::jmin(bin0+1,bins-1);const double fx=position-(double)bin0;
    const double top=a[bin0]+(a[bin1]-a[bin0])*fx,bottom=b[bin0]+(b[bin1]-b[bin0])*fx;
    pixels.setPixelColour(x,y,heat((float)(top+(bottom-top)*fy),lower,upper));
   }
  }
  return std::make_pair(token,std::move(image));
 });
}
void RTAPanel::presentFrame(double){
 if(pendingSpectrogram.valid()&&pendingSpectrogram.wait_for(std::chrono::seconds(0))==std::future_status::ready){
  auto ready=pendingSpectrogram.get();if(ready.first==spectrogramGeneration)spectrogramImage=std::move(ready.second);
 }
 updateSpectrogramImage();
 if(isShowing())repaint();
}
void RTAPanel::paint(juce::Graphics& g){
 using namespace juce;const Colour plot(0xff080a0c),line(0xff626e78),muted(0xffa9bac8),green(0xff54ee22),orange(0xffffac24);auto bounds=getLocalBounds();g.fillAll(plot);auto p=plotArea();if(p.isEmpty())return;
 if(bandedStyle.getSelectedId()==4){
  g.setColour(Colour(0xff101b26));g.fillRoundedRectangle(p.toFloat(),12.f);
  auto logo=ImageCache::getFromMemory(MyaBrandAssets::myaanalyzericonv1_png,MyaBrandAssets::myaanalyzericonv1_pngSize);
  if(logo.isValid()){auto size=jmin(p.getWidth()/3,p.getHeight()/2);g.setOpacity(.065f);g.drawImageWithin(logo,p.getCentreX()-size/2,p.getCentreY()-size/2,size,size,RectanglePlacement::centred);g.setOpacity(1.f);}
  g.setColour(Colour(0xffa9c9d8));g.setFont(16.f);
  g.drawFittedText(spanishLanguage?String::fromUTF8("Modo 3D: Preparado para inyección de malla de aceleración por hardware"):String::fromUTF8("3D Mode: Ready for hardware acceleration mesh injection"),p.reduced(24),Justification::centred,2);
  return;
 }
 g.setColour(line);g.drawRect(p);
 for(int db=-120;db<=0;db+=20){float y=(float)(p.getBottom()-(db+120.)/120.*p.getHeight());g.setColour(line.withAlpha(.55f));g.drawHorizontalLine((int)y,(float)p.getX(),(float)p.getRight());g.setColour(muted);g.setFont(10.f);g.drawText(String(db),5,(int)y-9,43,18,Justification::centredLeft);}
 for(double hz:PlotPresentation::frequencyTicks){float x=(float)(p.getX()+std::log(hz/20.)/std::log(1000.)*p.getWidth());g.setColour(line.withAlpha(.45f));g.drawVerticalLine((int)x,(float)p.getY(),(float)p.getBottom());g.setColour(muted);g.setFont(10.f);g.drawText(hz>=1000?String(hz/1000.,hz==1000?0:1)+"k":String(hz,0),(int)x-24,p.getBottom()+5,48,18,Justification::centred);}
 for(const auto& trace:traces){
  if(microphoneChoice.getSelectedId()>0&&trace.id!=microphoneChoice.getSelectedId()-1)continue;
  if(trace.dbfs.size()<2)continue;
  const int divisions=bandDivisions(smoothingMode);
  g.setColour(trace.colour);
  if(bandedStyle.getSelectedId()!=2&&divisions>0){
   int bands=(int)std::ceil(divisions*std::log2(std::min(20000.,trace.nyquistHz)/20.));
   for(int band=0;band<=bands;++band){double center=20.*std::pow(2.,band/(double)divisions);if(center>trace.nyquistHz||center>20000.)break;int index=juce::jlimit(0,(int)trace.dbfs.size()-1,juce::roundToInt(MultiWindowSpectrum::pointsPerOctave*std::log2(center/20.)));float db=trace.dbfs[(size_t)index];if(!std::isfinite(db))continue;double lower=center*std::pow(2.,-.5/divisions),upper=center*std::pow(2.,.5/divisions);float left=(float)(p.getX()+std::log(std::max(20.,lower)/20.)/std::log(1000.)*p.getWidth()),right=(float)(p.getX()+std::log(std::min(20000.,upper)/20.)/std::log(1000.)*p.getWidth());float y=(float)(p.getBottom()-jlimit(0.f,120.f,db+120.f)/120.f*p.getHeight());g.fillRect(juce::Rectangle<float>(left,y,std::max(1.f,right-left-1.f),(float)p.getBottom()-y));}
  }
  Path path;bool started=false;
  for(size_t i=0;i<trace.dbfs.size();++i){if(!std::isfinite(trace.dbfs[i])){started=false;continue;}float x=(float)p.getX()+(float)i/(float)(trace.dbfs.size()-1)*(float)p.getWidth();float y=(float)(p.getBottom()-jlimit(0.f,120.f,trace.dbfs[i]+120.f)/120.f*p.getHeight());if(bandedStyle.getSelectedId()!=2&&divisions==0)g.drawLine(x,y,x,(float)p.getBottom(),juce::jmax(1.f,(float)p.getWidth()/(float)trace.dbfs.size()*.75f));if(!started){path.startNewSubPath(x,y);started=true;}else path.lineTo(x,y);}
  if(bandedStyle.getSelectedId()!=1)g.strokePath(path,PathStrokeType(lineThickness));
 }
 auto s=spectrogramArea();if(!s.isEmpty()){
  g.setColour(line);g.drawRect(s);
  if(spectrogramImage.isValid()){g.setImageResamplingQuality(Graphics::mediumResamplingQuality);g.drawImage(spectrogramImage,s.toFloat(),RectanglePlacement::stretchToFit);}
  auto handle=[&](double db,Colour colour){float y=(float)s.getY()+(float)(-db/120.)*(float)s.getHeight();Path triangle;triangle.addTriangle((float)s.getX()-12.f,y-5.f,(float)s.getX()-12.f,y+5.f,(float)s.getX()-2.f,y);g.setColour(colour);g.fillPath(triangle);g.setFont(10.f);g.drawFittedText(String(db,0),3,roundToInt(y)-8,36,16,Justification::centredRight,1);};
  handle(upperLimit,Colour(0xffffdd70));handle(lowerLimit,Colour(0xff51bfff));
 }
 g.setColour(traces.empty()?orange:green);g.setFont(13.f);g.drawText(cursorReading.isNotEmpty()?cursorReading:traces.empty()?(spanishLanguage?juce::String::fromUTF8("RTA sin señal: pulsa Play en una TF configurada"):"RTA without signal: press Play on a configured TF"):(spanishLanguage?"RTA en directo: ":"Live RTA: ")+selectedTrace()->name,10,7,jmax(0,bounds.getWidth()-610),23,Justification::centredLeft);
 if(cursorVisible){g.setColour(Colour(0xff00e5ed).withAlpha(.9f));g.drawVerticalLine(roundToInt(cursorX),(float)p.getY(),(float)p.getBottom());g.drawHorizontalLine(roundToInt(cursorY),(float)p.getX(),(float)p.getRight());}
}
