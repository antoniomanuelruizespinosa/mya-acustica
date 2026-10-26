// Paint actual components into images, without creating a native window or starting audio.
#define MYA_OFFSCREEN_RENDER 1
#include "../src/Main.cpp"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 2;juce::ScopedJuceInitialiser_GUI init;
 juce::File out(juce::String::fromUTF8(argv[1]));if(!out.createDirectory())return 3;
 auto write=[&](const juce::Image& image,juce::String name){juce::FileOutputStream stream(out.getChildFile(name));if(!stream.openedOk()||!stream.setPosition(0)||!stream.truncate())return false;return juce::PNGImageFormat{}.writeImageToStream(image,stream);};
 {
  Workstation station;station.previewCursor(2,.6);
  for(auto size:{juce::Point<int>{1440,960},juce::Point<int>{1120,900},juce::Point<int>{1760,1080}}){station.setSize(size.x,size.y);station.previewCursor(2,.6);if(!write(station.createComponentSnapshot(station.getLocalBounds()),"workspace-"+juce::String(size.x)+"x"+juce::String(size.y)+".png"))return 4;}
  station.setSize(1440,960);
  std::function<void(juce::Component&)> inspect=[&](juce::Component& component){
   if(auto* meter=dynamic_cast<TFInputMeters*>(&component))for(const auto& channel:meter->channels)jassert(!channel.level&&!channel.peak);
   if(auto* button=dynamic_cast<juce::Button*>(&component))if(bool(button->getProperties()["trackCircle"]))jassert(!button->isEnabled()&&!button->getToggleState());
   if(auto* viewport=dynamic_cast<juce::Viewport*>(&component))if(dynamic_cast<PaintSurface*>(viewport->getViewedComponent()))viewport->setViewPosition(0,viewport->getViewedComponent()->getHeight());
   for(auto* child:component.getChildren())inspect(*child);
  };inspect(station);
  if(!write(station.createComponentSnapshot(station.getLocalBounds()),"workspace-TF4-scroll.png"))return 6;
 }
 // Separate, explicitly synthetic visual fixture. Never delivered to the application's meters.
 juce::Image demo(juce::Image::RGB,640,350,true);juce::Graphics g(demo);g.fillAll(juce::Colour(0xff192126));g.setColour(juce::Colours::white);g.setFont(15.f);g.drawText("DEMO VISUAL dBFS / datos sinteticos / sin audio",15,8,610,30,juce::Justification::centredLeft);
 const juce::Colour colours[]={juce::Colour(0xff54ee22),juce::Colour(0xff4d88ff),juce::Colour(0xfff03baf),juce::Colour(0xffffe33b)};
 for(int i=0;i<4;++i){TFInputMeters meter;meter.setSize(580,56);meter.colour=colours[i];meter.channels[0].update(-2.,0,meter.settings);meter.channels[0].update(-30.+i*9,.2,meter.settings);meter.channels[1].update(i==3?-1.:-18.+i*5,0,meter.settings);auto image=meter.createComponentSnapshot(meter.getLocalBounds());g.drawImageAt(image,40,48+i*74);g.setColour(colours[i]);g.drawText("TF"+juce::String(i+1),6,48+i*74,32,20,juce::Justification::centredLeft);}
 if(!write(demo,"input-meters-DEMO.png"))return 5;
 std::cout<<"PASS: offscreen component renders only; no native window or audio acquisition\n";return 0;
}
