#pragma once
#include "CaptureLibrary.h"
#include "Commands.h"
#include "CompactLayout.h"
#include <map>
#include <cmath>
#include <cstdlib>
#include <juce_gui_extra/juce_gui_extra.h>

class CaptureLibraryPanel : public juce::Component, public juce::DragAndDropContainer, private juce::AsyncUpdater {
 using Kind=CaptureLibrary::Kind;
 class PaletteGrid : public juce::Component, private juce::ChangeListener {
  std::function<void(juce::Colour)> choose;
  juce::ColourSelector spectrum{juce::ColourSelector::showColourAtTop|juce::ColourSelector::showColourspace};
  std::array<juce::TextEditor,3> rgb;
  std::array<juce::Label,3> labels;
  juce::TextButton apply{"Apply"},cancel{"Cancel"};
  bool updating=false;
  void syncFields(){updating=true;const auto c=spectrum.getCurrentColour();const int values[]{(int)c.getRed(),(int)c.getGreen(),(int)c.getBlue()};for(int i=0;i<3;++i)rgb[(size_t)i].setText(juce::String(values[i]),false);updating=false;}
  void changeListenerCallback(juce::ChangeBroadcaster*) override{syncFields();}
  void updateSpectrum(){if(updating)return;auto component=[](const juce::TextEditor& field){return (juce::uint8)juce::jlimit(0,255,field.getText().getIntValue());};spectrum.setCurrentColour(juce::Colour(component(rgb[0]),component(rgb[1]),component(rgb[2])));}
 public:
  PaletteGrid(juce::Colour selected,std::function<void(juce::Colour)> callback):choose(std::move(callback)){
   setSize(320,330);spectrum.setCurrentColour(selected);spectrum.addChangeListener(this);addAndMakeVisible(spectrum);
   const char* names[]{"Red","Green","Blue"};for(int i=0;i<3;++i){labels[(size_t)i].setText(names[i],juce::dontSendNotification);labels[(size_t)i].setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(labels[(size_t)i]);rgb[(size_t)i].setInputRestrictions(3,"0123456789");rgb[(size_t)i].onTextChange=[this]{updateSpectrum();};addAndMakeVisible(rgb[(size_t)i]);}
   syncFields();addAndMakeVisible(apply);addAndMakeVisible(cancel);apply.onClick=[this]{auto* dialog=findParentComponentOfClass<juce::DialogWindow>();if(choose)choose(spectrum.getCurrentColour().withAlpha(1.f));if(dialog)dialog->exitModalState(1);};cancel.onClick=[this]{if(auto* dialog=findParentComponentOfClass<juce::DialogWindow>())dialog->exitModalState(0);};
  }
  ~PaletteGrid() override{spectrum.removeChangeListener(this);}
  void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xff17232b));g.setColour(juce::Colours::white);g.setFont(14.f);g.drawText("Color de traza",12,7,getWidth()-24,25,juce::Justification::centred);}
  void resized()override{spectrum.setBounds(12,36,220,225);for(int i=0;i<3;++i){int y=52+i*65;labels[(size_t)i].setBounds(242,y,68,20);rgb[(size_t)i].setBounds(242,y+22,65,28);}apply.setBounds(36,280,105,34);cancel.setBounds(179,280,105,34);}
 };
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
   if(!folder){g.setColour(juce::Colour(0xffa9bac8));g.setFont(9.f);g.drawText(n->kind==Kind::SnapshotTF?(n->role=="average"?"AVG":"TF"):n->kind==Kind::Sweep?"SWP":n->kind==Kind::ImpulseResponse?"IR":"RTA",juce::jmax(21,w-31),0,29,h,juce::Justification::centredRight);}
  }
  void itemClicked(const juce::MouseEvent& e) override{
   if(e.mods.isPopupMenu()){if(folder)panel.folderMenu(id,e.getScreenPosition());else panel.traceMenu(id,e.getScreenPosition());return;}
   if(!folder&&e.x<19){auto key=id;juce::Component::SafePointer<CaptureLibraryPanel> safe(&panel);juce::MessageManager::callAsync([safe,key]{if(safe)safe->toggleTraceVisibility(key);});return;}
   // TreeView updates its selection after itemClicked. Focus it once from
   // handleAsyncUpdate, using the final single-item selection.
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
 juce::TextButton tracesTab{"TF+AVG"},irTab{"Barridos"};juce::TextButton newFolder{"Carpeta +"};juce::TextButton more{juce::String::fromUTF8("≡")},info{"Info"},remove{"Eliminar"};
 std::map<juce::String,bool> openChanges;
 std::unique_ptr<juce::FileChooser> importChooser;
 juce::Button* captureControl=nullptr;
 bool rebuilding=false;MyaCommands::MeterSequence meterSequence;
 enum class SortBy { Original, Name, Type } sortBy=SortBy::Original;
 bool spanishLanguage=true;
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
  if(n.kind==Kind::Folder){bool showAll=inherited||(searching&&matches(n));std::vector<const CaptureLibrary::Node*> children;for(const auto& c:library.state().nodes)if(c.parent==n.id&&c.id!=library.state().active)children.push_back(&c);sortNodes(children);for(const auto* c:children)append(*item,*c,showAll);item->setOpen(searching||n.expanded);}
 }
 void sortNodes(std::vector<const CaptureLibrary::Node*>& nodes)const{if(sortBy==SortBy::Original)return;std::stable_sort(nodes.begin(),nodes.end(),[this](const auto* a,const auto* b){if(sortBy==SortBy::Type&&a->kind!=b->kind)return (int)a->kind<(int)b->kind;return a->name.compareIgnoreCase(b->name)<0;});}
 juce::String destinationForAction(const juce::String& trace={})const{for(const auto& id:selected())if(auto* n=library.find(id))if(n->kind==Kind::Folder)return id;if(trace.isNotEmpty())if(auto* n=library.find(trace))return n->parent;return library.state().active;}
 static std::optional<double> csvNumber(const juce::String& token){auto value=token.trim().toStdString();if(value.empty())return {};char* end=nullptr;double result=std::strtod(value.c_str(),&end);return end!=value.c_str()&&*end=='\0'&&std::isfinite(result)?std::optional<double>(result):std::nullopt;}
 static juce::var csvPayload(const juce::String& content){auto lines=juce::StringArray::fromLines(content);juce::Array<juce::var> samples;int frequency=0,real=1,imaginary=2,coherence=-1,magnitude=-1,phase=-1;bool headerChecked=false;for(auto line:lines){line=line.trim();if(line.isEmpty()||line.startsWithChar('#'))continue;auto cells=juce::StringArray::fromTokens(line.containsChar(';')?line.replaceCharacter(';',','):line,",","");cells.trim();if(!headerChecked){headerChecked=true;if(cells.isEmpty())return {};if(!csvNumber(cells[0])){frequency=real=imaginary=-1;for(int i=0;i<cells.size();++i){auto label=cells[i].toLowerCase().removeCharacters(" _-()[]");if(label=="frequencyhz"||label=="frequency"||label=="freq"||label=="hz")frequency=i;else if(label=="real")real=i;else if(label=="imaginary"||label=="imag")imaginary=i;else if(label=="magnitudedb"||label=="magnitud"||label=="magnitude")magnitude=i;else if(label=="phasedegrees"||label=="phase"||label=="fase")phase=i;else if(label=="coherence"||label=="coherencia")coherence=i;}if(frequency<0||((real<0||imaginary<0)&&(magnitude<0||phase<0)))return {};continue;}}auto number=[&](int index)->std::optional<double>{return index>=0&&index<cells.size()?csvNumber(cells[index]):std::nullopt;};auto hz=number(frequency);auto re=number(real),im=number(imaginary);if(!hz)return {};if(!re||!im){auto db=number(magnitude),degrees=number(phase);if(!db||!degrees)return {};double amplitude=std::pow(10.,*db/20.),angle=*degrees*juce::MathConstants<double>::pi/180.;re=amplitude*std::cos(angle);im=amplitude*std::sin(angle);}auto* point=new juce::DynamicObject();point->setProperty("frequencyHz",*hz);point->setProperty("real",*re);point->setProperty("imaginary",*im);if(auto c=number(coherence))point->setProperty("coherence",*c);samples.add(juce::var(point));}if(samples.size()<2)return {};auto* payload=new juce::DynamicObject();payload->setProperty("schema",1);payload->setProperty("acquisitionMethod","import-csv");payload->setProperty("samples",samples);return juce::var(payload);}
 void importTrace(){auto destination=destinationForAction();auto folder=library.folderFile(destination);if(folder==juce::File{}){if(onError)onError("Selecciona una carpeta de destino");return;}importChooser=std::make_unique<juce::FileChooser>("Importar traza JSON o CSV",folder,"*.json;*.csv");juce::Component::SafePointer<CaptureLibraryPanel> safe(this);importChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe,destination](const juce::FileChooser& chooser){if(!safe)return;auto file=chooser.getResult();if(file.existsAsFile()){auto payload=file.hasFileExtension(".csv")?csvPayload(file.loadFileAsString()):juce::JSON::parse(file);auto id=safe->library.capture(payload,file.getFileNameWithoutExtension(),"ff54ee22",{},false,destination);safe->finish(id.isNotEmpty());}juce::MessageManager::callAsync([safe]{if(safe)safe->importChooser.reset();});});}
 void exportTrace(const juce::String& id){auto target=library.exportTrace(id,destinationForAction(id));if(target==juce::File{}){if(onError)onError(library.lastError());return;}juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Traza exportada",target.getFullPathName(),"Aceptar");}
 void showPalette(const juce::String& id,juce::Point<int>){auto* node=library.find(id);if(!node)return;juce::Component::SafePointer<CaptureLibraryPanel> safe(this);juce::DialogWindow::LaunchOptions options;options.dialogTitle=spanishLanguage?"Color de traza":"Trace colour";options.dialogBackgroundColour=juce::Colour(0xff17232b);options.content.setOwned(new PaletteGrid(juce::Colour::fromString(node->colour),[safe,id](juce::Colour colour){if(safe)safe->finish(safe->library.setColour({id},colour.toString()));}));options.componentToCentreAround=this;options.useNativeTitleBar=false;options.escapeKeyTriggersCloseButton=true;if(auto* dialog=options.launchAsync())dialog->toFront(true);}
 void traceMenu(const juce::String& id,juce::Point<int> screen){auto* node=library.find(id);if(!node||node->kind==Kind::Folder)return;juce::PopupMenu menu;menu.addItem(1,spanishLanguage?juce::String::fromUTF8("Información"):"Information");menu.addItem(2,spanishLanguage?"Renombrar":"Rename");menu.addItem(3,spanishLanguage?"Promedio":"Average");menu.addItem(4,spanishLanguage?"Recapturar":"Recapture");menu.addItem(5,spanishLanguage?"Color...":"Color...");menu.addSeparator();menu.addItem(6,spanishLanguage?"Duplicar":"Duplicate");menu.addItem(7,spanishLanguage?"Exportar datos":"Export data");menu.addItem(8,spanishLanguage?"Eliminar":"Remove");juce::Component::SafePointer<CaptureLibraryPanel> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x,screen.y,1,1}),[safe,id,screen](int action){if(!safe)return;auto* current=safe->library.find(id);if(!current)return;switch(action){case 1:safe->showInfo(id);break;case 2:safe->askName("Renombrar",current->name,[safe,id](juce::String name){if(safe)safe->finish(safe->onRename?safe->onRename(id,name):safe->library.rename(id,name));});break;case 3:if(safe->onAverage){auto ids=safe->selected();if(std::find(ids.begin(),ids.end(),id)==ids.end())ids.push_back(id);safe->onAverage(ids);}break;case 4:if(safe->onRecapture)safe->onRecapture(id);break;case 5:safe->showPalette(id,screen);break;case 6:safe->finish(safe->library.duplicate(id).isNotEmpty());break;case 7:safe->exportTrace(id);break;case 8:safe->finish(safe->library.remove(id));break;default:break;}});}
 void folderMenu(const juce::String& id,juce::Point<int> screen){auto* node=library.find(id);if(!node||node->kind!=Kind::Folder)return;juce::PopupMenu menu;menu.addItem(1,spanishLanguage?juce::String::fromUTF8("Información"):"Information");menu.addItem(2,spanishLanguage?"Renombrar":"Rename");menu.addItem(3,spanishLanguage?"Crear nueva carpeta":"Create new folder");menu.addSeparator();menu.addItem(4,spanishLanguage?"Eliminar carpeta":"Remove folder");juce::Component::SafePointer<CaptureLibraryPanel> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({screen.x,screen.y,1,1}),[safe,id](int action){if(!safe)return;auto* current=safe->library.find(id);if(!current)return;switch(action){case 1:juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,current->name,safe->library.folderPath(id),"Aceptar");break;case 2:safe->askName("Renombrar",current->name,[safe,id](juce::String name){if(safe)safe->finish(safe->onRename?safe->onRename(id,name):safe->library.rename(id,name));});break;case 3:safe->command(901);break;case 4:if(safe->onDeleteFolder)safe->finish(safe->onDeleteFolder(id));break;default:break;}});}
 void askName(juce::String title,juce::String initial,std::function<void(juce::String)> apply){
  auto* box=new juce::AlertWindow(title,"Escribe un nombre:",juce::MessageBoxIconType::NoIcon);box->addTextEditor("name",initial,"Nombre");if(auto* editor=box->getTextEditor("name")){editor->setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xfff2f7fa));editor->setColour(juce::TextEditor::textColourId,juce::Colour(0xff17222b));editor->setColour(juce::TextEditor::outlineColourId,juce::Colour(0xff00c9d8));}box->addButton("Aceptar",1,juce::KeyPress(juce::KeyPress::returnKey));box->addButton("Cancelar",0,juce::KeyPress(juce::KeyPress::escapeKey));
  juce::Component::SafePointer<CaptureLibraryPanel> safe(this);
  box->enterModalState(true,juce::ModalCallbackFunction::create([safe,box,callback=std::move(apply)](int result){if(safe&&result==1)callback(box->getTextEditorContents("name"));}),true);
 }
 void chooseMove(){
  auto ids=selected();if(ids.empty())return;juce::PopupMenu menu;std::vector<juce::String> targets;
  for(const auto& n:library.state().nodes)if(n.kind==Kind::Folder){targets.push_back(n.id);menu.addItem((int)targets.size(),library.folderPath(n.id));}
  juce::Component::SafePointer<CaptureLibraryPanel> safe(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more),[safe,ids,targets](int result){if(safe&&result>0&&(size_t)result<=targets.size())safe->finish(safe->library.move(ids,targets[(size_t)result-1]));});
 }
 void showInfo(const juce::String& target={}){
  auto ids=target.isNotEmpty()?std::vector<juce::String>{target}:selected();if(ids.size()!=1)return;auto* n=library.find(ids.front());if(!n)return;
  juce::String text="ID: "+n->id+"\nCarpeta: "+library.folderPath(n->parent)+"\nArchivo: "+library.dataFile(n->id).getFullPathName();
  if(n->kind!=Kind::Folder){auto trace=library.readTrace(n->id);text+="\nTipo: "+juce::String(n->kind==Kind::SnapshotTF?(n->role=="average"?"TF / AVG":"TF / Individual"):n->kind==Kind::ImpulseResponse?"IR":"RTA")+"\nOrigen: "+n->origin+juce::String::fromUTF8("\nMétodo: ")+n->method+"\nOriginal ID: "+(n->derivedFrom.isEmpty()?n->id:n->derivedFrom);
   if(trace){const auto& v=trace->metadata;int coh=0;for(const auto& s:trace->samples)if(s.coherence)++coh;
    auto value=[&](const juce::String& key){auto item=v[juce::Identifier(key)];return !item.isVoid()&&!item.isUndefined()&&item.toString().isNotEmpty()?item.toString():juce::String("desconocido");};
    text+="\nMuestras: "+juce::String(trace->samples.size())+"\nCoherencia: "+juce::String(coh)+" / "+juce::String(trace->samples.size())+"\nFs: "+value("sampleRateHz")+juce::String::fromUTF8("\nRetardo de medición (ms): ")+value("measurementDelayMs")+"\nReferencia temporal: "+value("timeReference")+"\nDSP en captura: "+value("captureDSPState");
   }
  }
  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,n->name,text,"OK");
 }
