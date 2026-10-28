#define MYA_OFFSCREEN_RENDER 1
#include "../src/Main.cpp"
#include <iostream>
int main(int argc,char** argv){if(argc!=2&&argc!=3)return 2;juce::ScopedJuceInitialiser_GUI init;File dir=File(String::fromUTF8(argv[1])).getChildFile(Uuid().toString());if(!dir.createDirectory())return 3;int failures=0;auto check=[&](bool ok,const char* name){std::cout<<(ok?"PASS ":"FAIL ")<<name<<"\n";if(!ok)++failures;};auto file=dir.getChildFile("proyecto-demo.json");
 {Workstation original(file);original.fixtureCleanSave();check(original.fixtureIsClean(),"saved clean state including retained demo capture");}
 auto saved=file.loadFileAsString();
 InputRoutePreferences latest;for(int i=0;i<4;++i)latest.routes[(size_t)i]={"Core / Newer machine route",i+1,8};auto routeFile=dir.getChildFile("entradas-niveles.json");check(latest.save(routeFile),"seed newer per-machine routes after project save");auto latestBytes=routeFile.loadFileAsString();
 {Workstation reopened(file);check(reopened.fixtureIsClean(),"automatic reopen restores saved clean session");check(reopened.fixtureRoutes()==JSON::toString(latest.encode()),"startup prefers newer machine routes over old project snapshot");check(routeFile.loadFileAsString()==latestBytes,"startup does not rewrite routing preferences");reopened.fixtureLoad();check(reopened.fixtureIsClean(),"explicit load restores clean session");}
 check(file.loadFileAsString()==saved,"reopen does not rewrite project");
 String key="CoreAudio / Fixture card";std::array<String,4> names{{"TF1","TF2","TF3","TF4"}},keys{{key,key,key,key}};std::array<int,4> m{{1,2,3,4}},r{{5,5,5,5}};String receivedKey;int receivedM=-1,receivedR=-1;
 auto change=[&](int,String,String k,int mic,int ref){receivedKey=k;receivedM=mic;receivedR=ref;};
 {RoutingPanel absent({},names,keys,m,r,{},0,[](int){return Colours::green;},change,[](String,int){});absent.fixtureRename(0);check(receivedKey==key&&receivedM==1&&receivedR==5,"offline E/S name edit preserves device and M/R");}
 {EnginePanel absent("TF1",0,1,5,Colours::green,"Fixture absent",{},[&](String,double,int mic,int ref,Colour){receivedM=mic;receivedR=ref;});absent.fixtureRename();check(receivedM==1&&receivedR==5,"offline TF name edit preserves M/R");}
 std::vector<AudioEndpoint> connected{{key,"Fixture card","CoreAudio",{"Input 1","Input 2","Input 3","Input 4","Input 5"},{}}};
 {RoutingPanel present(connected,names,keys,m,r,{},0,[](int){return Colours::green;},change,[](String,int){});present.fixtureRename(0);check(receivedKey==key&&receivedM==1&&receivedR==5,"reconnected E/S resolves original M/R");}
 connected[0].inputs.remove(4);
 {RoutingPanel shortened(connected,names,keys,m,r,{},0,[](int){return Colours::green;},change,[](String,int){});shortened.fixtureRename(0);check(receivedKey==key&&receivedM==1&&receivedR==5,"temporarily missing channel is retained, not remapped");}
 connected.push_back({"Core / Different card","Different card","Core",{"Input 1","Input 2"},{}});
 {RoutingPanel changed(connected,names,keys,m,r,{},0,[](int){return Colours::green;},change,[](String,int){});changed.fixtureSelectDevice(0,2);check(receivedKey=="Core / Different card"&&receivedM==0&&receivedR==0,"explicit device reassignment still clears M/R for user selection");}
 if(argc==3){
  File approved(String::fromUTF8(argv[2]));auto copied=dir.getChildFile("approved-preferences-copy");check(copied.createDirectory().wasOk(),"create local-only approved-state audit fixture");auto approvedFile=copied.getChildFile("proyecto-demo.json");
  {Workstation seed(approvedFile);seed.fixtureCleanSave();} // Synthetic library, never copy private capture payloads.
  std::vector<std::pair<File,String>> snapshots;
  for(auto name:{"proyecto-demo.json","entradas-niveles.json","vista-trabajo.json","vista-graficos.json","tf-graficos.json","medidores.json","medidores-lateral.json","medidores-flotantes.json"}){auto from=approved.getChildFile(name),to=copied.getChildFile(name);check(from.copyFileTo(to),"copy approved JSON into isolated local audit");snapshots.push_back({to,to.loadFileAsString()});}
  auto approvedRoutes=InputRoutePreferences::read(copied.getChildFile("entradas-niveles.json"));
  {Workstation audit(approvedFile);check(audit.fixtureIsClean(),"approved clean sources and hidden TFs restored with no audio");check(approvedRoutes&&audit.fixtureRoutes()==JSON::toString(approvedRoutes->encode()),"approved routes restored exactly including intentional zeros");}
  bool unchanged=true;for(const auto& row:snapshots)unchanged=unchanged&&row.first.loadFileAsString()==row.second;check(unchanged,"all eight approved JSON copies unchanged by startup");
 }
 std::cout<<"No device opened; isolated project/library fixtures only.\n";return failures?1:0;}
