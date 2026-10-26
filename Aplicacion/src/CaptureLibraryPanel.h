#pragma once
#include "CaptureLibrary.h"
#include "Commands.h"
#include <map>
#include <juce_gui_extra/juce_gui_extra.h>

class CaptureLibraryPanel : public juce::Component, public juce::DragAndDropContainer, private juce::AsyncUpdater {
 using Kind=CaptureLibrary::Kind;
 struct Item : public juce::TreeViewItem {
  CaptureLibraryPanel& panel;juce::String id;bool folder=false;
  Item(CaptureLibraryPanel& p,juce::String key,bool isFolder):panel(p),id(std::move(key)),folder(isFolder){}
  juce::String getUniqueName() const override{return id;}
  bool mightContainSubItems() override{return folder;}
  int getItemHeight() const override{return 24;}
  juce::String getTooltip() override{auto* n=panel.library.find(id);return n?panel.library.folderPath(id)+(n->kind==Kind::Folder?"": juce::String::fromUTF8(" · ")+n->origin+juce::String::fromUTF8(" · ")+n->method):juce::String{};}
  void paintItem(juce::Graphics& g,int w,int h) override{
   auto* n=panel.library.find(id);if(!n)return;
   if(isSelected()){g.setColour(juce::Colour(0xff254555));g.fillRect(0,0,w,h);}
   if(folder){g.setColour(juce::Colour(0xffd4b36c));g.fillRoundedRectangle(2,7,14,11,2);g.fillRect(3,4,7,5);}
   else{auto c=juce::Colour::fromString(n->colour);g.setColour(c.withAlpha(n->visible?1.f:.25f));g.fillEllipse(2,6,13,13);if(!n->visible){g.setColour(c);g.drawLine(3,7,14,18,1.5f);g.drawLine(14,7,3,18,1.5f);}}
   g.setColour(juce::Colour(0xffe5edf4));g.setFont(12.f);g.drawFittedText(n->name,{21,0,juce::jmax(1,w-(folder?23:55)),h},juce::Justification::centredLeft,1,.8f);
   if(!folder){g.setColour(juce::Colour(0xffa9bac8));g.setFont(9.f);g.drawText(n->kind==Kind::SnapshotTF?(n->role=="average"?"AVG":"TF"):n->kind==Kind::ImpulseResponse?"IR":"RTA",juce::jmax(21,w-31),0,29,h,juce::Justification::centredRight);}
  }
  void itemClicked(const juce::MouseEvent& e) override{
   if(e.mods.isPopupMenu()){panel.menu();return;}
   if(!folder&&e.x<19){auto* n=panel.library.find(id);if(n){auto key=id;bool visible=!n->visible;juce::Component::SafePointer<CaptureLibraryPanel> safe(&panel);juce::MessageManager::callAsync([safe,key,visible]{if(safe)safe->finish(safe->library.setVisibility({key},visible));});}return;}
   if(!folder&&panel.onFront)panel.onFront(id);
  }
  void itemDoubleClicked(const juce::MouseEvent&) override{if(!folder)panel.command(907);}
  void itemOpennessChanged(bool open) override{
   if(panel.rebuilding||id.isEmpty())return;
   // Persist openness after the current tree event; do not destroy this item in its callback.
   panel.openChanges[id]=open;panel.triggerAsyncUpdate();
  }
  void itemSelectionChanged(bool) override{if(!panel.rebuilding){panel.triggerAsyncUpdate();}}
  juce::var getDragSourceDescription() override{
   auto ids=panel.selected();if(ids.empty())ids.push_back(id);juce::Array<juce::var> list;for(const auto& key:ids)list.add(key);
   auto* o=new juce::DynamicObject();o->setProperty("libraryId",panel.library.state().id);o->setProperty("ids",list);return juce::var(o);
  }
  bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& d) override{return folder&&d.description["libraryId"].toString()==panel.library.state().id&&d.description["ids"].isArray();}
  void itemDropped(const juce::DragAndDropTarget::SourceDetails& d,int) override{
   if(!isInterestedInDragSource(d))return;std::vector<juce::String> ids;for(const auto& v:*d.description["ids"].getArray())ids.push_back(v.toString());auto targetId=id;juce::Component::SafePointer<CaptureLibraryPanel> safe(&panel);juce::MessageManager::callAsync([safe,ids,targetId]{if(safe)safe->finish(safe->library.move(ids,targetId));});
  }
 };
 CaptureLibrary& library;juce::TreeView tree;std::unique_ptr<Item> root;
 juce::Label destination,empty;juce::TextEditor search;
 juce::TextButton tracesTab{"TF+AVG"},irTab{"Sweep/IR"};juce::TextButton more{juce::String::fromUTF8("≡")},info{"Info"},remove{"Eliminar"};
 std::map<juce::String,bool> openChanges;
 juce::Button* captureControl=nullptr;
 bool rebuilding=false;MyaCommands::MeterSequence meterSequence;
 void handleAsyncUpdate() override{
  auto changes=std::move(openChanges);openChanges.clear();bool changed=false;
  for(const auto& item:changes){if(library.setExpanded(item.first,item.second))changed=true;else if(onError)onError(library.lastError());}
  info.setEnabled(selected().size()==1);auto current=selected();if(current.size()==1){auto* n=library.find(current[0]);if(n&&n->kind!=Kind::Folder&&n->visible&&onFront)onFront(n->id);}if(changed&&onChange)onChange();if(onSelection)onSelection();
 }
 bool matches(const CaptureLibrary::Node& n) const{
  auto term=search.getText().trim();return term.isEmpty()||n.name.containsIgnoreCase(term);
 }
 bool subtreeMatches(const CaptureLibrary::Node& n) const{
  if(n.kind!=Kind::Folder)return n.kind==library.state().view&&matches(n);
  if(matches(n))return true;for(const auto& c:library.state().nodes)if(c.parent==n.id&&subtreeMatches(c))return true;return false;
 }
 void append(Item& parent,const CaptureLibrary::Node& n,bool inherited=false){
  bool searching=search.getText().trim().isNotEmpty();if(n.kind!=Kind::Folder&&n.kind!=library.state().view)return;
  if(!inherited&&!subtreeMatches(n)&&n.id!=library.state().active)return;
  auto* item=new Item(*this,n.id,n.kind==Kind::Folder);parent.addSubItem(item);
  if(n.kind==Kind::Folder){bool showAll=inherited||(searching&&matches(n));for(const auto& c:library.state().nodes)if(c.parent==n.id&&c.id!=library.state().active)append(*item,c,showAll);item->setOpen(searching||n.expanded);}
 }
 juce::String folderForNew() const{
  auto ids=selected();if(ids.size()==1){auto* n=library.find(ids[0]);if(n&&n->kind==Kind::Folder)return n->id;}return library.state().active;
 }
 void askName(juce::String title,juce::String initial,std::function<void(juce::String)> apply){
  auto* box=new juce::AlertWindow(title,"Nombre / Name",juce::MessageBoxIconType::NoIcon);box->addTextEditor("name",initial);box->addButton("Aceptar / OK",1,juce::KeyPress(juce::KeyPress::returnKey));box->addButton("Cancelar / Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
  juce::Component::SafePointer<CaptureLibraryPanel> safe(this);
  box->enterModalState(true,juce::ModalCallbackFunction::create([safe,box,callback=std::move(apply)](int result){if(safe&&result==1)callback(box->getTextEditorContents("name"));}),true);
 }
 void chooseMove(){
  auto ids=selected();if(ids.empty())return;juce::PopupMenu menu;std::vector<juce::String> targets;
  for(const auto& n:library.state().nodes)if(n.kind==Kind::Folder){targets.push_back(n.id);menu.addItem((int)targets.size(),library.folderPath(n.id));}
  juce::Component::SafePointer<CaptureLibraryPanel> safe(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more),[safe,ids,targets](int result){if(safe&&result>0&&(size_t)result<=targets.size())safe->finish(safe->library.move(ids,targets[(size_t)result-1]));});
 }
 void showInfo(){
  auto ids=selected();if(ids.size()!=1)return;auto* n=library.find(ids.front());if(!n)return;
  juce::String text="ID: "+n->id+"\nCarpeta / Folder: "+library.folderPath(n->parent)+"\nArchivo / File: "+library.dataFile(n->id).getFullPathName();
  if(n->kind!=Kind::Folder){auto trace=library.readTrace(n->id);text+="\nTipo / Type: "+juce::String(n->kind==Kind::SnapshotTF?(n->role=="average"?"TF / AVG":"TF / Individual"):n->kind==Kind::ImpulseResponse?"IR":"RTA")+"\nOrigen / Origin: "+n->origin+juce::String::fromUTF8("\nMétodo / Method: ")+n->method+"\nOriginal ID: "+(n->derivedFrom.isEmpty()?n->id:n->derivedFrom);
   if(trace){const auto& v=trace->metadata;int coh=0;for(const auto& s:trace->samples)if(s.coherence)++coh;
    auto value=[&](const juce::String& key){auto item=v[juce::Identifier(key)];return !item.isVoid()&&!item.isUndefined()&&item.toString().isNotEmpty()?item.toString():juce::String("desconocido / unknown");};
    text+="\nMuestras / Samples: "+juce::String(trace->samples.size())+"\nCoherencia / Coherence: "+juce::String(coh)+" / "+juce::String(trace->samples.size())+"\nFs: "+value("sampleRateHz")+juce::String::fromUTF8("\nDelay de medición / Measurement delay (ms): ")+value("measurementDelayMs")+"\nReferencia temporal / Time reference: "+value("timeReference")+"\nDSP en captura / Capture DSP: "+value("captureDSPState");
   }
  }
  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,n->name,text,"OK");
 }
