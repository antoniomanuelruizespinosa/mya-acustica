#pragma once
#include "AudioRouting.h"
#include "InputLevelCore.h"
#include <juce_gui_extra/juce_gui_extra.h>

// The tables describe the audio endpoints. Routing remains in the existing
// four TF selectors and the single generator output selector below them.
class IOConfigWindow : public juce::DocumentWindow {
 class Body : public juce::Component {
  struct Table : juce::TableListBoxModel {
   juce::TableListBox view;
   juce::StringArray rows;
   std::function<juce::String(int,int)> cell;
   std::function<void(int)> selected;
   Table(const juce::StringArray& headings,const std::vector<int>& widths):view({},this){
    for(int i=0;i<headings.size();++i)view.getHeader().addColumn(headings[i],i+1,widths[(size_t)i],30,2000);
    view.getHeader().setStretchToFitActive(false);
    view.setRowHeight(25);view.setOutlineThickness(1);view.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xfff4f4f4));
   }
   int getNumRows() override{return rows.size();}
   void paintRowBackground(juce::Graphics& g,int row,int w,int h,bool chosen) override{g.fillAll(chosen?juce::Colour(0xffb3dce8):row%2?juce::Colour(0xffededed):juce::Colours::white);g.setColour(juce::Colour(0xffaaaaaa));g.drawHorizontalLine(h-1,0.f,(float)w);}
   void paintCell(juce::Graphics& g,int row,int col,int w,int h,bool) override{g.setColour(juce::Colour(0xff171717));g.setFont(13.f);g.drawFittedText(cell?cell(row,col):juce::String{},juce::Rectangle<int>(5,0,w-10,h),juce::Justification::centredLeft,1);g.setColour(juce::Colour(0xffbbbbbb));g.drawVerticalLine(w-1,0.f,(float)h);}
   void selectedRowsChanged(int row) override{if(selected&&row>=0)selected(row);}
   void refresh(){view.updateContent();view.repaint();}
  };
  juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};
  juce::Component inputPage,outputPage;
  Table inputDevices{{"Use","API : Driver Name","Friendly Name","Status"},{55,280,290,95}};
  Table outputDevices{{"Use","API : Driver Name","Friendly Name","Status"},{55,280,290,95}};
  Table inputChannels{{"Use","Ch","Channel Name","Friendly Name","Cal. Offset","Mic Correction Curve","Level"},{48,40,150,150,85,155,85}};
  Table outputChannels{{"Use","Ch","Channel Name","Friendly Name","Trim"},{42,34,127,127,62}};
  Table presetSlots{{"Slot","Preset Name"},{50,170}};
  juce::Label inputDeviceTitle,outputDeviceTitle,routeTitle,outputTitle,presetTitle,status;
  juce::TextButton calibrate{"Calibrar SPL RMS"},close{"Cerrar"};
  juce::TextButton clearInput{"Clear Settings"},clearOutput{"Clear Settings"},resetTrims{"Reset Trims"},storePreset{"Store"},activatePreset{"Make Active"},deletePreset{"Delete"};
  juce::ToggleButton gainTracking{"Gain Tracking"},loopback{"Enable Smaart Loopback"};
  std::array<juce::Label,4> routeName;
  std::array<juce::ComboBox,4> routeDevice,routeMic,routeRef;
  juce::ComboBox generatorDevice,generatorChannel;
  std::vector<AudioEndpoint> endpoints;
  std::array<juce::String,4> routeKeys;
  std::array<int,4> micValues,refValues;
  juce::String outputKey;
  int outputValue=0,inputSelected=0,outputSelected=0;
  std::function<void(int,juce::String,int,int)> routeChanged;
  std::function<void(juce::String,int)> outputChanged;
  std::function<void()> calibrateRequested,closeRequested;
  void populate(juce::ComboBox& box,bool input,const juce::String& value){box.clear(juce::dontSendNotification);int selected=0;for(size_t i=0;i<endpoints.size();++i){const auto& d=endpoints[i];if((input?d.inputs:d.outputs).isEmpty())continue;box.addItem(d.name+" ["+d.backend+"]",(int)i+1);if(d.key==value)selected=(int)i+1;}box.setSelectedId(selected,juce::dontSendNotification);box.setTextWhenNothingSelected("Sin asignar");}
  juce::String key(int id)const{return id>0&&(size_t)id<=endpoints.size()?endpoints[(size_t)id-1].key:juce::String{};}
  void channelItems(juce::ComboBox& box,const juce::StringArray& labels,int chosen,bool output){box.clear(juce::dontSendNotification);box.addItemList(labels,1);if(output&&!labels.isEmpty())box.addItem("Todas las salidas (mono)",InputLevels::maxChannels+1);if(chosen>0&&box.indexOfItemId(chosen)<0){box.addItem((output?"Output ":"Input ")+juce::String(chosen)+" [Unavailable]",chosen);box.setItemEnabled(chosen,false);}box.setSelectedId(chosen,juce::dontSendNotification);box.setTextWhenNothingSelected("Sin asignar");}
  void updateRouteChannels(int i){auto k=(size_t)i;auto id=routeDevice[k].getSelectedId();const auto* d=id>0&&(size_t)id<=endpoints.size()?&endpoints[(size_t)id-1]:nullptr;channelItems(routeMic[k],d?d->inputs:juce::StringArray{},micValues[k],false);channelItems(routeRef[k],d?d->inputs:juce::StringArray{},refValues[k],false);}
  void updateOutputChannels(){auto id=generatorDevice.getSelectedId();const auto* d=id>0&&(size_t)id<=endpoints.size()?&endpoints[(size_t)id-1]:nullptr;channelItems(generatorChannel,d?d->outputs:juce::StringArray{},outputValue,true);}
  void selectDevice(bool input,int row){if(row<0||(size_t)row>=endpoints.size())return;auto& index=input?inputSelected:outputSelected;index=row;auto& title=input?inputDeviceTitle:outputDeviceTitle;title.setText(endpoints[(size_t)row].name,juce::dontSendNotification);auto& table=input?inputChannels:outputChannels;table.rows.clear();const auto& names=input?endpoints[(size_t)row].inputs:endpoints[(size_t)row].outputs;for(const auto& name:names)table.rows.add(name);table.refresh();}
  void setTables(){for(const auto& d:endpoints){inputDevices.rows.add(d.name);outputDevices.rows.add(d.name);}auto deviceCell=[this](bool input,int row,int col)->juce::String{if(row<0||(size_t)row>=endpoints.size())return juce::String{};const auto& d=endpoints[(size_t)row];if(col==1){bool used=false;if(input){for(const auto& key:routeKeys)used|=key==d.key;}else used=outputKey==d.key;return used?juce::String("[x]"):juce::String("[ ]");}if(col==2)return d.backend+" : "+d.name;if(col==3)return d.name;return juce::String((input?d.inputs:d.outputs).isEmpty()?"--":"Detectado");};inputDevices.cell=[deviceCell](int r,int c){return deviceCell(true,r,c);};outputDevices.cell=[deviceCell](int r,int c){return deviceCell(false,r,c);};inputChannels.cell=[this](int r,int c)->juce::String{if((size_t)inputSelected>=endpoints.size())return juce::String{};auto& d=endpoints[(size_t)inputSelected];if(r<0||r>=d.inputs.size())return juce::String{};bool used=false;for(size_t i=0;i<4;++i)used|=routeKeys[i]==d.key&&(micValues[i]==r+1||refValues[i]==r+1);switch(c){case 1:return used?juce::String("[x]"):juce::String("[ ]");case 2:return juce::String(r+1);case 3:case 4:return d.inputs[r];case 5:return "--";case 6:return "None";default:return "--";}};outputChannels.cell=[this](int r,int c)->juce::String{if((size_t)outputSelected>=endpoints.size())return juce::String{};auto& d=endpoints[(size_t)outputSelected];if(r<0||r>=d.outputs.size())return juce::String{};bool used=outputKey==d.key&&(outputValue==r+1||outputValue==InputLevels::maxChannels+1);switch(c){case 1:return used?juce::String("[x]"):juce::String("[ ]");case 2:return juce::String(r+1);case 3:case 4:return d.outputs[r];default:return "--";}};inputDevices.selected=[this](int r){selectDevice(true,r);};outputDevices.selected=[this](int r){selectDevice(false,r);};inputDevices.refresh();outputDevices.refresh();if(!endpoints.empty()){inputDevices.view.selectRow(inputSelected);outputDevices.view.selectRow(outputSelected);selectDevice(true,inputSelected);selectDevice(false,outputSelected);}}
 public:
  Body(std::vector<AudioEndpoint> catalog,const std::array<juce::String,4>& names,const std::vector<juce::String>& keys,const std::vector<int>& mic,const std::vector<int>& ref,juce::String outKey,int outChannel,std::function<void(int,juce::String,int,int)> change,std::function<void(juce::String,int)> outChange,std::function<void()> calibration,std::function<void()> dismiss):endpoints(std::move(catalog)),routeKeys{{keys[0],keys[1],keys[2],keys[3]}},micValues{{mic[0],mic[1],mic[2],mic[3]}},refValues{{ref[0],ref[1],ref[2],ref[3]}},outputKey(std::move(outKey)),outputValue(outChannel),routeChanged(std::move(change)),outputChanged(std::move(outChange)),calibrateRequested(std::move(calibration)),closeRequested(std::move(dismiss)){
   addAndMakeVisible(tabs);tabs.addTab("Input",juce::Colour(0xff20272c),&inputPage,false);tabs.addTab("Output",juce::Colour(0xff20272c),&outputPage,false);
   for(auto* table:{&inputDevices,&inputChannels})inputPage.addAndMakeVisible(table->view);for(auto* table:{&outputDevices,&outputChannels,&presetSlots})outputPage.addAndMakeVisible(table->view);
   presetSlots.cell=[](int row,int col)->juce::String{return col==1?juce::String(row):juce::String("---");};for(int i=0;i<10;++i)presetSlots.rows.add(juce::String(i));presetSlots.refresh();
   for(auto* control:{static_cast<juce::Component*>(&clearInput),static_cast<juce::Component*>(&gainTracking)})inputPage.addAndMakeVisible(control);
   for(auto* control:{static_cast<juce::Component*>(&clearOutput),static_cast<juce::Component*>(&resetTrims),static_cast<juce::Component*>(&storePreset),static_cast<juce::Component*>(&activatePreset),static_cast<juce::Component*>(&deletePreset),static_cast<juce::Component*>(&loopback)})outputPage.addAndMakeVisible(control);
   for(auto* button:{&clearInput,&clearOutput,&resetTrims,&storePreset,&activatePreset,&deletePreset})button->setEnabled(false);gainTracking.setEnabled(false);loopback.setEnabled(false);
   for(auto* label:{&inputDeviceTitle,&routeTitle})inputPage.addAndMakeVisible(label);for(auto* label:{&outputDeviceTitle,&outputTitle,&presetTitle})outputPage.addAndMakeVisible(label);
   routeTitle.setText("Rutas TF",juce::dontSendNotification);outputTitle.setText("Ruta del generador",juce::dontSendNotification);presetTitle.setText("Output Preset Manager",juce::dontSendNotification);
   for(int i=0;i<4;++i){auto k=(size_t)i;routeName[k].setText(names[k],juce::dontSendNotification);inputPage.addAndMakeVisible(routeName[k]);for(auto* box:{&routeDevice[k],&routeMic[k],&routeRef[k]})inputPage.addAndMakeVisible(box);populate(routeDevice[k],true,routeKeys[k]);updateRouteChannels(i);routeDevice[k].onChange=[this,i,k]{routeKeys[k]=key(routeDevice[k].getSelectedId());micValues[k]=refValues[k]=0;updateRouteChannels(i);routeChanged(i,routeKeys[k],0,0);inputDevices.refresh();inputChannels.refresh();};routeMic[k].onChange=[this,i,k]{micValues[k]=routeMic[k].getSelectedId();routeChanged(i,routeKeys[k],micValues[k],refValues[k]);inputChannels.refresh();};routeRef[k].onChange=[this,i,k]{refValues[k]=routeRef[k].getSelectedId();routeChanged(i,routeKeys[k],micValues[k],refValues[k]);inputChannels.refresh();};}
   outputPage.addAndMakeVisible(generatorDevice);outputPage.addAndMakeVisible(generatorChannel);populate(generatorDevice,false,outputKey);updateOutputChannels();generatorDevice.onChange=[this]{outputKey=key(generatorDevice.getSelectedId());outputValue=0;updateOutputChannels();outputChanged(outputKey,0);outputDevices.refresh();outputChannels.refresh();};generatorChannel.onChange=[this]{outputValue=generatorChannel.getSelectedId();outputChanged(outputKey,outputValue);outputChannels.refresh();};
   addAndMakeVisible(status);addAndMakeVisible(calibrate);addAndMakeVisible(close);calibrate.onClick=[this]{if(calibrateRequested)calibrateRequested();};close.onClick=[this]{if(closeRequested)closeRequested();};setAcquisitionState(false);setSize(820,650);setTables();
  }
  void setAcquisitionState(bool active,bool waiting=false,const juce::String& details={}){status.setText(waiting?"Esperando permiso":active?"Entradas activas":"Entradas detenidas",juce::dontSendNotification);status.setTooltip(details);}
  void resized() override{auto w=getWidth(),h=getHeight();tabs.setBounds(8,8,w-16,h-58);status.setBounds(16,h-42,270,28);calibrate.setBounds(w-335,h-42,190,28);close.setBounds(w-130,h-42,110,28);auto layout=[&](juce::Component& page,Table& devices,juce::Label& title){auto pw=page.getWidth();devices.view.setBounds(12,14,pw-24,164);title.setBounds(18,186,pw-36,24);};layout(inputPage,inputDevices,inputDeviceTitle);layout(outputPage,outputDevices,outputDeviceTitle);auto iw=inputPage.getWidth(),ih=inputPage.getHeight();inputChannels.view.setBounds(12,214,iw-24,ih-214-187);clearInput.setBounds(18,ih-182,135,25);gainTracking.setBounds(iw-175,ih-182,160,25);routeTitle.setBounds(18,ih-150,iw-36,22);for(int i=0;i<4;++i){int y=ih-125+i*27;routeName[(size_t)i].setBounds(20,y,100,24);routeDevice[(size_t)i].setBounds(120,y,260,24);routeMic[(size_t)i].setBounds(390,y,150,24);routeRef[(size_t)i].setBounds(550,y,150,24);}auto ow=outputPage.getWidth(),oh=outputPage.getHeight(),split=ow*62/100;outputChannels.view.setBounds(12,214,split-20,oh-214-128);presetTitle.setBounds(split+8,186,ow-split-20,24);presetSlots.view.setBounds(split+4,214,ow-split-16,oh-214-174);clearOutput.setBounds(18,oh-123,128,25);resetTrims.setBounds(split-145,oh-123,125,25);deletePreset.setBounds(split+8,oh-166,85,24);activatePreset.setBounds(split+98,oh-166,116,24);storePreset.setBounds(ow-83,oh-166,70,24);loopback.setBounds(split+8,oh-129,ow-split-20,28);outputTitle.setBounds(18,oh-88,ow-36,22);generatorDevice.setBounds(18,oh-57,ow/2-26,27);generatorChannel.setBounds(ow/2+4,oh-57,ow/2-22,27);}
  int selectedTab()const{return tabs.getCurrentTabIndex();}
  void selectTab(int index){tabs.setCurrentTabIndex(index);}
 };
 Body* body;
public:
 IOConfigWindow(std::vector<AudioEndpoint> catalog,const std::array<juce::String,4>& names,const std::vector<juce::String>& keys,const std::vector<int>& mic,const std::vector<int>& ref,juce::String outKey,int outChannel,std::function<void(int,juce::String,int,int)> change,std::function<void(juce::String,int)> outChange,std::function<void()> calibration):DocumentWindow("I-O Config",juce::Colour(0xff20272c),closeButton){setUsingNativeTitleBar(true);body=new Body(std::move(catalog),names,keys,mic,ref,std::move(outKey),outChannel,std::move(change),std::move(outChange),std::move(calibration),[this]{setVisible(false);});setContentOwned(body,true);setResizable(true,false);setResizeLimits(760,570,1500,1100);centreWithSize(820,650);}
 void setAcquisitionState(bool active,bool waiting=false,const juce::String& details={}){body->setAcquisitionState(active,waiting,details);}
 int selectedTab()const{return body->selectedTab();}
 void selectTab(int index){body->selectTab(index);}
 void closeButtonPressed()override{setVisible(false);}
};
