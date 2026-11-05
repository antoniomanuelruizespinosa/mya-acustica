#pragma once
#include <algorithm>
#include <cmath>
namespace CompactLayout {
inline constexpr int libraryTabHeight=24;
inline int libraryColumnWidth(int width){return (width-2)/2;}
inline constexpr int tfRow=102,tfCard=98,tfFooter=4,tfCardsHeight=4*tfRow,avgTop=tfCardsHeight+4,avgHeight=26,tfContentHeight=avgTop+avgHeight+4;
inline constexpr int tfTop=370;
inline int tfStart(bool mainVisible=true){return mainVisible?tfTop:274;}
inline int signalTop(int height){return height-134;}
inline int generatorTop(int height){return height-102;}
inline int actionTop(int height,bool alignmentExpanded=false){return signalTop(height)-(alignmentExpanded?168:64);}
inline int tfBottom(int height,bool alignmentExpanded=false){return actionTop(height,alignmentExpanded)-8;}
inline int tfViewportHeight(int height,bool alignmentExpanded=false,bool mainVisible=true){return std::max(0,tfBottom(height,alignmentExpanded)-tfStart(mainVisible));}
struct Heights {int ir,phase,magnitude;};
inline Heights heights(int available,bool showIR,double irFraction=.14,double phaseFraction=.5){
 const int usable=std::max(0,available-(showIR?16:8));
 int ir=0;
 if(showIR){
  const int minIR=std::min(60,usable/3);
  const int minPhase=std::min(80,(usable-minIR)/2);
  const int minMagnitude=std::min(60,usable-minIR-minPhase);
  ir=std::clamp((int)std::lround(available*irFraction),minIR,usable-minPhase-minMagnitude);
 }
 const int remaining=usable-ir;
 const int minPhase=std::min(80,remaining/2);
 const int minMagnitude=std::min(60,remaining-minPhase);
 const int phase=std::clamp((int)std::floor(remaining*phaseFraction),minPhase,remaining-minMagnitude);
 const int magnitude=phaseFraction==.5?std::min(phase,remaining-phase):remaining-phase;
 return {ir,phase,magnitude};
}
struct Box {int x,y,w,h;bool intersects(const Box& b) const{return x<b.x+b.w&&b.x<x+w&&y<b.y+b.h&&b.y<y+h;}};
inline Box meterBar(int width,int channel){return {17,channel*24+1,std::max(1,width-19),14};}
inline Box meterScale(int width,int channel){return {17,channel*24+15,std::max(1,width-19),9};}
inline Box smoothing(int plotWidth){return {plotWidth-185,2,64,18};}
inline Box plot(int width,int height){return {44,23,std::max(1,width-52),std::max(1,height-48)};}
inline int divisions(int kind){return kind==0?6:12;}
inline double phaseTick(int index,double zoom=1.,double pan=0.){return (180.-index*30.)/zoom+pan*360.;}
}
