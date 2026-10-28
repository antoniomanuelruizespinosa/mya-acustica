#pragma once
#include <algorithm>
#include <cmath>

class GeneratorFilter {
 struct Biquad {double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;float process(float x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return (float)y;}void clear(){z1=z2=0;}};
 Biquad hp,lp;
 static void configure(Biquad& q,double rate,double frequency,bool high){frequency=std::clamp(frequency,5.,rate*.49);double w=2*3.14159265358979323846*frequency/rate,c=std::cos(w),s=std::sin(w),alpha=s/(2*.7071067811865476),a0=1+alpha;if(high){q.b0=(1+c)/(2*a0);q.b1=-(1+c)/a0;q.b2=q.b0;}else{q.b0=(1-c)/(2*a0);q.b1=(1-c)/a0;q.b2=q.b0;}q.a1=-2*c/a0;q.a2=(1-alpha)/a0;q.clear();}
public:
 void configure(double rate,double highPass,double lowPass){configure(hp,rate,highPass,true);configure(lp,rate,std::max(highPass*1.01,lowPass),false);}
 float process(float x){return lp.process(hp.process(x));}
};
