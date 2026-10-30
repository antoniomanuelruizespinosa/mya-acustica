#pragma once
#include <algorithm>
#include <cmath>
namespace CompactLayout {
inline constexpr int libraryTabHeight=24;
inline int libraryColumnWidth(int width){return (width-2)/2;}
inline constexpr int tfRow=100,tfCard=96,tfFooter=4,tfCardsHeight=4*tfRow,avgTop=tfCardsHeight+4,avgHeight=26,tfContentHeight=avgTop+avgHeight+4;
inline constexpr int tfTop=312;
inline int tfStart(bool mainVisible=true){return mainVisible?tfTop:216;}
inline int signalTop(int height){return height-134;}
inline int generatorTop(int height){return height-102;}
inline int actionTop(int height,bool alignmentExpanded=false){return signalTop(height)-(alignmentExpanded?168:64);}
inline int tfBottom(int height,bool alignmentExpanded=false){return actionTop(height,alignmentExpanded)-8;}
inline int tfViewportHeight(int height,bool alignmentExpanded=false,bool mainVisible=true){return std::max(0,tfBottom(height,alignmentExpanded)-tfStart(mainVisible));}
struct Heights {int ir,phase,magnitude;};
inline Heights heights(int available,bool showIR,double irFraction=.14,double phaseFraction=.5){int ir=showIR?std::clamp((int)std::lround(available*irFraction),60,std::max(60,available-176)):0;int rest=available-(showIR?ir+8:0);int phase=std::clamp((int)std::floor((rest-8)*phaseFraction),80,std::max(80,rest-88));return {ir,phase,std::max(60,phaseFraction==.5?phase:rest-phase-8)};}
struct Box {int x,y,w,h;bool intersects(const Box& b) const{return x<b.x+b.w&&b.x<x+w&&y<b.y+b.h&&b.y<y+h;}};
inline Box meterBar(int width,int channel){return {17,channel*24+1,std::max(1,width-19),14};}
inline Box meterScale(int width,int channel){return {17,channel*24+15,std::max(1,width-19),9};}
inline Box smoothing(int plotWidth){return {plotWidth-185,2,64,18};}
inline Box plot(int width,int height){return {36,26,std::max(1,width-38),std::max(1,height-48)};}
inline int divisions(int kind){return kind==0?6:12;}
inline double phaseTick(int index,double zoom=1.,double pan=0.){return (180.-index*30.)/zoom+pan*360.;}
}
