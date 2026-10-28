#include "../src/WorkspaceViewState.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
 assert(argc==2);TFMeterSettings s;assert(s.valid()&&s.orangeDbFS==-12&&s.redDbFS==-6);TFMeterChannel ch;
 assert(TFMeterChannel::position(-12,17,200)==177&&TFMeterChannel::position(-6,17,200)==197);assert(TFMeterChannel::position(-18,10,100)==80&&TFMeterChannel::position(-3,10,100)==105);
 assert(!ch.level&&!ch.peak&&!ch.clipped());ch.update(-30.,0,s);assert(*ch.level==-30&&*ch.peak==-30);
 assert(TFMeterChannel::fraction(*ch.level)<TFMeterChannel::fraction(s.orangeDbFS));
 ch.update(-9.,.1,s);assert(*ch.peak==-9&&!ch.clipped());
 assert(TFMeterChannel::fraction(*ch.level)>TFMeterChannel::fraction(s.orangeDbFS)&&TFMeterChannel::fraction(*ch.level)<TFMeterChannel::fraction(s.redDbFS));
 ch.update(-18.,1.5,s);assert(*ch.peak==-9);ch.update(-18.,2.1,s);assert(*ch.peak==-18);
 ch.update(-12.,2.11,s);assert(*ch.level==s.orangeDbFS&&!ch.clipped());ch.update(-6.,2.12,s);assert(*ch.level==s.redDbFS&&!ch.clipped());ch.update(-5.9,2.13,s);assert(*ch.level>s.redDbFS&&!ch.clipped());ch.update(-.1,2.14,s);assert(!ch.clipped());ch.update(0.,2.2,s);assert(ch.clipped());ch.update(std::nullopt,2.3,s);assert(!ch.level&&!ch.peak&&!ch.clipped());
 ch.update(std::numeric_limits<double>::quiet_NaN(),3,s);assert(!ch.level);
 s.peakOn=false;ch.update(-4.,4,s);assert(ch.level&&!ch.peak);s.peakOn=true;s.holdSeconds=.5;ch.update(-2.,5,s);ch.update(-12.,5.49,s);assert(*ch.peak==-2);ch.update(-12.,5.5,s);assert(*ch.peak==-12);
 assert(TFMeterChannel::fraction(-80)==0&&TFMeterChannel::fraction(3)==1);s.orangeDbFS=105;s.redDbFS=110;assert(!s.valid());s={};s.orangeDbFS=s.redDbFS;assert(!s.valid());
 WorkspaceViewState view;view.leftWidth=220;view.rightWidth=280;view.leftVisible=false;view.meters.holdSeconds=5;view.meters.gloss=false;view.meters.redDbFS=-3.;view.avgColour="ff123456";
 juce::File directory(juce::String::fromUTF8(argv[1]));assert(directory.createDirectory());auto file=directory.getChildFile("view.json");assert(view.save(file));auto read=WorkspaceViewState::read(file);assert(read&&read->leftWidth==220&&read->rightWidth==280&&!read->leftVisible&&read->meters.holdSeconds==5&&!read->meters.gloss&&read->avgColour=="ff123456"&&read->meters.redDbFS==-3.);
 auto legacy=view.encode();legacy.getDynamicObject()->removeProperty("avgColour");auto legacyRead=WorkspaceViewState::decode(legacy);assert(legacyRead&&legacyRead->avgColour=="ffffac24"&&legacyRead->meters.redDbFS==-3.);
 auto invalid=view.encode();invalid.getDynamicObject()->setProperty("leftWidth","bad");assert(!WorkspaceViewState::decode(invalid));invalid=view.encode();invalid.getDynamicObject()->setProperty("rightWidth",1000);read=WorkspaceViewState::decode(invalid);assert(read&&read->rightWidth==WorkspaceViewState::rightMax);
 assert(!TFMeterSettings::decode(juce::var()));invalid=TFMeterSettings{}.encode();invalid.getDynamicObject()->setProperty("redDbFS",110);assert(!TFMeterSettings::decode(invalid));
 using namespace std;cout<<"PASS: no audio stays empty; new -12/-6 defaults and preserved saved -3; threshold boundaries separate from clip, held peak expiry/off/reset, invalid samples and persistent bounded workspace/meter settings\n";
}
