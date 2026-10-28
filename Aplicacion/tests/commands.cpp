#include "../src/Commands.h"
#include <cassert>
#include <iostream>
#include <set>
using namespace MyaCommands;
int main(int argc,char**){
 Context x;std::set<int> ids;std::set<std::string> keys;int dispatched=0,pending=0;
 for(const auto& c:catalog()){
  assert(c.id>0&&ids.insert(c.id).second);assert(!c.key.empty()&&keys.insert(c.key).second);
  assert(!c.name.es.empty()&&!c.name.en.empty());assert(c.menu>=0&&c.menu<7);
  assert(c.path.es.empty()==c.path.en.empty());
  assert(!compactName(c,Language::ES).empty()&&!compactName(c,Language::EN).empty());
  if(c.capability==Capability::Pending){++pending;assert(!c.reason.es.empty()&&!c.reason.en.empty());}
  // Dispatch must reject every pending or context-disabled action, for both languages.
  for(auto lang:{Language::ES,Language::EN})for(int mode=0;mode<4;++mode)for(int tf:{1,5,0}){
   x.language=lang;x.mode=mode;x.selectedTF=tf;int calls=0;
   bool ran=dispatch(c.id,x,[&](int id){assert(id==c.id);++calls;});
   assert(ran==state(c,x).enabled);assert(calls==(ran?1:0));dispatched+=calls;
   if(c.capability==Capability::Pending)assert(!ran);
  }
 }
 for(const auto& m:menus())assert(!m.es.empty()&&!m.en.empty());
 for(int id:topBindings)assert(find(id));for(int id:measurementBindings)assert(find(id));for(int id:tfBindings)assert(find(id));
 for(const auto& p:extraMenus){assert(p[0]>=0&&p[0]<7);assert(find(p[1]));}
 x={};int calls=0;assert(!dispatch(-1,x,[&](int){++calls;}));assert(!dispatch(99999,x,[&](int){++calls;}));assert(calls==0);
 assert(!dispatch(103,x,{}));assert(state(*find(103),x).enabled);
 x.selectedTF=5;assert(!state(*find(103),x).enabled);assert(!state(*find(202),x).enabled);assert(!state(*find(520),x).enabled);
 x={};assert(state(*find(405),x).checked);x.left=false;assert(!state(*find(405),x).checked);
 assert(state(*find(301),x).checked);x.linked=false;assert(!state(*find(301),x).checked);
 x.drawer=false;assert(!state(*find(602),x).enabled);x.drawer=true;assert(state(*find(602),x).enabled);
 x.smoothing={{4,7}};assert(state(*find(313),x).checked);assert(state(*find(326),x).checked);assert(!state(*find(310),x).checked);
 for(auto lang:{Language::ES,Language::EN}){x.language=lang;assert(state(*find(380),x).checked==(lang==Language::ES));assert(state(*find(381),x).checked==(lang==Language::EN));}
 assert(shortcut('k',false,false,false,true,false)==972);assert(shortcut('k',false,true,false,true,false)==971);assert(shortcut('k',false,false,false,true,true)==0);assert(shortcut('k',false,true,false,true,true)==0);
 assert(shortcut('b',false,false,false,true,false)==405);assert(shortcut('o',false,false,false,true,false)==406);
 assert(shortcut('l',false,false,false,true,false)==502&&shortcut('f',false,false,false,true,false)==502);
 assert(shortcut('g',false,false,false,true,false)==504&&shortcut('s',false,false,false,true,false)==505);
 assert(shortcut(' ',false,false,false,true,false)==103);assert(shortcut('5',false,true,false,true,false)==409);
 assert(shortcut('+',false,false,true,true,false)==850);assert(shortcut('-',true,false,false,true,false)==853);assert(shortcut('+',true,true,true,true,false)==854);
 assert(shortcut('+',false,false,false,false,false)==0);
 for(char c:std::string("bo cflgstdev+-5"))assert(shortcut(c,false,false,false,true,true)==0);
 assert(shortcut('g',true,false,false,true,false)==0);assert(shortcut('e',false,true,false,true,false)==0);
 for(int view:{0,1}){x={};x.measurementView=view;assert(state(*find(980),x).checked==(view==0));assert(state(*find(981),x).checked==(view==1));assert(!state(*find(420),x).enabled&&!state(*find(420),x).checked);assert(state(*find(407),x).enabled==(view==0));}
 assert(shortcut('v',false,false,false,true,false)==526);assert(find(526)->capability==Capability::Pending);x={};assert(!dispatch(526,x,[&](int){assert(false);}));assert(find(403)->shortcut.empty()&&find(402)->shortcut.empty());assert(shortcut('d',false,false,false,true,false)==0);assert(shortcut('t',false,false,false,true,false)==980);x.mode=2;assert(state(*find(980),x).enabled);
 for(char key:std::string("boc lfgsvt")){assert(shortcut(key,false,false,true,true,false)==0);assert(shortcut(key,false,false,false,true,true)==0);}assert(shortcut('k',false,true,true,true,false)==0&&shortcut('t',false,true,true,true,false)==0&&shortcut('5',false,true,true,true,false)==0);assert(shortcut('+',true,true,true,true,false)==854);
 for(int mask=0;mask<16;++mask){bool command=(mask&1)!=0,control=(mask&2)!=0,alt=(mask&4)!=0,shift=(mask&8)!=0;assert(isPlainMeterKey('e',command,control,alt,shift)==(mask==0));assert(isPlainMeterKey('E',command,control,alt,shift)==(mask==0));assert(!isPlainMeterKey('x',command,control,alt,shift));}
 MeterSequence sequence;assert(sequence.press(0,true)==0);assert(sequence.press(500,true)==203);
 assert(sequence.press(1000,true)==0);assert(sequence.press(1700,true)==0);sequence.reset();assert(sequence.press(1800,true)==0);
 assert(sequence.press(1801,false)==0);assert(sequence.press(1802,true)==0);assert(sequence.press(1900,true)==203);
 x={};x.libraryTF=false;assert(!state(*find(103),x).enabled);x.libraryTF=true;x.libraryReady=false;assert(!state(*find(900),x).enabled);assert(state(*find(911),x).enabled);
 x={};x.librarySelection=2;x.libraryCaptures=2;assert(state(*find(904),x).enabled&&state(*find(908),x).enabled);assert(!state(*find(902),x).enabled&&!state(*find(903),x).enabled);assert(!state(*find(909),x).enabled&&!state(*find(910),x).enabled);
 x.graphTypes={{2,1}};x.irScale=2;assert(state(*find(940),x).checked&&state(*find(951),x).checked&&state(*find(932),x).checked);
 std::cout<<"PASS: "<<ids.size()<<" unique IDs/keys, ES/EN, "<<pending<<" pending commands, "<<dispatched<<" guarded dispatches, bindings, context/toggles and shortcuts\n";
 if(argc>1){x={};std::cout<<"MENU\tID\tKEY\tACTION\tES\tEN\tSTATE (default)\tREASON ES / EN\n";
  auto row=[&](int menu,const Command& c){auto s=state(c,x);std::cout<<menus()[(size_t)menu].es<<"/"<<c.path.es<<"\t"<<c.id<<"\t"<<c.key<<"\t"<<(c.capability==Capability::Pending?"blocked":c.key)<<"\t"<<c.name.es<<"\t"<<c.name.en<<"\t"<<(c.capability==Capability::Pending?"pending":c.capability==Capability::Demo?"demo":"ready")<<(s.enabled?" enabled":" disabled")<<"\t"<<s.reason.es<<" / "<<s.reason.en<<"\n";};
  for(const auto& c:catalog())row(c.menu,c);for(const auto& p:extraMenus)row(p[0],*find(p[1]));
 }
}
