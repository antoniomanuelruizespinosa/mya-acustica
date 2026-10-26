#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Presentation policy only: no acquisition, averaging, routing or captured data.
namespace PlotPresentation {
inline constexpr std::array<double,10> frequencyTicks{{31.5,63.,125.,250.,500.,1000.,2000.,4000.,8000.,16000.}};
inline int frontTrace(int choice, int selected, const std::array<bool,4>& visible) {
 if(choice>1){int i=choice-2;return i<4&&visible[(size_t)i]?i:-1;}
 if(selected>=0&&selected<4&&visible[(size_t)selected])return selected;
 for(int i=3;i>=0;--i)if(visible[(size_t)i])return i;
 return -1;
}
inline std::array<int,4> drawOrder(int front) {
 std::array<int,4> order{{0,1,2,3}};
 if(front>=0&&front<4){for(int i=front;i<3;++i)order[(size_t)i]=i+1;order[3]=front;}
 return order;
}
inline bool validTF(double u,int index,double coherence,double threshold) {
 return index>=0&&index<4&&std::isfinite(u)&&u>=0.&&u<=1.&&
   std::isfinite(coherence)&&coherence>=threshold;
}
inline double coordinate(double screen,double pan,double zoom){return (screen-.5)/zoom+.5+pan;}
inline double screen(double coordinate,double pan,double zoom){return (coordinate-pan-.5)*zoom+.5;}
// Arrows change horizontal span, never translate samples or delays.
inline void linkFrequency(std::array<double,3>& zoom,std::array<double,3>& pan,int active){if(active!=1&&active!=2)return;size_t from=(size_t)active,to=active==1?2:1;zoom[to]=zoom[from];pan[to]=pan[from];}
inline void horizontalZoom(double& zoom,double& pan,bool widen,bool impulse){
 zoom=std::clamp(zoom*(widen?.8:1.25),impulse?.25:1.,64.);
 if(impulse){pan=0.;return;}double half=.5/zoom;double centre=std::clamp(.5+pan,half,1.-half);pan=centre-.5;
}
inline void zoomAxes(double& x,double& y,double factor,bool command,bool alt){
 if(command)x=std::clamp(x*factor,.25,16.);
 if(!alt)y=std::clamp(y*factor,.25,16.);
}
}
