#pragma once
#include "Commands.h"
#include "CommandText.h"
#include <juce_gui_extra/juce_gui_extra.h>

class CommandUI {
 struct Binding {juce::Button* button;int id;bool compact;};
 std::vector<Binding> bindings;
public:
 MyaCommands::Language language=MyaCommands::Language::ES;
 std::function<MyaCommands::Context()> context;
 std::function<void(int)> action;
 static juce::String utf8(const std::string& s){return juce::String::fromUTF8(s.c_str());}
 MyaCommands::Context current() const{auto x=context?context():MyaCommands::Context{};x.language=language;return x;}
 juce::String title(const MyaCommands::Command& c) const{return utf8(c.name.get(language));}
 juce::String details(const MyaCommands::Command& c,const MyaCommands::State& s) const{
  return MyaCommands::commandText(c,s,language);
 }
 bool invoke(int id){bool result=MyaCommands::dispatch(id,current(),action);refresh();return result;}
 void bind(juce::Button& b,int id,bool compact=false){jassert(MyaCommands::find(id)!=nullptr);bindings.push_back({&b,id,compact});b.setClickingTogglesState(false);b.onClick=[this,id]{invoke(id);};}
 void refresh(){auto x=current();for(auto& b:bindings){auto* c=MyaCommands::find(b.id);if(!c)continue;auto s=MyaCommands::state(*c,x);
  b.button->setEnabled(s.enabled);b.button->setToggleState(s.checked,juce::dontSendNotification);
  b.button->setTitle(title(*c));b.button->setTooltip(details(*c,s));
  if(b.button->getProperties()["transportIcon"].toString().isEmpty()&&!bool(b.button->getProperties()["commandGlyph"]))b.button->setButtonText(b.compact?utf8(MyaCommands::compactName(*c,language)):title(*c));
 }}
 juce::StringArray menuNames() const{juce::StringArray names;for(const auto& m:MyaCommands::menus())names.add(utf8(m.get(language)));return names;}
 juce::PopupMenu menu(int group) const{
  auto x=current();std::function<juce::PopupMenu(juce::StringArray)> branch;
  branch=[&](juce::StringArray prefix){juce::PopupMenu menu;juce::StringArray children;
   for(const auto& c:MyaCommands::catalog()){if(c.menu==group){auto path=juce::StringArray::fromTokens(utf8(c.path.get(language)),"/","");path.trim();path.removeEmptyStrings();bool belongs=path.size()>=prefix.size();for(int i=0;i<prefix.size()&&belongs;++i)belongs=path[i]==prefix[i];if(!belongs)continue;
    if(path.size()==prefix.size()){auto s=MyaCommands::state(c,x);menu.addItem(c.id,details(c,s),s.enabled,s.checked);}else{auto child=path[prefix.size()];if(!children.contains(child)){children.add(child);auto nested=prefix;nested.add(child);menu.addSubMenu(child,branch(nested));}}
   }}
   return menu;
  };auto root=branch({});auto add=[&](juce::PopupMenu& menu,const MyaCommands::Command& c){auto s=MyaCommands::state(c,x);menu.addItem(c.id,details(c,s),s.enabled,s.checked);};
  // Repeated placements reuse exactly the same ID, availability and action.
  for(const auto& placement:MyaCommands::extraMenus)if(placement[0]==group){root.addSeparator();add(root,*MyaCommands::find(placement[1]));}
  return root;
 }
 juce::String shortcutsHelp() const{
  juce::String value;
  for(const auto& c:MyaCommands::catalog())if(!c.shortcut.empty())value+=details(c,MyaCommands::state(c,current()))+"\n";
  value+=language==MyaCommands::Language::ES?juce::String::fromUTF8("\nIzquierda: reducir intervalo X; derecha: ampliar intervalo X; arriba/abajo: Y. Shift reduce el paso Y. D/V conservan navegación demo. Doble clic: centrar panel.\nLos atajos no actúan mientras se escribe o se usa otro control."):"\nLeft narrows X; right widens X; up/down scroll Y. Shift reduces the Y step. D/V retain demo navigation. Double click: centre panel.\nShortcuts do not run while typing or operating another control.";
  return value;
 }
};
