#include "../src/WorkspaceViewState.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
 assert(argc==2);TFMeterSettings s;assert(s.valid());TFMeterChannel ch;
 assert(!ch.level&&!ch.peak&&!ch.clipped());ch.update(-30.,0,s);assert(*ch.level==-30&&*ch.peak==-30);
 assert(TFMeterChannel::fraction(*ch.level)<TFMeterChannel::fraction(s.orangeDbFS));
 ch.update(-6.,.1,s);assert(*ch.peak==-6&&!ch.clipped());
 assert(TFMeterChannel::fraction(*ch.level)>TFMeterChannel::fraction(s.orangeDbFS)&&TFMeterChannel::fraction(*ch.level)<TFMeterChannel::fraction(s.redDbFS));
 ch.update(-18.,1.5,s);assert(*ch.peak==-6);ch.update(-18.,2.1,s);assert(*ch.peak==-18);
 ch.update(0.,2.2,s);assert(ch.clipped());ch.update(std::nullopt,2.3,s);assert(!ch.level&&!ch.peak&&!ch.clipped());
 ch.update(std::numeric_limits<double>::quiet_NaN(),3,s);assert(!ch.level);
 s.peakOn=false;ch.update(-4.,4,s);assert(ch.level&&!ch.peak);s.peakOn=true;s.holdSeconds=.5;ch.update(-2.,5,s);ch.update(-12.,5.49,s);assert(*ch.peak==-2);ch.update(-12.,5.5,s);assert(*ch.peak==-12);
 assert(TFMeterChannel::fraction(-80)==0&&TFMeterChannel::fraction(3)==1);s.orangeDbFS=105;s.redDbFS=110;assert(!s.valid());s={};s.orangeDbFS=s.redDbFS;assert(!s.valid());
 WorkspaceViewState view;view.leftWidth=220;view.rightWidth=280;view.leftVisible=false;view.meters.holdSeconds=5;view.meters.gloss=false;
 juce::File directory(juce::String::fromUTF8(argv[1]));assert(directory.createDirectory());auto file=directory.getChildFile("view.json");assert(view.save(file));auto read=WorkspaceViewState::read(file);assert(read&&read->leftWidth==220&&read->rightWidth==280&&!read->leftVisible&&read->meters.holdSeconds==5&&!read->meters.gloss);
 auto invalid=view.encode();invalid.getDynamicObject()->setProperty("leftWidth","bad");assert(!WorkspaceViewState::decode(invalid));invalid=view.encode();invalid.getDynamicObject()->setProperty("rightWidth",1000);read=WorkspaceViewState::decode(invalid);assert(read&&read->rightWidth==WorkspaceViewState::rightMax);
 assert(!TFMeterSettings::decode(juce::var()));invalid=TFMeterSettings{}.encode();invalid.getDynamicObject()->setProperty("redDbFS",110);assert(!TFMeterSettings::decode(invalid));
 using namespace std;cout<<"PASS: no audio stays empty; dBFS zones, clip, held peak expiry/off/reset, invalid samples and persistent bounded workspace/meter settings\n";
}
