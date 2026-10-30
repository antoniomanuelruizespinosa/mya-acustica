#include "../src/CompactLayout.h"
#include "../src/CommandText.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace CompactLayout;
 for(int height:{900,960,1080,1440,2000}){
  int viewport=tfViewportHeight(height);
  assert(viewport>=tfCard);if(height>=960)assert(viewport>=tfContentHeight); // Smaller windows retain independent scroll access.
  int scrollMaximum=std::max(0,tfContentHeight-viewport);
  assert(3*tfRow+tfCard-scrollMaximum<=viewport);
  Box viewportBox{0,tfTop,264,viewport};Box controls{0,actionTop(height),264,27};
  assert(!viewportBox.intersects(controls));
  for(int row=0;row<4;++row){int top=row*tfRow;assert(top+tfCard<=tfContentHeight);assert(top+24+48<=top+76);assert(top+76+18<=top+tfCard);}
 }
 for(int h:{960,1080})for(bool expanded:{false,true}){assert(tfViewportHeight(h,expanded)>=tfCard);if(h>=1080||!expanded)assert(tfViewportHeight(h,expanded)>=tfContentHeight);Box list{0,tfTop,216,tfViewportHeight(h,expanded)},actions{0,actionTop(h,expanded),216,expanded?164:58},generator{0,signalTop(h),216,28};assert(!list.intersects(actions)&&!actions.intersects(generator));Box controls{0,generatorTop(h),216,66};assert(!controls.intersects(generator)&&controls.y+controls.h<=h-33);}
 for(int width:{184,192,316})for(int ch=0;ch<2;++ch){auto bar=meterBar(width,ch),scale=meterScale(width,ch);assert(bar.w==scale.w&&bar.x==scale.x);assert(!bar.intersects(scale));assert(bar.y+bar.h<=48&&scale.y+scale.h<=48);assert(bar.x+bar.w<=width);}
 for(int width:{680,1000,1600})for(int height:{80,160,400}){
  auto control=smoothing(width);Box ticks{0,height-27,width,16},source{width-115,2,110,18},type{5,2,140,18},coherence{175,2,95,18};
  assert(control.x>=0&&control.x+control.w<=width);
  assert(!control.intersects(ticks)&&!control.intersects(source)&&!control.intersects(type)&&!control.intersects(coherence));
 }
 for(int w:{680,1000})for(int h:{80,200,400}){auto p=plot(w,h);assert(p.x>=0&&p.y>=20&&p.x+p.w<=w&&p.y+p.h<=h);assert(p.w>w*.9&&p.h>0);Box label{0,h-19,w-24,14},gear{w-20,h-18,16,16};assert(!label.intersects(gear));}
 for(int i=0;i<=12;++i)assert(phaseTick(i)==180-i*30);
 for(int available=717;available<=1817;++available)for(bool ir:{false,true}){auto h=heights(available,ir);assert(h.phase==h.magnitude);assert(h.phase>=80);assert(h.ir+(ir?8:0)+h.phase+8+h.magnitude<=available);auto grid=plot(800,h.phase);assert(grid.x+grid.w==798);}
 using namespace MyaCommands;Context context;auto* audio=find(201);assert(audio);
 auto es=commandText(*audio,state(*audio,context),Language::ES);
 auto en=commandText(*audio,state(*audio,context),Language::EN);
 assert(audio->capability==Capability::Ready);assert(es.startsWith("Audio / E-S")&&es.contains("Play")&&es.contains("Stop/Pause")&&!es.contains("desactivada")&&!es.contains("demo"));
 assert(en.startsWith("Audio / I-O")&&en.contains("Play")&&en.contains("Stop/Pause")&&!en.contains("acquisition disabled")&&!en.contains("demo"));

 std::cout<<"PASS: four compact TFs plus AVG and bottom controls fit without scrolling; numeric readout shares full capsule centre; scale and delay do not overlap; smoothing avoids axis labels and header controls; ES/EN tooltip separators and accents decode as UTF-8\n";
}