bool headerOnly=false;
public:
 void updateComponentLanguage(bool spanish){
  spanishLanguage=spanish;
  tracesTab.setButtonText("TF+AVG");irTab.setButtonText(spanish?"Barridos":"Sweeps");
  newFolder.setButtonText(spanish?"Crear nueva carpeta":"New Folder");
  search.setTextToShowWhenEmpty(spanish?"Buscar":"Search",juce::Colour(0xffa9bac8));
  info.setButtonText(spanish?"Info":"Info");remove.setButtonText(spanish?"Eliminar":"Remove");repaint();
 }
 void importSelectedTrace(){importTrace();}
 void setHeaderOnly(bool only){headerOnly=only;for(auto* c:std::initializer_list<juce::Component*>{&newFolder,&search,&tree,&more,&info,&remove})c->setVisible(!only);if(captureControl)captureControl->setVisible(!only);empty.setVisible(false);}
 std::function<void(int)> onCommand;
 std::function<juce::String(int)> commandText;
 std::function<void()> onChange,onSelection;
 std::function<void(juce::String)> onFront,onError;
 std::function<bool(juce::String,juce::String)> onRename;
 std::function<bool(juce::String)> onDeleteFolder;
 std::function<void(std::vector<juce::String>)> onAverage;
 std::function<void(juce::String)> onRecapture;
 explicit CaptureLibraryPanel(CaptureLibrary& model):library(model){
  tracesTab.onClick=[this]{if(onCommand)onCommand(913);};irTab.onClick=[this]{if(onCommand)onCommand(914);};addAndMakeVisible(tracesTab);addAndMakeVisible(irTab);
  destination.setFont(juce::Font(juce::FontOptions(11.f)));addChildComponent(destination);
  newFolder.onClick=[this]{command(901);};addAndMakeVisible(newFolder);
  search.setTextToShowWhenEmpty("Buscar",juce::Colour(0xffa9bac8));search.onTextChange=[this]{refresh();};addAndMakeVisible(search);
  tree.setRootItemVisible(false);tree.setMultiSelectEnabled(true);tree.setIndentSize(11);tree.setColour(juce::TreeView::backgroundColourId,juce::Colour(0xff11181e));addAndMakeVisible(tree);
  more.onClick=[this]{menu();};addAndMakeVisible(more);info.onClick=[this]{if(onCommand)onCommand(907);};addAndMakeVisible(info);
  remove.setEnabled(false);remove.setTooltip(juce::String::fromUTF8("El borrado recuperable aún no está disponible"));addAndMakeVisible(remove);
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
  std::vector<const CaptureLibrary::Node*> roots;for(const auto& n:library.state().nodes)if(n.parent.isEmpty()&&n.id!=library.state().active)roots.push_back(&n);sortNodes(roots);for(const auto* n:roots)append(*root,*n);
  root->setOpen(true);tree.setRootItem(root.get());
  std::function<void(Item&)> restore=[&](Item& p){if(std::find(selection.begin(),selection.end(),p.id)!=selection.end())p.setSelected(true,false);for(int i=0;i<p.getNumSubItems();++i)restore(*static_cast<Item*>(p.getSubItem(i)));};restore(*root);
  rebuilding=false;tracesTab.setToggleState(library.state().view==Kind::SnapshotTF,juce::dontSendNotification);irTab.setToggleState(library.state().view==Kind::Sweep,juce::dontSendNotification);tracesTab.setEnabled(library.ready());irTab.setEnabled(library.ready());
  destination.setText(juce::String::fromUTF8("→ ")+library.folderPath(library.state().active),juce::dontSendNotification);destination.setTooltip("Destino de captura: "+library.folderPath(library.state().active));
  int count=0;for(const auto& n:library.state().nodes)if(n.kind==library.state().view)++count;
  empty.setText(!library.ready()?library.lastError():count==0?(library.state().view==Kind::SnapshotTF?"Sin capturas TF.":library.state().view==Kind::Sweep?"Sin barridos guardados.":"Sin datos guardados."):juce::String{},juce::dontSendNotification);empty.setVisible(false);tree.setTooltip(empty.getText());info.setEnabled(selection.size()==1);resized();repaint();
 }
 void finish(bool success){if(!success&&onError)onError(library.lastError());refresh();if(onChange)onChange();if(onSelection)onSelection();}
 void toggleTraceVisibility(const juce::String& key){
  auto* node=library.find(key);if(!node||node->kind==Kind::Folder)return;
  const bool show=!node->visible;
  if(!library.setVisibility({key},show)){if(onError)onError(library.lastError());return;}
  tree.repaint();if(onChange)onChange();if(show&&onFront)onFront(key);if(onSelection)onSelection();
 }
 void menu(){juce::PopupMenu menu;auto ids=selected();bool canExport=ids.size()==1&&library.find(ids[0])&&library.find(ids[0])->kind!=Kind::Folder;menu.addItem(1,spanishLanguage?juce::String::fromUTF8("Nueva carpeta de sesión"):"New Session Folder",library.ready());menu.addItem(2,spanishLanguage?"Nueva carpeta":"New Folder",library.ready());menu.addSeparator();menu.addItem(3,spanishLanguage?"Importar traza (JSON/CSV)":"Import Trace (JSON/CSV)",library.ready());menu.addItem(4,spanishLanguage?"Exportar traza seleccionada":"Export Selected Trace",canExport);menu.addSeparator();juce::PopupMenu sort;sort.addItem(101,spanishLanguage?"Orden original":"Original order",true,sortBy==SortBy::Original);sort.addItem(102,spanishLanguage?"Nombre":"Name",true,sortBy==SortBy::Name);sort.addItem(103,spanishLanguage?"Tipo":"Type",true,sortBy==SortBy::Type);menu.addSubMenu(spanishLanguage?"Ordenar por":"Sort By",sort);menu.addItem(5,spanishLanguage?"Ocultar todas":"Hide All",library.ready());menu.addItem(6,spanishLanguage?"Actualizar biblioteca":"Refresh Data Library",library.ready());juce::Component::SafePointer<CaptureLibraryPanel> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more),[safe,ids](int choice){if(!safe)return;switch(choice){case 1:safe->command(900);break;case 2:safe->command(901);break;case 3:safe->importTrace();break;case 4:if(ids.size()==1)safe->exportTrace(ids[0]);break;case 5:{std::vector<juce::String> all;for(const auto& node:safe->library.state().nodes)if(node.kind!=Kind::Folder)all.push_back(node.id);if(!all.empty())safe->finish(safe->library.setVisibility(all,false));break;}case 6:safe->finish(safe->library.reload());break;case 101:case 102:case 103:safe->sortBy=choice==101?SortBy::Original:choice==102?SortBy::Name:SortBy::Type;safe->refresh();break;default:break;}});}
 MyaCommands::Context context() const{
  MyaCommands::Context x;x.libraryReady=library.ready();x.libraryTF=library.state().view==Kind::SnapshotTF;auto ids=selected();x.librarySelection=(int)ids.size();
  for(const auto& id:ids){auto* n=library.find(id);if(n&&n->kind==Kind::Folder)++x.libraryFolders;else if(n)++x.libraryCaptures;}return x;
 }
 void command(int id){
  auto ids=selected();juce::Component::SafePointer<CaptureLibraryPanel> safe(this);
  switch(id){
  case 900:case 901:askName(id==900?juce::String::fromUTF8("Nueva carpeta de sesión"):juce::String("Nueva carpeta"),"",[safe](juce::String name){if(safe)safe->finish(safe->library.createFolder(name,{},true).isNotEmpty());});break;
  case 902:if(ids.size()==1){auto key=ids[0];askName("Renombrar",library.find(key)->name,[safe,key](juce::String name){if(!safe)return;bool ok=safe->onRename?safe->onRename(key,name):safe->library.rename(key,name);if(ok)safe->finish(true);else safe->refresh();});}break;
  case 903:if(ids.size()==1)finish(library.setDestination(ids[0]));break;
  case 904:chooseMove();break;
  case 905:case 906:finish(library.setVisibility(ids,id==905));break;
  case 907:showInfo();break;
  case 908:{juce::PopupMenu colors;const char* names[]={"Verde","Azul","Magenta","Amarillo","Cian","Naranja"};for(int i=0;i<6;++i)colors.addItem(i+1,names[i]);colors.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&more),[safe,ids](int c){const char* values[]={"ff54ee22","ff4d88ff","fff03baf","ffffe33b","ff00e5ed","ffffac24"};if(safe&&c>0&&c<=6)safe->finish(safe->library.setColour(ids,values[c-1]));});break;}
  case 911:finish(library.reload());break;
  case 912:finish(library.setView(Kind::RTA));break;case 913:finish(library.setView(Kind::SnapshotTF));break;case 914:finish(library.setView(Kind::Sweep));break;
  default:break;
  }
 }
 bool keyPressed(const juce::KeyPress& key) override{
  if(!tree.hasKeyboardFocus(true)&&!hasKeyboardFocus(false))return false;
  auto mods=key.getModifiers();bool command=mods.isCommandDown()||mods.isCtrlDown();int code=key.getKeyCode();char c=code>=0&&code<128?(char)std::tolower((unsigned char)code):0;
  if(!command&&!mods.isAltDown()&&!mods.isShiftDown()&&key==juce::KeyPress::returnKey&&selected().size()==1){if(onCommand)onCommand(907);return true;}
  if(MyaCommands::isPlainMeterKey(key.getKeyCode(),mods.isCommandDown(),mods.isCtrlDown(),mods.isAltDown(),mods.isShiftDown())){int id=meterSequence.press(juce::Time::getMillisecondCounterHiRes(),true);if(id&&onCommand)onCommand(id);return true;}
  meterSequence.reset();int id=MyaCommands::shortcut(c,command,mods.isAltDown(),mods.isShiftDown(),false,false);if(id&&onCommand){onCommand(id);return true;}return false;
 }
 #if defined(MYA_OFFSCREEN_RENDER)
 bool previewHeaderMatches(int width,int height)const{return tracesTab.getBounds()==juce::Rectangle<int>{0,0,width,height}&&irTab.getBounds()==juce::Rectangle<int>{width+2,0,width,height}&&more.getY()==search.getY()&&more.getHeight()==search.getHeight()&&more.getWidth()==more.getHeight()&&search.getRight()<more.getX()&&more.getRight()<=getWidth();}
 #endif
 void resized() override{
  int column=CompactLayout::libraryColumnWidth(getWidth()),height=CompactLayout::libraryTabHeight;tracesTab.setBounds(0,0,column,height);irTab.setBounds(column+2,0,column,height);if(headerOnly)return;newFolder.setBounds(3,28,getWidth()-6,25);more.setBounds(getWidth()-28,58,25,25);search.setBounds(3,58,getWidth()-36,25);
  tree.setBounds(2,87,getWidth()-4,juce::jmax(40,getHeight()-119));empty.setBounds(5,91+tree.getNumRowsInTree()*24,getWidth()-10,52);info.setBounds(3,getHeight()-28,30,25);if(captureControl)captureControl->setBounds(36,getHeight()-28,getWidth()-68,25);remove.setButtonText("X");remove.setBounds(getWidth()-29,getHeight()-28,26,25);
 }
};
