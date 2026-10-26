#include "../src/FractionalSmoothing.h"
#include <cassert>
#include <iostream>
#include <limits>
int main(){
 FractionalSmoothing s;
 s.rebuild([](double){return std::polar(2.,.7);});
 for(double n:{48.,24.,12.,6.,3.,1.})for(double f:{20.,1000.,20000.}){
  assert(std::abs(std::abs(s.at(f,n,false))-2.)<1.e-9);
  assert(std::abs(std::arg(s.at(f,n,true))-.7)<1.e-9);
 }
 s.rebuild([](double f){return std::polar(1.,(f<1000?179.:-179.)*std::acos(-1.)/180.);});
 assert(std::abs(std::arg(s.at(1000,3,true)))>3.1);
 // Geometric magnitude mean differs from the erroneous arithmetic amplitude mean.
 s.rebuild([](double f){return std::complex<double>(std::pow(10.,f/20000.),0.);});
 double ratio=std::pow(2.,.5/3.);double meanHz=(1000./ratio+1000.*ratio)*.5;
 assert(std::abs(20*std::log10(std::abs(s.at(1000,3,false)))-meanHz/1000.)<1.e-8);
 // Narrow peak: compare integration with an independent dense numerical reference.
 auto peak=[](double f){double db=12.*std::exp(-std::pow((f-1000.)/15.,2));return std::complex<double>(std::pow(10.,db/20.),0.);};
 s.rebuild(peak);
 for(double n:{48.,12.,3.,1.}){
  double ratio=std::pow(2.,.5/n),a=1000./ratio,b=1000.*ratio,mean=0.;
  constexpr int points=200000;
  for(int i=0;i<points;++i)mean+=20*std::log10(std::abs(peak(a+(b-a)*(i+.5)/points)))/points;
  std::cout<<"peak 1/"<<n<<" error dB: "<<std::abs(20*std::log10(std::abs(s.at(1000,n,false)))-mean)<<std::endl;
  // Error budget: half the 0.01 dB cursor display increment.
  assert(std::abs(20*std::log10(std::abs(s.at(1000,n,false)))-mean)<.005);
 }
 // Gaps do not poison distant bands or produce fabricated 0 dB samples.
 s.rebuild([](double f){return f>990&&f<1010?std::complex<double>(std::numeric_limits<double>::quiet_NaN(),0.):std::complex<double>(2.,0.);});
 assert(!std::isfinite(s.at(1000,3,false).real()));assert(!std::isfinite(s.at(1000,3,true).real()));
 assert(std::abs(std::abs(s.at(5000,3,false))-2.)<1.e-9);
 for(double f:{19.,20001.,std::numeric_limits<double>::quiet_NaN()})assert(!std::isfinite(s.at(f,3,false).real()));
 assert(!std::isfinite(s.at(1000,0,false).real()));
 s.rebuild([](double){return std::complex<double>{};});assert(!std::isfinite(s.at(1000,3,false).real()));
 s.rebuild([](double){return std::complex<double>(std::numeric_limits<double>::infinity(),0.);});assert(!std::isfinite(s.at(1000,3,true).real()));
 std::cout<<"PASS: constant gain/phase, band edges, wrapped phase, dB averaging, narrow peak reference, gaps, NaN/Inf/zero and out-of-range\n";
}
