#include "../src/ImpulseDisplay.h"
#include <cassert>
#include <iostream>
int main(){
 constexpr int n=1024;double pi=std::acos(-1.);std::vector<double> signal,expected;
 for(int i=0;i<n;++i){double t=2*pi*i/n;double envelope=1.+.25*std::cos(3*t);expected.push_back(envelope);signal.push_back(envelope*std::cos(32*t));}
 auto envelope=ImpulseDisplay::envelope(signal);assert(envelope.size()==n);
 for(int i=0;i<n;++i)assert(std::abs(envelope[(size_t)i]-expected[(size_t)i])<1.e-11);
 assert(std::abs(signal[8])<1.e-12&&envelope[8]>.9); // envelope is not abs(IR)
 assert(ImpulseDisplay::timeMs(0)==-85&&ImpulseDisplay::timeMs(.5)==0&&ImpulseDisplay::timeMs(1)==85);
 ImpulseDisplay display;
 for(int i=0;i<=1000;++i)for(int mode=0;mode<3;++mode){assert(!display.physical(i/1000.,mode));assert(!display.normalized(i/1000.,mode));}
 assert(!display.physical(-.1,0));assert(!display.physical(1.1,2));assert(ImpulseDisplay::envelope({1.,2.,3.}).empty());
 std::cout<<"PASS: analytic envelope, empty impulse at startup, range -85..85 ms\n";
}