public:
 std::function<void(int)> onCommand;
 std::function<juce::String(int)> commandText;
 std::function<void()> onChange,onSelection;
 std::function<void(juce::String)> onFront,onError;
 explicit CaptureLibraryPanel(CaptureLibrary& model):library(model){
  tracesTab.onClick=[this]{if(onCommand)onCommand(913);};irTab.onClick=[this]{if(onCommand)onCommand(914);};addAndMakeVisible(tracesTab);addAndMakeVisible(irTab);
  destination.setFont(juce::Font(juce::FontOptions(11.f)));addAndMakeVisible(destination);
  search.setTextToShowWhenEmpty("Buscar / Search",juce::Colour(0xffa9bac8));search.onTextChange=[this]{refresh();};addAndMakeVisible(search);
  tree.setRootItemVisible(false);tree.setMultiSelectEnabled(true);tree.setIndentSize(11);tree.setColour(juce::TreeView::backgroundColourId,juce::Colour(0xff11181e));addAndMakeVisible(tree);
  more.onClick=[this]{menu();};addAndMakeVisible(more);info.onClick=[this]{if(onCommand)onCommand(907);};addAndMakeVisible(info);
  remove.setEnabled(false);remove.setTooltip("Borrado recuperable pendiente / Recoverable deletion pending");addAndMakeVisible(remove);
  empty.setFont(juce::Font(juce::FontOptions(11.f)));empty.setJustificationType(juce::Justification::topLeft);empty.setInterceptsMouseClicks(false,false);addAndMakeVisible(empty);
  refresh();
 }
 ~CaptureLibraryPanel() override{rebuilding=true;cancelPendingUpdate();tree.setRootItem(nullptr);}
 std::vector<juce::String> selected() const{
  std::vector<juce::String> ids;for(int i=0;i<tree.getNumSelectedItems();++i)if(auto* item=dynamic_cast<Item*>(tree.getSelectedItem(i)))ids.push_back(item->id);return ids;
 }
 void setCaptureControl(juce::Button& button){captureControl=&button;button.setTransform({});addAndMakeVisible(button);resized();}
 void select(const juce::String& id){if(auto* item=tree.findItemFromIdentifierString("/root/"+id))item->setSelected(true,true);}
 void refresh(){
  auto selection=selected();rebuilding=true;tree.setRootItem(nullptr);root=std::make_unique<Item>(*this,"root",true);
  auto* active=library.find(library.state().active);if(active)append(*root,*active);
  for(const auto& n:library.state().nodes)if(n.parent.isEmpty()&&n.id!=library.state().active)append(*root,n);
  root->setOpen(true);tree.setRootItem(root.get());
  std::function<void(Item&)> restore=[&](Item& p){if(std::find(selection.begin(),selection.end(),p.id)!=selection.end())p.setSelected(true,false);for(int i=0;i<p.getNumSubItems();++i)restore(*static_cast<Item*>(p.getSubItem(i)));};restore(*root);
  rebuilding=false;tracesTab.setToggleState(library.state().view==Kind::SnapshotTF,juce::dontSendNotification);irTab.setToggleState(library.state().view==Kind::ImpulseResponse,juce::dontSendNotification);tracesTab.setEnabled(library.ready());irTab.setEnabled(library.ready());
  destination.setText(juce::String::fromUTF8("→ ")+library.folderPath(library.state().active),juce::dontSendNotification);destination.setTooltip("Destino de captura / Capture destination: "+library.folderPath(library.state().active));
  int count=0;for(const auto& n:library.state().nodes)if(n.kind==library.state().view)++count;
  empty.setText(!library.ready()?library.lastError():count==0?(library.state().view==Kind::SnapshotTF?"Sin capturas TF. Espacio guarda una DEMO en el destino.":(library.state().view==Kind::RTA?juce::String::fromUTF8("RTA: sin datos. Adquisición/importación pendientes."):juce::String::fromUTF8("Sweep/IR: sin datos. Adquisición/importación pendientes."))):juce::String{},juce::dontSendNotification);empty.setVisible(false);tree.setTooltip(empty.getText());info.setEnabled(selection.size()==1);resized();repaint();
 }
 void finish(bool success){if(!success&&onError)onError(library.lastError());refresh();if(onChange)onChange();if(onSelection)onSelection();}
 void menu(){if(!onCommand)return;juce::PopupMenu m;
  for(int id:{913,914,912,900,901,902,903,904,905,906,907,908,909,910,911}){
   auto* c=MyaCommands::find(id);auto x=context();auto s=MyaCommands::state(*c,x);m.addItem(id,commandText?commandText(id):juce::String::fromUTF8(c->name.es.c_str()),s.enabled,s.checked);
  }
  juce::Component::SafePointer<CaptureLibraryPanel> safe(this);m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more),[safe](int id){if(safe&&id&&safe->onCommand)safe->onCommand(id);});
 }
 MyaCommands::Context context() const{
  MyaCommands::Context x;x.libraryReady=library.ready();x.libraryTF=library.state().view==Kind::SnapshotTF;auto ids=selected();x.librarySelection=(int)ids.size();
  for(const auto& id:ids){auto* n=library.find(id);if(n&&n->kind==Kind::Folder)++x.libraryFolders;else if(n)++x.libraryCaptures;}return x;
 }
 void command(int id){
  auto ids=selected();juce::Component::SafePointer<CaptureLibraryPanel> safe(this);
  switch(id){
  case 900:askName(juce::String::fromUTF8("Nueva sesión / New session"),"",[safe](juce::String name){if(safe)safe->finish(safe->library.createFolder(name,{},true).isNotEmpty());});break;
  case 901:{auto parent=folderForNew();askName("Nueva carpeta / New folder","",[safe,parent](juce::String name){if(safe)safe->finish(safe->library.createFolder(name,parent).isNotEmpty());});break;}
  case 902:if(ids.size()==1){auto key=ids[0];askName("Renombrar / Rename",library.find(key)->name,[safe,key](juce::String name){if(safe)safe->finish(safe->library.rename(key,name));});}break;
  case 903:if(ids.size()==1)finish(library.setDestination(ids[0]));break;
  case 904:chooseMove();break;
  case 905:case 906:finish(library.setVisibility(ids,id==905));break;
  case 907:showInfo();break;
  case 908:{juce::PopupMenu colors;const char* names[]={"Verde / Green","Azul / Blue","Magenta","Amarillo / Yellow","Cian / Cyan","Naranja / Orange"};for(int i=0;i<6;++i)colors.addItem(i+1,names[i]);colors.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more),[safe,ids](int c){const char* values[]={"ff54ee22","ff4d88ff","fff03baf","ffffe33b","ff00e5ed","ffffac24"};if(safe&&c>0&&c<=6)safe->finish(safe->library.setColour(ids,values[c-1]));});break;}
  case 911:finish(library.reload());break;
  case 912:finish(library.setView(Kind::RTA));break;case 913:finish(library.setView(Kind::SnapshotTF));break;case 914:finish(library.setView(Kind::ImpulseResponse));break;
  default:break;
  }
 }
 bool keyPressed(const juce::KeyPress& key) override{
  if(!tree.hasKeyboardFocus(true)&&!hasKeyboardFocus(false))return false;
  auto mods=key.getModifiers();bool command=mods.isCommandDown()||mods.isCtrlDown();int code=key.getKeyCode();char c=code>=0&&code<128?(char)std::tolower((unsigned char)code):0;
  if(key==juce::KeyPress::returnKey&&selected().size()==1){if(onCommand)onCommand(907);return true;}
  if(c=='e'&&!command&&!mods.isAltDown()){int id=meterSequence.press(juce::Time::getMillisecondCounterHiRes(),true);if(id&&onCommand)onCommand(id);return true;}
  meterSequence.reset();int id=MyaCommands::shortcut(c,command,mods.isAltDown(),mods.isShiftDown(),false,false);if(id&&onCommand){onCommand(id);return true;}return false;
 }
 void resized() override{
  tracesTab.setBounds(2,0,54,22);irTab.setBounds(58,0,getWidth()-86,22);more.setBounds(getWidth()-26,0,24,22);destination.setBounds(4,26,getWidth()-8,30);search.setBounds(3,59,getWidth()-6,25);
  tree.setBounds(2,88,getWidth()-4,juce::jmax(40,getHeight()-120));empty.setBounds(5,92+tree.getNumRowsInTree()*24,getWidth()-10,52);info.setBounds(3,getHeight()-28,30,25);if(captureControl)captureControl->setBounds(36,getHeight()-28,getWidth()-68,25);remove.setButtonText("X");remove.setBounds(getWidth()-29,getHeight()-28,26,25);
 }
};
