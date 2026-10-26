#include "../src/ImpulseDisplay.h"
#include <cassert>
#include <iostream>
int main(){
 constexpr int n=1024;double pi=std::acos(-1.);std::vector<double> signal,expected;
 for(int i=0;i<n;++i){double t=2*pi*i/n;double envelope=1.+.25*std::cos(3*t);expected.push_back(envelope);signal.push_back(envelope*std::cos(32*t));}
 auto envelope=ImpulseDisplay::envelope(signal);assert(envelope.size()==n);
 for(int i=0;i<n;++i)assert(std::abs(envelope[(size_t)i]-expected[(size_t)i])<1.e-11);
 assert(std::abs(signal[8])<1.e-12&&envelope[8]>.9); // envelope is not abs(IR)
 assert(ImpulseDisplay::timeMs(0)==-40&&ImpulseDisplay::timeMs(.5)==0&&ImpulseDisplay::timeMs(1)==40);
 ImpulseDisplay demo;bool negative=false;
 for(int i=0;i<=1000;++i){double u=i/1000.;auto lin=demo.physical(u,0),log=demo.physical(u,1),etc=demo.physical(u,2);assert(lin&&log&&etc);negative|=*lin<0;assert(std::isfinite(*log)&&*log>=-120&&std::isfinite(*etc)&&*etc>=-120);assert(std::abs(*demo.normalized(u,0)*2-*lin)<1.e-12);}
 assert(negative);assert(!demo.physical(-.1,0));assert(!demo.physical(1.1,2));assert(ImpulseDisplay::envelope({1.,2.,3.}).empty());
 std::cout<<"PASS: analytic envelope against amplitude-modulated periodic reference, signed Lin, finite Log/ETC, range -40..40 ms and amplitude-coordinate conversion\n";
}
