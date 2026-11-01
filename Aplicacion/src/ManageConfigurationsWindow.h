#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

class ManageConfigurationsWindow : public juce::DocumentWindow {
 class Body : public juce::Component {
  juce::Label heading,feedback;
  juce::TextEditor name;
  juce::ComboBox saved;
  juce::TextButton store{"Guardar configuración"},exportButton{"Exportar..."},restoreButton{"Restaurar"},close{"Cerrar"};
  std::function<juce::StringArray()> list;
  std::function<bool(juce::String)> save;
  std::function<bool(juce::String,const juce::File&)> exportTo;
  std::function<bool(juce::String)> restore;
  std::function<void()> dismiss;
  std::shared_ptr<juce::FileChooser> chooser;
  void refresh(){auto current=saved.getText();auto items=list();saved.clear(juce::dontSendNotification);for(int i=0;i<items.size();++i)saved.addItem(items[i],i+1);saved.setSelectedId(juce::jmax(1,items.indexOf(current)+1),juce::dontSendNotification);bool any=items.size()>0;exportButton.setEnabled(any);restoreButton.setEnabled(any);}
 public:
  Body(std::function<juce::StringArray()> available,std::function<bool(juce::String)> storeAction,std::function<bool(juce::String,const juce::File&)> exportAction,std::function<bool(juce::String)> restoreAction,std::function<void()> closeAction):list(std::move(available)),save(std::move(storeAction)),exportTo(std::move(exportAction)),restore(std::move(restoreAction)),dismiss(std::move(closeAction)){
   heading.setText("Canales, nombres y distribución de pantalla",juce::dontSendNotification);for(auto* component:{static_cast<juce::Component*>(&heading),static_cast<juce::Component*>(&feedback),static_cast<juce::Component*>(&name),static_cast<juce::Component*>(&saved),static_cast<juce::Component*>(&store),static_cast<juce::Component*>(&exportButton),static_cast<juce::Component*>(&restoreButton),static_cast<juce::Component*>(&close)})addAndMakeVisible(component);
   name.setText("Mi configuración");name.setSelectAllWhenFocused(true);saved.setTextWhenNothingSelected("Sin configuraciones guardadas");refresh();
   store.onClick=[this]{auto title=name.getText().trim();if(title.isEmpty()){feedback.setText("Escribe un nombre.",juce::dontSendNotification);return;}bool ok=save(title);feedback.setText(ok?"Configuración guardada.":"No se pudo guardar.",juce::dontSendNotification);if(ok){refresh();saved.setText(title,juce::dontSendNotification);}};
   exportButton.onClick=[this]{auto title=saved.getText();if(title.isEmpty())return;chooser=std::make_shared<juce::FileChooser>("Exportar configuración",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(juce::File::createLegalFileName(title)+".json"),"*.json");auto keep=chooser;juce::Component::SafePointer<Body> safe(this);chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe,keep,title](const juce::FileChooser& selected){if(!safe)return;auto target=selected.getResult();if(target==juce::File{})return;bool ok=safe->exportTo(title,target);safe->feedback.setText(ok?"Configuración exportada.":"No se pudo exportar.",juce::dontSendNotification);safe->chooser.reset();});};
   restoreButton.onClick=[this]{auto title=saved.getText();if(title.isEmpty())return;bool ok=restore(title);feedback.setText(ok?"Configuración restaurada.":"No se pudo restaurar.",juce::dontSendNotification);};close.onClick=[this]{if(dismiss)dismiss();};setSize(480,240);
  }
  void resized()override{int w=getWidth();heading.setBounds(16,12,w-32,25);name.setBounds(16,50,w-165,28);store.setBounds(w-140,50,124,28);saved.setBounds(16,94,w-32,28);exportButton.setBounds(16,135,132,28);restoreButton.setBounds(160,135,132,28);close.setBounds(w-108,135,92,28);feedback.setBounds(16,181,w-32,28);}
 };
public:
 ManageConfigurationsWindow(std::function<juce::StringArray()> available,std::function<bool(juce::String)> save,std::function<bool(juce::String,const juce::File&)> exportTo,std::function<bool(juce::String)> restore):juce::DocumentWindow("Manage Configurations...",juce::Colour(0xff20272c),juce::DocumentWindow::closeButton){setUsingNativeTitleBar(true);setContentOwned(new Body(std::move(available),std::move(save),std::move(exportTo),std::move(restore),[this]{setVisible(false);}),true);setResizable(false,false);centreWithSize(480,240);}
 void closeButtonPressed()override{setVisible(false);}
};
