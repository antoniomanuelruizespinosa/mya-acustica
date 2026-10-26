#include "../src/PlotPresentation.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace PlotPresentation;
int main(){
 std::array<bool,4> visible{{true,true,true,true}};
 // Phase TF1 and magnitude TF3 must resolve independently of selected global TF2.
 assert(frontTrace(2,1,visible)==0);assert(frontTrace(4,1,visible)==2);
 assert(frontTrace(1,1,visible)==1);assert(drawOrder(1).back()==1);
 visible[1]=false;assert(frontTrace(1,1,visible)==3);assert(frontTrace(3,1,visible)==-1);
 // AVG must never be clamped to TF4 and labelled as an average.
 assert(frontTrace(1,4,visible)==3);
 visible.fill(false);assert(frontTrace(1,0,visible)==-1);
 for(int front=0;front<4;++front){auto order=drawOrder(front);auto sorted=order;std::sort(sorted.begin(),sorted.end());assert((sorted==std::array<int,4>{{0,1,2,3}}));assert(order.back()==front);}
 assert(validTF(.5,0,80,80));assert(!validTF(.5,0,79.99,80));
 assert(!validTF(-.01,0,100,0));assert(!validTF(1.01,0,100,0));assert(!validTF(.5,-1,100,0));
 assert(!validTF(.5,0,std::numeric_limits<double>::quiet_NaN(),0));
 double x=1,y=1;zoomAxes(x,y,1.25,false,false);assert(x==1&&y==1.25);
 zoomAxes(x,y,1.25,true,true);assert(x==1.25&&y==1.25);
 zoomAxes(x,y,.8,true,false);assert(x==1&&y==1);
 for(int i=0;i<100;++i)zoomAxes(x,y,1.25,true,false);assert(x==16&&y==16);
 for(double z:{.25,1.,4.,16.})for(double pan:{-1.,0.,.8})for(double u:{0.,.3,1.})
  assert(std::abs(coordinate(screen(u,pan,z),pan,z)-u)<1.e-12);
 // Shared frequency with independent X viewports, identical data coordinate after inversion.
 double u=.6;assert(std::abs(coordinate(screen(u,0,2),0,2)-coordinate(screen(u,.3,4),.3,4))<1.e-12);
 std::cout<<"PASS: independent panel sources, visibility fallback, draw order, validity, Y/X/XY zoom and coordinate inversion\n";
}
