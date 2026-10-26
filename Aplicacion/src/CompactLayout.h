#pragma once
#include <algorithm>
#include <cmath>
namespace CompactLayout {
inline constexpr int tfRow=112,tfCard=110,tfFooter=4,tfContentHeight=4*tfRow+tfFooter;
inline constexpr int tfTop=299;
inline int tfBottom(int height){return height-304;}
inline int tfViewportHeight(int height){return std::max(0,tfBottom(height)-tfTop);}
struct Heights {int ir,phase,magnitude;};
inline Heights heights(int available,bool showIR,double irFraction=.14,double phaseFraction=.5){int ir=showIR?std::clamp((int)std::lround(available*irFraction),60,std::max(60,available-176)):0;int rest=available-(showIR?ir+8:0);int phase=std::clamp((int)std::floor((rest-8)*phaseFraction),80,std::max(80,rest-88));return {ir,phase,std::max(60,phaseFraction==.5?phase:rest-phase-8)};}
struct Box {int x,y,w,h;bool intersects(const Box& b) const{return x<b.x+b.w&&b.x<x+w&&y<b.y+b.h&&b.y<y+h;}};
inline Box smoothing(int plotWidth){return {plotWidth-185,2,64,18};}
inline Box plot(int width,int height){return {36,26,std::max(1,width-38),std::max(1,height-48)};}
inline int divisions(int kind){return kind==0?6:12;}
inline double phaseTick(int index,double zoom=1.,double pan=0.){return (180.-index*30.)/zoom+pan*360.;}
}
