// Paint actual components into images, without creating a native window or starting audio.
#define MYA_OFFSCREEN_RENDER 1
#include "../src/Main.cpp"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 2;juce::ScopedJuceInitialiser_GUI init;juce::File(juce::String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("MYA-006-A-fixture").createDirectory();
 if(juce::String::fromUTF8(argv[1])=="--axis-grid-steps"){
  TransferFunctionPanel panel([]{return std::vector<TransferFunctionPanel::Trace>{};});
  panel.setSize(800,500);panel.setPlotLayout({0,0,800,230},{0,238,800,262});panel.refreshNow();
  std::array<double,3> pan{},zoom{{1.,1.,1.}};
  auto checkGrid=[&](const juce::Image& image,int top,int height,double minimum,double maximum,double step,double verticalZoom,int index){
   const int plotTop=top+23,plotHeight=height-48;
   const double centre=(minimum+maximum)*.5,span=maximum-minimum;
   std::vector<int> expected,actual;
   for(int tick=(int)std::ceil(minimum/step);tick<=(int)std::floor(maximum/step);++tick){
    const double value=tick*step;
    const int y=juce::roundToInt(plotTop+plotHeight*.5-(value-centre)/span*verticalZoom*plotHeight);
    if(y>plotTop+2&&y<plotTop+plotHeight-2)expected.push_back(y);
   }
   for(int y=plotTop+3;y<plotTop+plotHeight-2;++y){
    int pixels=0;
    for(int x=100;x<700;++x)if(image.getPixelAt(x,y)==juce::Colour(0xff2c3135))++pixels;
    if(pixels>400)actual.push_back(y);
   }
   if(actual.size()!=expected.size()){
    std::cerr<<"Grid "<<index<<" zoom "<<verticalZoom<<": expected "<<expected.size()<<" rows, got "<<actual.size()<<"\n";
    return false;
   }
   for(size_t i=0;i<expected.size();++i)if(std::abs(actual[i]-expected[expected.size()-1-i])>1){
    std::cerr<<"Grid "<<index<<" zoom "<<verticalZoom<<": row "<<actual[i]<<" differs from "<<expected[expected.size()-1-i]<<"\n";
    return false;
   }
   return true;
  };
  auto baseline=panel.createComponentSnapshot(panel.getLocalBounds());
  if(!checkGrid(baseline,0,230,-180.,180.,30.,1.,1)||!checkGrid(baseline,238,262,-18.,15.,3.,1.,2))return 122;
  zoom[1]=zoom[2]=2.;panel.setPlotTransforms(pan,pan,std::array<double,3>{{1.,1.,1.}},zoom);
  auto enlarged=panel.createComponentSnapshot(panel.getLocalBounds());
  if(!checkGrid(enlarged,0,230,-180.,180.,30.,2.,1)||!checkGrid(enlarged,238,262,-18.,15.,3.,2.,2))return 123;
  std::cout<<"PASS fixed 30-degree phase and 3-dB magnitude grid at normal and 2x vertical zoom\n";
  return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--floating-meter-grid"){
  auto fixture=juce::File(juce::String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("floating-grid-"+juce::Uuid().toString());if(!fixture.createDirectory())return 124;
  MeterState principal(fixture.getChildFile("principal.json")),floating(fixture.getChildFile("floating.json"),&principal);
  auto dockFile=fixture.getChildFile("dock.json");MeterState dock(dockFile,&principal);
  if(!dock.add()||!dock.setGrid(5,1)||!dock.sidebar(true))return 136;
  MeterPanel fixed(dock,true,&principal);fixed.setSize(148,600);
  PrincipalMeter main(principal);main.setSize(224,194);
  auto fixedState=dockFile.loadFileAsString();auto fixedBounds=fixed.getBounds();auto mainBounds=main.getBounds();
  auto fixedImage=fixed.createComponentSnapshot(fixed.getLocalBounds());
  auto mainImage=main.createComponentSnapshot(main.getLocalBounds());
  if(!fixed.previewPages()||!fixedImage.isValid()||!mainImage.isValid())return 137;
  MeterPanel panel(floating,false,&principal);panel.setSize(480,310);if(!panel.previewPages()||!panel.previewFloatingCellMinimums())return 125;
  const auto originalCards=floating.state().cards;
  auto baseline=panel.createComponentSnapshot(panel.getLocalBounds());
  if(!panel.addMeterRight())return 126;panel.refreshNow();if(floating.state().rows!=2||floating.state().columns!=3||floating.state().cards.size()!=6||!panel.previewPages()||!panel.previewFloatingCellMinimums())return 127;
  auto wider=panel.createComponentSnapshot(panel.getLocalBounds());
  if(!panel.addMeterBelow())return 128;panel.refreshNow();if(floating.state().rows!=3||floating.state().columns!=3||floating.state().cards.size()!=9||panel.addMeterBelow()||!panel.previewPages()||!panel.previewFloatingCellMinimums())return 129;
  auto taller=panel.createComponentSnapshot(panel.getLocalBounds());
  auto changed=[](const juce::Image& a,const juce::Image& b){int count=0;for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)count+=a.getPixelAt(x,y)!=b.getPixelAt(x,y);return count;};
  if(!baseline.isValid()||!wider.isValid()||!taller.isValid()||changed(baseline,wider)<500||changed(wider,taller)<500)return 130;
  if(!panel.addMeterRight()||!panel.addMeterRight())return 131;panel.refreshNow();if(floating.state().rows!=3||floating.state().columns!=5||floating.state().cards.size()!=15||!panel.previewFloatingHorizontalScroll()||!panel.previewFloatingCellMinimums())return 132;
  panel.setSize(800,570);if(panel.previewFloatingHorizontalScroll()||!panel.previewFloatingCellMinimums()||principal.state().rows!=2||principal.state().columns!=2||principal.state().cards.size()!=4)return 133;
  if(!panel.removeMeterRight())return 142;panel.refreshNow();if(floating.state().rows!=3||floating.state().columns!=4||floating.state().cards.size()!=12||!panel.previewPages()||!panel.previewFloatingCellMinimums())return 143;
  if(!panel.removeMeterBelow())return 144;panel.refreshNow();if(floating.state().rows!=2||floating.state().columns!=4||floating.state().cards.size()!=8||!panel.previewPages()||!panel.previewFloatingCellMinimums())return 145;
  for(int row=0;row<2;++row)for(int column=0;column<2;++column)if(floating.state().cards[(size_t)row*4+(size_t)column].id!=originalCards[(size_t)row*2+(size_t)column].id)return 146;
  if(dockFile.loadFileAsString()!=fixedState||dock.state().rows!=5||dock.state().columns!=1||dock.state().cards.size()!=5||fixed.getBounds()!=fixedBounds||main.getBounds()!=mainBounds)return 138;
  if(changed(fixedImage,fixed.createComponentSnapshot(fixed.getLocalBounds()))!=0||changed(mainImage,main.createComponentSnapshot(main.getLocalBounds()))!=0)return 139;
  if(!floating.setGrid(4,5))return 134;MeterWindow window(floating,&principal);juce::Button* gear=nullptr;for(auto* child:window.getChildren())if(auto* button=dynamic_cast<juce::TextButton*>(child))if(button->getButtonText()==juce::String::fromUTF8("⚙"))gear=button;
  if(floating.state().rows!=3||window.isUsingNativeTitleBar()||!gear||!gear->isVisible()||!window.getTitleBarArea().contains(gear->getBounds()))return 134;
  auto titleSnapshot=window.createComponentSnapshot(window.getLocalBounds());juce::FileOutputStream imageFile(fixture.getChildFile("floating-meter-titlebar.png"));if(!titleSnapshot.isValid()||!imageFile.openedOk()||!juce::PNGImageFormat{}.writeImageToStream(titleSnapshot,imageFile))return 135;
  std::cout<<"PASS floating meter add/remove columns and rows, strict three-row limit, responsive tiles and isolated fixed rack\n";return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--qa-fixed-racks"){
  WorkstationSurface surface;surface.setSize(1440,960);
  if(!surface.station.previewFixedRacksAfterFloatingGrowth())return 140;
  auto image=surface.createComponentSnapshot(surface.getLocalBounds());
  auto file=juce::File(juce::String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("qa-fixed-racks.png");
  juce::FileOutputStream stream(file);
  if(!image.isValid()||!stream.openedOk()||!juce::PNGImageFormat{}.writeImageToStream(image,stream))return 141;
  std::cout<<"PASS fixed five-meter vertical rack and main/TF bounds unchanged by floating grid growth\n";return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--trace-visibility"){WorkstationSurface surface;return surface.station.previewHiddenTraceKeepsAverage()?0:114;}
 if(juce::String::fromUTF8(argv[1])=="--sample-validity"){
  TransferFunctionCore::Point phase,magnitude;phase.phaseDegrees=90.;phase.coherence=.9;magnitude.magnitudeDb=-6.;magnitude.coherence=.8;
  auto phaseOnly=analyzerLiveSample(1000.,phase,{},false);
  auto magnitudeOnly=analyzerLiveSample(2000.,{},magnitude,false);
  auto both=analyzerLiveSample(3000.,phase,magnitude,false);
  auto neither=analyzerLiveSample(4000.,{},{},false);
  if(!phaseOnly.valid||!phaseOnly.validPhase||phaseOnly.validMagnitude||!magnitudeOnly.valid||magnitudeOnly.validPhase||!magnitudeOnly.validMagnitude||!both.validPhase||!both.validMagnitude||neither.valid||neither.validPhase||neither.validMagnitude)return 117;
  TransferFunctionPanel::Trace trace;trace.id=1;trace.samples={phaseOnly,magnitudeOnly,both,neither};
  TransferFunctionPanel panel([&]{return std::vector<TransferFunctionPanel::Trace>{trace};});
  panel.setSize(800,500);panel.setPlotLayout({0,0,800,230},{0,238,800,262});panel.refreshNow();
  return panel.sampleAt(1,1000.,true)&&!panel.sampleAt(1,1000.,false)&&panel.sampleAt(1,2000.,false)&&!panel.sampleAt(1,2000.,true)&&panel.sampleAt(1,3000.,true)&&panel.sampleAt(1,3000.,false)&&!panel.sampleAt(1,4000.,true)&&!panel.sampleAt(1,4000.,false)?0:118;
 }
 if(juce::String::fromUTF8(argv[1])=="--graph-focus"){
  for(auto size:{juce::Point<int>{1120,900},juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){WorkstationSurface surface;surface.setSize(size.x,size.y);if(!surface.station.previewGraphFocus())return 115;}
  return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--hit-regions"){
  for(auto size:{juce::Point<int>{1120,900},juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){WorkstationSurface surface;surface.setSize(size.x,size.y);surface.setVisible(true);auto failure=surface.station.previewHitRegions(surface);if(failure.isNotEmpty())std::cerr<<size.x<<"x"<<size.y<<": "<<failure<<"\n";if(failure.isNotEmpty()||!surface.station.previewFindPressIsolation())return 116;}
  return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--mouse-test"){
  auto describe=[](const juce::Component* component){
   if(component==nullptr)return juce::String("none");
   auto name=component->getTitle();
   if(name.isEmpty())name=component->getName();
   if(auto* button=dynamic_cast<const juce::Button*>(component))name+=" ["+button->getButtonText()+"]";
   if(name.isNotEmpty())return name;
   if(auto* label=dynamic_cast<const juce::Label*>(component))return juce::String("Label [")+label->getText()+"]";
   if(dynamic_cast<const juce::TextEditor*>(component))return juce::String("TextEditor");
   if(dynamic_cast<const juce::Slider*>(component))return juce::String("Slider");
   return juce::String("unnamed child");
  };
  auto contains=[](const juce::Component* target,const juce::Component* received){
   return received!=nullptr&&(received==target||target->isParentOf(received));
  };
  for(auto size:{juce::Point<int>{1120,900},juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){
   WorkstationSurface surface;
   surface.setSize(size.x,size.y);
   surface.setVisible(true);
   auto failure=surface.station.previewHitRegions(surface);
   if(failure.isNotEmpty()){std::cerr<<"Local layout "<<size.x<<"x"<<size.y<<": "<<failure<<"\n";return 150;}
  }
  {
   auto fixture=juce::File(juce::String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("mouse-test-"+juce::Uuid().toString());
   if(!fixture.createDirectory())return 149;
   juce::PropertiesFile::Options options;
   options.applicationName="mya-qa-mouse-test";
   options.filenameSuffix=".settings";
   options.osxLibrarySubFolder="Application Support";
   options.folderName=fixture.getFullPathName();
   juce::ApplicationProperties preferences;
   preferences.setStorageParameters(options);
   auto* properties=preferences.getUserSettings();
   if(properties==nullptr)return 155;
   properties->setValue("window.bounds",juce::Rectangle<int>(30,45,1120,900).toString());
   properties->setValue("window.fullScreen",false);
   properties->setValue("isWindowMaximized",true);
   MainWindow window(preferences);
   window.showRestored([]{});
   window.toFront(true);
   auto* surface=dynamic_cast<WorkstationSurface*>(window.getContentComponent());
   if(surface==nullptr)return 147;
   auto* peer=window.getPeer();
   if(peer==nullptr||!window.isUsingNativeTitleBar())return 156;
   const auto client=peer->getBounds();
   const auto windowBounds=window.getBounds();
   const auto frame=peer->getFrameSize();
   const auto outer=frame.addedTo(client);
   auto* display=juce::Desktop::getInstance().getDisplays().getDisplayForRect(outer);
   if(display==nullptr)display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
   std::cout<<"Maximized native window: frame="<<frame.getLeft()<<","<<frame.getTop()<<","<<frame.getRight()<<","<<frame.getBottom()
            <<" peer="<<client.toString()<<" MainWindow="<<windowBounds.toString()
            <<" outer="<<outer.toString()<<" content="<<surface->getBounds().toString()
            <<" userArea="<<(display==nullptr?juce::String("none"):display->userArea.toString())<<"\n";
   if(client!=windowBounds||surface->getWidth()!=client.getWidth()||surface->getHeight()!=client.getHeight()){
    std::cerr<<"Native peer, MainWindow and content bounds disagree after showRestored\n";
    return 154;
   }
   if(display==nullptr||display->userArea.isEmpty())
    std::cout<<"SKIP maximized monitor coverage: no valid display in CLI QA process\n";
   else if(outer!=display->userArea){
    std::cerr<<"Native window frame does not cover the display user area after showRestored\n";
    return 157;
   }else std::cout<<"PASS maximized native frame covers display user area\n";
   auto findTarget=[&](const std::function<bool(juce::Component&)>& matches)->juce::Component*{
    std::function<juce::Component*(juce::Component&)> visit=[&](juce::Component& parent)->juce::Component*{
     for(auto* child:parent.getChildren()){
      if(matches(*child)&&child->isShowing())return child;
      if(auto* found=visit(*child))return found;
     }
     return nullptr;
    };
    return visit(*surface);
   };
   auto* find=findTarget([](juce::Component& component){if(auto* button=dynamic_cast<juce::TextButton*>(&component))return button->getButtonText()=="Find";return false;});
   auto* smoothing=findTarget([](juce::Component& component){return component.getTitle()=="Suavizado de la pantalla activa";});
   auto* phaseSmoothing=findTarget([](juce::Component& component){return component.getTitle().startsWith("Fase / Phase")&&component.getTitle().contains("smoothing");});
   auto* magnitudeSmoothing=findTarget([](juce::Component& component){return component.getTitle().startsWith("Magnitud / Magnitude")&&component.getTitle().contains("smoothing");});
   auto* delay=findTarget([](juce::Component& component){return component.getTitle().startsWith("Delay de medicion MICRO 1");});
   auto* coherence=findTarget([](juce::Component& component){if(auto* button=dynamic_cast<juce::Button*>(&component))return button->getButtonText()=="Coh";return false;});
   const std::array<std::pair<const char*,juce::Component*>,6> targets{{{"Find",find},{"Suavizado",smoothing},{"Suavizado Fase",phaseSmoothing},{"Suavizado Magnitud",magnitudeSmoothing},{"Milisegundos",delay},{"Coh",coherence}}};
   for(const auto& [name,target]:targets)if(target==nullptr){std::cerr<<"Missing mouse-test target "<<name<<"\n";return 148;}
   MeterState primary(fixture.getChildFile("primary.json")),floating(fixture.getChildFile("floating.json"),&primary);
   MeterWindow meterWindow(floating,&primary);
   meterWindow.setVisible(false);
   auto testState=[&](const char* state)->bool{
    auto failure=surface->station.previewHitRegions(*surface);
    if(failure.isNotEmpty()){std::cerr<<window.getWidth()<<"x"<<window.getHeight()<<" "<<state<<": "<<failure<<"\n";return false;}
    for(const auto& [name,target]:targets){
     auto screen=target->getScreenBounds().getCentre();
     auto local=target->getLocalPoint(nullptr,screen);
     auto* hit=window.getComponentAt(window.getLocalPoint(nullptr,screen));
     auto* stationHit=surface->station.getComponentAt(surface->station.getLocalPoint(nullptr,screen));
     std::cout<<window.getWidth()<<"x"<<window.getHeight()<<" "<<state<<" "<<name<<" screen="<<screen.toString()
              <<" local="<<local.toString()<<" bounds="<<target->getScreenBounds().toString()
              <<" expected="<<describe(target).toStdString()<<" windowHit="<<describe(hit).toStdString()
              <<" stationHit="<<describe(stationHit).toStdString()<<"\n";
     if(!contains(target,hit)||!contains(target,stationHit)
        ||std::abs(local.x-target->getWidth()/2)>1||std::abs(local.y-target->getHeight()/2)>1)return false;
    }
    return true;
   };
   if(!testState("cerrada"))return 150;
   meterWindow.setVisible(true);
   if(!testState("abierta"))return 151;
   meterWindow.maximiseButtonPressed();
   if(!testState("ampliada"))return 152;
   meterWindow.maximiseButtonPressed();
   if(!testState("restaurada"))return 153;
   meterWindow.setVisible(false);
   window.setVisible(false);
  }
  std::cout<<"PASS JUCE local centre hit geometry across floating meter states. Native mouseDown/mouseUp receivers require the in-app GeometryProbe and a physical UI click.\n";
  return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--geometry-telemetry"){
  for(auto size:{juce::Point<int>{1120,900},juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){
   WorkstationSurface surface;surface.setSize(size.x,size.y);surface.setVisible(true);
   std::cout<<surface.station.previewGeometryTelemetry(surface).toStdString();
   if(surface.station.previewHitRegions(surface).isNotEmpty())return 119;
  }
  return 0;
 }
 if(juce::String::fromUTF8(argv[1])=="--cursor-direct-click"){
  for(bool frozen:{false,true})for(int plot:{1,2}){
   TransferFunctionPanel::Trace trace;trace.id=1;trace.selected=true;trace.frozen=frozen;
   trace.samples={{20.,-6.,30.,1.,true},{20000.,-6.,30.,1.,true}};
   TransferFunctionPanel panel([&]{return std::vector<TransferFunctionPanel::Trace>{trace};});
   panel.setSize(800,500);panel.setPlotLayout({0,0,800,230},{0,238,800,262});panel.refreshNow();
   auto baseline=panel.createComponentSnapshot(panel.getLocalBounds());
   int cursorPlot=-1;bool validSample=false;
   panel.onCursorMoved=[&](int selected,double,std::optional<TransferFunctionPanel::Sample> sample){cursorPlot=selected;validSample=sample&&sample->valid;};
   auto* pane=panel.getChildComponent(plot-1);const juce::Point<float> point{400.f,100.f};auto now=juce::Time::getCurrentTime();
   juce::MouseEvent click(juce::Desktop::getInstance().getMainMouseSource(),point,juce::ModifierKeys{},1.f,0.f,0.f,0.f,0.f,pane,pane,now,point,now,1,false);
   pane->mouseDown(click);
   if(cursorPlot!=plot||!validSample)return 120;
   auto active=panel.createComponentSnapshot(panel.getLocalBounds());
   auto changed=[&](juce::Rectangle<int> area){int count=0;for(int y=area.getY();y<area.getBottom();++y)for(int x=area.getX();x<area.getRight();++x)count+=baseline.getPixelAt(x,y)!=active.getPixelAt(x,y);return count;};
   if(changed({0,0,800,230})<100||changed({0,238,800,262})<100)return 121;
  }
  return 0;
 }
 juce::File out(juce::String::fromUTF8(argv[1]));if(!out.createDirectory())return 3;
 auto write=[&](const juce::Image& image,juce::String name){juce::FileOutputStream stream(out.getChildFile(name));if(!stream.openedOk()||!stream.setPosition(0)||!stream.truncate())return false;return juce::PNGImageFormat{}.writeImageToStream(image,stream);};
 if(MyaIdentity::name()!=juce::String::fromUTF8("Mya Acústica Analyzer")||MyaIdentity::brand()+" "+MyaIdentity::descriptor()!=MyaIdentity::name())return 74;
 for(auto pair:{std::pair<float,int>{25,400},{23,396}})if(juce::GlyphArrangement::getStringWidth(juce::Font(juce::FontOptions(pair.first)),MyaIdentity::brand())>pair.second)return 75;
 for(auto pair:{std::pair<float,int>{18,400},{17,396}})if(juce::GlyphArrangement::getStringWidth(juce::Font(juce::FontOptions(pair.first)),MyaIdentity::descriptor())>pair.second)return 76;
 #if JUCE_MAC
 auto titleWidth=myaNativeTitleRequiredWidth();if(titleWidth<=24||titleWidth>1120-240)return 77;std::cout<<"PASS native title metrics: "<<titleWidth<<" px incl. icon, centred at minimum window width1120, no native window created\n";
 #endif
 {MyaAboutPanel about;if(!about.assetValid())return 34;if(!write(about.createComponentSnapshot(about.getLocalBounds()),"about-Mya-Acustica-Analyzer.png"))return 35;}
 {juce::StringArray outputs,inputs;for(int i=1;i<=4;++i)outputs.add("Output "+juce::String(i));for(int i=1;i<=7;++i)inputs.add("Input "+juce::String(i));AudioOptionsView panel(true,"Another device [CoreAudio]","Output 4","Input 7",false,-55,6,outputs,4,inputs,7,1,{"TF 1","TF 2","TF 3","TF 4"},2,{"OCTA-CAPTURE [CoreAudio]","Another device [CoreAudio]"},2);panel.updatePhysicalLoop(false,true,{}, {},false,26.35,false,juce::String::fromUTF8("Calibración guardada"),false,true,-55,6);if(!write(panel.createComponentSnapshot(panel.getLocalBounds()),"loop-saved-delay-no-audio.png"))return 81;bool resetEnabled=false,calibrateEnabled=false,pinkValue=false,referenceValue=false,storedDelay=false,outputRoute=false,referenceRoute=false,tfRoute=false,deviceRoute=false;for(auto* child:panel.getChildren()){if(auto* button=dynamic_cast<juce::TextButton*>(child)){if(button->getButtonText()=="Reset Loop")resetEnabled=button->isEnabled();if(button->getButtonText()=="Calib Loop")calibrateEnabled=button->isEnabled();}if(auto* slider=dynamic_cast<juce::Slider*>(child)){pinkValue|=std::abs(slider->getValue()+55)<.01;referenceValue|=std::abs(slider->getValue()-6)<.01;}if(auto* combo=dynamic_cast<juce::ComboBox*>(child)){outputRoute|=combo->getText()=="Output 4";referenceRoute|=combo->getText()=="Input 7";tfRoute|=combo->getText()=="TF 2";deviceRoute|=combo->getText()=="Another device [CoreAudio]";}if(auto* label=dynamic_cast<juce::Label*>(child))if(label->getText().startsWith("Loop Delay:"))storedDelay=label->getText().contains(juce::String::fromUTF8("26.35 ms · guardado / pendiente de validar"));}if(!resetEnabled||calibrateEnabled||!pinkValue||!referenceValue||!storedDelay||!outputRoute||!referenceRoute||!tfRoute||!deviceRoute)return 80;}
 {auto message=MyaUpdate::currentMessage(MyaIdentity::version());if(message!=juce::String::fromUTF8("Mya Acústica Analyzer está actualizada. Versión instalada: 0.3.5."))return 82;juce::Label updateLabel;updateLabel.setText(message,juce::dontSendNotification);updateLabel.setSize(560,60);if(!write(updateLabel.createComponentSnapshot(updateLabel.getLocalBounds()),"update-current-0.3.5.png"))return 83;}
 {
  TransferFunctionPanel panel([]{return std::vector<TransferFunctionPanel::Trace>{};});
  panel.setSize(800,500);panel.setPlotLayout({0,0,800,230},{0,238,800,262});
  std::array<double,3> panX{},panY{},zoomX{{1.,1.,1.}},zoomY{{1.,1.,1.}};
  panel.setPlotTransforms(panX,panY,zoomX,zoomY);
  auto baseline=panel.createComponentSnapshot(panel.getLocalBounds());
  auto changes=[](const juce::Image& a,const juce::Image& b,juce::Rectangle<int> area){int count=0;for(int y=area.getY();y<area.getBottom();++y)for(int x=area.getX();x<area.getRight();++x)count+=a.getPixelAt(x,y)!=b.getPixelAt(x,y);return count;};
  zoomY[1]=2.;panel.setPlotTransforms(panX,panY,zoomX,zoomY);
  auto phaseZoom=panel.createComponentSnapshot(panel.getLocalBounds());
  if(changes(baseline,phaseZoom,{0,0,800,230})==0||changes(baseline,phaseZoom,{0,238,800,262})!=0)return 85;
  zoomY[1]=1.;zoomX[1]=2.;panel.setPlotTransforms(panX,panY,zoomX,zoomY);
  auto phaseHorizontal=panel.createComponentSnapshot(panel.getLocalBounds());
  if(changes(baseline,phaseHorizontal,{0,0,800,230})==0||changes(baseline,phaseHorizontal,{0,238,800,262})!=0)return 88;
  zoomX[1]=1.;zoomY[2]=2.;panel.setPlotTransforms(panX,panY,zoomX,zoomY);
  auto magnitudeZoom=panel.createComponentSnapshot(panel.getLocalBounds());
  if(changes(baseline,magnitudeZoom,{0,0,800,230})!=0||changes(baseline,magnitudeZoom,{0,238,800,262})==0)return 86;
  zoomY[2]=1.;zoomX[2]=2.;panel.setPlotTransforms(panX,panY,zoomX,zoomY);
  auto magnitudeHorizontal=panel.createComponentSnapshot(panel.getLocalBounds());
  if(changes(baseline,magnitudeHorizontal,{0,0,800,230})!=0||changes(baseline,magnitudeHorizontal,{0,238,800,262})==0)return 89;
  zoomX[2]=1.;panel.setPlotTransforms(panX,panY,zoomX,zoomY);panel.setPlotLayout({0,0,800,220},{0,238,800,262});
  auto phaseSize=panel.createComponentSnapshot(panel.getLocalBounds());
  if(changes(baseline,phaseSize,{0,0,800,230})==0||changes(baseline,phaseSize,{0,238,800,262})!=0)return 87;
  panel.setPlotLayout({0,0,800,230},{0,238,800,262});panel.setSelectedPlot(1);
  auto phaseSelected=panel.createComponentSnapshot(panel.getLocalBounds());
  panel.setSelectedPlot(2);
  auto magnitudeSelected=panel.createComponentSnapshot(panel.getLocalBounds());
  if(changes(phaseSelected,magnitudeSelected,{0,0,800,230})==0||changes(phaseSelected,magnitudeSelected,{0,238,800,262})==0)return 90;
  panY[1]=2.;zoomX[1]=64.;zoomY[1]=16.;panel.setPlotTransforms(panX,panY,zoomX,zoomY);
  auto extreme=panel.createComponentSnapshot(panel.getLocalBounds());
  int gridLines=0,axisPixels=0;
  for(int y=30;y<200;++y){auto pixel=extreme.getPixelAt(300,y);if(pixel.getRed()==0x2c&&pixel.getGreen()==0x31&&pixel.getBlue()==0x35)++gridLines;}
  for(int y=30;y<200;++y)for(int x=10;x<48;++x){auto pixel=extreme.getPixelAt(x,y);if(pixel.getBlue()>180&&pixel.getGreen()>120&&pixel.getRed()<150)++axisPixels;}
  if(gridLines<3||axisPixels<8||!write(extreme,"tf-grid-extreme-zoom.png"))return 91;
 }
 {
  WorkstationSurface surface;auto& station=surface.station;
  for(auto size:{juce::Point<int>{1440,960},juce::Point<int>{1760,1080},juce::Point<int>{1120,900}}){surface.setSize(size.x,size.y);if(!station.previewRowDSPControls()||!station.previewFFTSelection())return 84;if(!station.previewSurface(surface))return 38;station.previewCursor(2,.6);if((size.y>=960&&!station.previewFits())||!station.previewListAccessible()||!station.previewLeftHeader())return 7;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-"+juce::String(size.x)+"x"+juce::String(size.y)+".png"))return 4;}
  surface.setSize(1440,960);station.previewAvgColour(juce::Colour(0xff00e5ed));if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-AVG-color-personalizado.png"))return 26;station.previewAvgColour(juce::Colour(0xffffac24));if(!station.previewPrincipal()||!station.previewMeterHeightSplitter())return 19;station.previewMeterClock(true);if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-principal-reloj.png"))return 21;station.previewMeterClock(false);station.previewPrincipalConfig();if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-spl-config.png"))return 20;
  const char* pages[]={"display","alarmas","metricas","registro","compensacion"};for(int tab=1;tab<=5;++tab){station.previewPrincipalConfig(tab);if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-spl-"+juce::String(pages[tab-1])+".png"))return 23;}
  for(auto option:std::initializer_list<std::pair<int,const char*>>{{970,"loop"},{517,"pink"},{518,"sweep"}}){if(!station.previewSignalOptions(option.first)){std::cerr<<"Signal options failed: "<<option.first<<"\n";return 17;}if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-options-"+juce::String(option.second)+".png"))return 18;station.previewCloseSignalOptions();}
  surface.setSize(1120,900);if(!station.previewSignalOptions(970))return 78;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-options-loop-minimum.png"))return 79;station.previewCloseSignalOptions();surface.setSize(1440,960);
  if(!station.previewReadoutSources())return 16;
  int meterCount=0,trackCount=0,inspectFailure=0;
  std::function<void(juce::Component&)> inspect=[&](juce::Component& component){
   if(auto* meter=dynamic_cast<TFInputMeters*>(&component)){auto b=CompactLayout::meterBar(meter->getWidth(),0);if(std::abs(meter->thresholdPosition(meter->settings.orangeDbFS)-(b.x+b.w*TFMeterChannel::fraction(meter->settings.orangeDbFS)))>.001||std::abs(meter->thresholdPosition(meter->settings.redDbFS)-(b.x+b.w*TFMeterChannel::fraction(meter->settings.redDbFS)))>.001)inspectFailure=22;++meterCount;for(const auto& channel:meter->channels)if(channel.level||channel.peak)inspectFailure=8;}
   if(auto* button=dynamic_cast<juce::Button*>(&component))if(bool(button->getProperties()["trackCircle"])){++trackCount;if(!button->isEnabled()||button->getToggleState())inspectFailure=9;}
   for(auto* child:component.getChildren())inspect(*child);
  };inspect(station);if(inspectFailure)return inspectFailure;if(meterCount!=4||trackCount!=5)return 10;
  for(auto size:{juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){station.previewFolds(true);surface.setSize(size.x,size.y);if(!station.previewListAccessible())return 13;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-alineacion-abierta-"+juce::String(size.x)+"x"+juce::String(size.y)+".png"))return 6;}
  surface.setSize(1440,960);station.previewFolds(false);if(!station.previewDoubleClickRouting())return 14;station.previewOpenMeterOptions();if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-opciones-doble-clic.png"))return 15;
 }
 {WorkstationSurface surface;auto& demo=surface.station;demo.previewMeters();for(auto size:{juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){surface.setSize(size.x,size.y);demo.previewCursor(2,.6);if(!demo.previewFits())return 11;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-DEMO-"+juce::String(size.x)+"x"+juce::String(size.y)+".png"))return 12;}}
 {WorkstationSurface surface;auto& station=surface.station;if(!station.previewIndependentViews())return 37;if(!station.previewRoutingStatus())return 36;if(!station.previewAcquisitionGuard()||!station.previewPanelToggle())return 30;surface.setSize(1440,960);for(auto grid:{juce::Point<int>{4,1},juce::Point<int>{1,4},juce::Point<int>{2,2},juce::Point<int>{1,1}}){if(!station.previewCompactMeters(grid.x,grid.y))return 31;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-meters-"+juce::String(grid.x)+"rows-"+juce::String(grid.y)+"cols.png"))return 32;if(!write(station.previewFloatingGrid(grid.x,grid.y),"floating-meters-"+juce::String(grid.x)+"rows-"+juce::String(grid.y)+"cols.png"))return 33;station.previewCloseCompactMeters();}}
 {WorkstationSurface surface;auto& station=surface.station;for(int stage=0;stage<3;++stage){if(!station.previewOrigins(stage))return 39;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"trace-origins-"+juce::String(stage)+".png"))return 40;}if(!station.previewAvgSelection())return 41;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"AVG-selected-no-estimator.png"))return 42;}
 // Synthetic rendering of configured backgrounds/alarms; no audio/calibration.
 {juce::Image preview(juce::Image::RGB,768,330,true);juce::Graphics g(preview);g.fillAll(juce::Colour(0xff222427));for(int index=0;index<3;++index){MeterState model(out.getChildFile("paint-fixture-"+juce::Uuid().toString()+".json"));auto card=*model.primaryCard();card.deviceKey="fixture / device";card.channel=1;card.backgroundColour="ff092532";card.warningColour="ff9a5e10";card.alarmColour="ff891f4f";card.warning=-12;card.alarm=-6;card.alarmMetric=1;card.alarmWeight=1;card.animate=false;if(!model.update(card))return 43;model.setSources({{card.deviceKey,"DEMO Input 1",1}});model.publishFs(card.id,card.deviceKey,1,index==0?-20:index==1?-9:-2,false);CompactMeterTile tile(model,card);tile.setSize(216,138);PrincipalMeter principal(model);principal.setSize(216,96);g.setColour(juce::Colours::white);g.setFont(12.f);g.drawText(index==0?"DEMO fondo propio":index==1?"DEMO aviso propio":"DEMO alarma propia",index*256+12,6,232,24,juce::Justification::centred);g.drawImageAt(tile.createComponentSnapshot(tile.getLocalBounds()),index*256+20,36);g.drawImageAt(principal.createComponentSnapshot(principal.getLocalBounds()),index*256+20,206);}if(!write(preview,"meter-relief-custom-colours-DEMO.png"))return 44;}
 {WorkstationSurface surface;auto& station=surface.station;for(bool ir:{false,true,true,false}){if(!station.previewMeasurementMode(ir))return 45;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),ir?"mode-IR-demo-exclusive.png":"mode-TF-three-panels-exclusive.png"))return 46;}}
 {WorkstationSurface surface;auto& station=surface.station;for(int width:{480,720,1200}){if(!write(station.previewFloatingGrid(1,4,width,330),"floating-cells-only-4cols-"+juce::String(width)+".png"))return 47;}for(auto size:{juce::Point<int>{1120,900},juce::Point<int>{1440,960},juce::Point<int>{1760,1080}}){surface.setSize(size.x,size.y);if(!station.previewCompactMeters(4,1))return 48;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"sidebar-cells-only-1col-"+juce::String(size.x)+".png"))return 49;station.previewCloseCompactMeters();}}
 {WorkstationSurface surface;auto& station=surface.station;if(!station.previewNavigationChoice()||!station.previewNavigationOverlay(true))return 50;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"navigation-chevron-open.png"))return 51;if(!station.previewNavigationOverlay(false))return 52;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"navigation-chevron-closed.png"))return 53;}
 {MyaStartupPanel startup(true),review(false);if(!startup.assetValid()||!review.assetValid())return 54;if(!write(startup.createComponentSnapshot(startup.getLocalBounds()),"startup-real-panel.png")||!write(review.createComponentSnapshot(review.getLocalBounds()),"startup-review-panel.png"))return 55;}
 // B calibration UI fixture: known samples and fake inventory only; no audio device is opened.
 {WorkstationSurface surface;if(!surface.station.previewCalibrationDrawer())return 56;if(!write(surface.createComponentSnapshot(surface.getLocalBounds()),"workspace-calibracion-sin-audio.png"))return 57;}
 {auto dir=juce::File(juce::String::fromUTF8(MYA_PROJECT_DIRECTORY)).getChildFile("calibration-ui-"+juce::Uuid().toString());if(!dir.createDirectory())return 58;auto file=dir.getChildFile("cal.json");SplCalibrationStore store(file);RmsCalibration::Session capture;CalibrationInputSnapshot in{true,true,false,48000,-43.01029995664,0,1,0};int plays=0,changes=0;
  AudioEndpoint endpoint{"Core / Card","Card","Core",{"Input 1","Input 2"},{}};
  SplCalibrationPanel panel(store,capture,{endpoint},endpoint.key,1,[&](const juce::String&,int){return in;},[&](juce::String,int){++plays;},[&](const SplCalibrationStore::Record& r){bool ok=store.confirm(r,capture);if(ok)capture.confirmed();return ok;},[&](juce::String,int){++changes;});
  if(!write(panel.createComponentSnapshot(panel.getLocalBounds()),"calibracion-RMS-inicial-DEMO.png"))return 59;
  if(!panel.previewBegin()||plays!=0||file.exists())return 60;
  auto feed=[&](int begin,int end,double amplitude){for(int n=begin;n<=end;++n){std::array<float,480> data;for(size_t i=0;i<data.size();++i)data[i]=(float)(amplitude*std::sin(juce::MathConstants<double>::twoPi*1000*i/48000));InputLevels::Frame f;f.sequence=(unsigned)n;f.timeSeconds=10+n*.01;f.channels[0]=InputLevels::peak(data.data(),480);capture.feed("Core / Card",f,48000);in.cursor=(unsigned)n;}};
  feed(1,500,.01);panel.previewRefresh();if(!panel.previewSaveEnabled()||file.exists())return 61;if(!write(panel.createComponentSnapshot(panel.getLocalBounds()),"calibracion-RMS-lista-DEMO.png"))return 62;
  panel.previewCommit();if(!file.existsAsFile()||!panel.previewAcknowledged()||capture.state!=RmsCalibration::Session::State::Confirmed)return 63;auto saved=file.loadFileAsString();
  ++in.generation;panel.previewRefresh();if(panel.previewChainChecked()||panel.previewAcknowledged())return 64;panel.previewSameChain();if(!panel.previewAcknowledged())return 65;panel.previewMicChange();if(panel.previewAcknowledged()||file.loadFileAsString()!=saved)return 66;
  panel.previewReference(2);if(!panel.previewBegin()||capture.reference!=114)return 67;panel.previewCancel();if(file.loadFileAsString()!=saved||capture.confirmable())return 68;
  for(auto invalid:{"NaN","94 garbage","181",""}){panel.previewManual(invalid);if(panel.previewBegin()||file.loadFileAsString()!=saved)return 73;}
  panel.previewManual("118.25");if(!panel.previewBegin()||capture.reference!=118.25)return 69;feed(501,501,1.);in.clipped=true;panel.previewRefresh();if(panel.previewSaveEnabled()||file.loadFileAsString()!=saved)return 70;if(!write(panel.createComponentSnapshot(panel.getLocalBounds()),"calibracion-RMS-rechazo-clip-DEMO.png"))return 71;
  in.clipped=false;if(!panel.previewBegin())return 74;feed(502,551,.01);feed(552,601,.02);panel.previewRefresh();if(capture.state!=RmsCalibration::Session::State::Measuring||capture.stabilityRetries!=1||!panel.previewTelemetry().contains("Reinicio automatico 1/3"))return 75;
  feed(602,651,.01);feed(652,701,.02);feed(702,751,.01);panel.previewRefresh();if(capture.state!=RmsCalibration::Session::State::Invalid||capture.stabilityRetries!=3||!panel.previewTelemetry().contains("RECHAZADA: diferencia RMS")||!panel.previewTelemetry().contains("6,021 dB")&&!panel.previewTelemetry().contains("6.021 dB"))return 76;
  if(plays||changes==0)return 72;
 }
 // Separate, explicitly synthetic visual fixture. Never delivered to the application's meters.
 juce::Image demo(juce::Image::RGB,640,350,true);juce::Graphics g(demo);g.fillAll(juce::Colour(0xff192126));g.setColour(juce::Colours::white);g.setFont(15.f);g.drawText("DEMO VISUAL AISLADA / M-R de TF / AVG sin medidores",15,8,610,30,juce::Justification::centredLeft);
 const juce::Colour colours[]={juce::Colour(0xff54ee22),juce::Colour(0xff4d88ff),juce::Colour(0xfff03baf),juce::Colour(0xffffe33b),juce::Colour(0xffffac24)};
 for(int i=0;i<4;++i){TFInputMeters meter;meter.setSize(580,48);meter.colour=colours[i];meter.channels[0].update(-2.,0,meter.settings);const double m[]={-18,-15,-10,-8,-16},r[]={-20,-17,-11,-1,-18};meter.channels[0].update(m[i],.2,meter.settings);meter.channels[1].update(r[i],0,meter.settings);auto image=meter.createComponentSnapshot(meter.getLocalBounds());g.drawImageAt(image,40,48+i*68);g.setColour(colours[i]);g.drawText("TF"+juce::String(i+1),6,48+i*68,32,20,juce::Justification::centredLeft);}
 if(!write(demo,"input-meters-DEMO.png"))return 5;
 {TFInputMeters custom;custom.setSize(580,48);custom.settings.orangeDbFS=-18;custom.settings.redDbFS=-3;custom.channels[0].update(-2.,0,custom.settings);custom.channels[0].update(-30.,.1,custom.settings);custom.channels[1].update(-36.,0,custom.settings);if(std::abs(custom.thresholdPosition(-18)-TFMeterChannel::position(-18,17,561))>.001||std::abs(custom.thresholdPosition(-3)-TFMeterChannel::position(-3,17,561))>.001)return 24;juce::Image example(juce::Image::RGB,640,110,true);juce::Graphics painter(example);painter.fillAll(juce::Colour(0xff192126));painter.setColour(juce::Colours::white);painter.setFont(14.f);painter.drawText("DEMO: umbrales personalizados -18 / -3 dBFS",12,8,610,24,juce::Justification::centredLeft);painter.drawImageAt(custom.createComponentSnapshot(custom.getLocalBounds()),30,44);if(!write(example,"input-meters-umbrales-personalizados-DEMO.png"))return 25;}

 {WorkstationSurface surface;if(!surface.station.previewRefreshAndGenerator())return 90;}
 {WorkstationSurface surface;if(!surface.station.previewTraceRemoval())return 85;}
 {WorkstationSurface surface;if(!surface.station.previewIndexAndAverages())return 113;}
 {WorkstationSurface surface;if(!surface.station.previewHiddenTraceKeepsAverage())return 114;}
 {std::vector<bool> accepted;juce::StringArray names{"MIC 1 / TF 1","MIC 2 / TF 2","MIC 3 / TF 3","MIC 4 / TF 4"};AverageTFDialog dialog(names,{true,false,true,false},[&](std::vector<bool> selection){accepted=std::move(selection);});
   if(!write(dialog.createComponentSnapshot(dialog.getLocalBounds()),"create-average-tf.png"))return 89;
   auto button=[&](juce::String name)->juce::Button*{for(auto* child:dialog.getChildren())if(auto* b=dynamic_cast<juce::Button*>(child))if(b->getButtonText()==name)return b;return nullptr;};
   auto* all=button("Select All");auto* clear=button("Clear");auto* ok=button("OK");if(!all||!clear||!ok)return 86;
   clear->onClick();for(auto* child:dialog.getChildren())if(auto* b=dynamic_cast<juce::ToggleButton*>(child))if(b->getToggleState())return 87;
   all->onClick();ok->onClick();if(accepted!=std::vector<bool>({true,true,true,true}))return 88;
 }

 std::cout<<"PASS B: real calibration dialog, 94/114/manual references, capture/confirm/cancel, persistence, explicit chain acknowledgement after generation change, metadata mismatch, clip rejection, source-only Play blocked offscreen; PASS R3: named saved capture origin, TF-only/hidden no-data cursor, AVG row/label/colour selection and colour double click; independent principal/sidebar/floating configurations; 2x2/1x4/4x1/1x1 pages; cell-only full-width grids with contextual pages, chevron navigation overlay without reflow, menu selection closes and T returns TF, phase/magnitude equal, startup/review panels rendered; primary meter visible by default; TFs+AVG remain scroll-accessible; all eight M/R double-clicks open meter options; card/colour keep TF settings; routing unchanged; readout preserves distinct sources; four left tabs equal; anchored Loop/Pink/Sweep toggle and preserve state/graph bounds; offscreen only, no audio\n";return 0;
}
