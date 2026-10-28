#include "../src/WeightedMeterCore.h"
#include <cassert>
#include <iostream>
#include <vector>

static double tone(double rate,int hz,int weight){WeightedMeter::Channel channel;channel.configure(rate);int size=(int)std::round(rate/100.);std::vector<float> block((size_t)size);double power=0;for(int n=0;n<500;++n){for(int i=0;i<size;++i)block[(size_t)i]=(float)(.1*std::sin(2*3.14159265358979323846*hz*(n*size+i)/rate));InputLevels::Peak p=InputLevels::peak(block.data(),size);channel.process(block.data(),size,p);assert(p.weightedValid);if(n>=400)power+=p.weightedSquares[(size_t)weight];}return 10*std::log10(power/(100*size));}
int main(){
 for(double rate:{44100.,48000.,96000.}){double z1=tone(rate,1000,0),a1=tone(rate,1000,1),c1=tone(rate,1000,2);assert(std::abs(a1-z1)<.08&&std::abs(c1-z1)<.08);}
 double z100=tone(48000,100,0),a100=tone(48000,100,1),c100=tone(48000,100,2);assert(a100-z100< -17&&a100-z100> -22);assert(c100-z100<0&&c100-z100> -2);
 WeightedMeter::Channel dynamics;dynamics.configure(48000);std::vector<float> step(48000,.5f);auto p=InputLevels::peak(step.data(),(int)step.size());dynamics.process(step.data(),(int)step.size(),p);assert(p.weightedValid&&p.clipped==false&&std::abs(p.weightedPeak[0]-.5)<1.e-12);assert(std::abs(p.fastPower[0]-.25*(1-std::exp(-8.)))<1.e-6);assert(std::abs(p.slowPower[0]-.25*(1-std::exp(-1.)))<1.e-6);
 dynamics.configure(96000);std::vector<float> silence(9600);p=InputLevels::peak(silence.data(),(int)silence.size());dynamics.process(silence.data(),(int)silence.size(),p);assert(p.weightedValid&&p.fastPower[0]==0&&p.slowPower[0]==0);auto silentDb=WeightedMeter::db(0);assert(silentDb&&std::isinf(*silentDb)&&*silentDb<0);
 float clipped[]={1.f,-1.f};p=InputLevels::peak(clipped,2);dynamics.process(clipped,2,p);assert(p.weightedValid&&p.clipped&&p.weightedPeak[0]==1.);
 auto unsupported=WeightedMeter::Channel{};unsupported.configure(8000);float x[8]{};p=InputLevels::peak(x,8);unsupported.process(x,8,p);assert(!p.weightedValid);
 std::cout<<"PASS weighted 1 kHz normalization at 44.1/48/96 kHz; 100 Hz A/C response; Fast/Slow constants; peak, clip, silence, rate reset and unsupported rate\n";
}
