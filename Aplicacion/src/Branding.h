#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "MyaBrandAssets.h"
#include "MyaIdentity.h"

class MyaStartupPanel : public juce::Component {
 juce::Image icon=juce::ImageCache::getFromMemory(MyaBrandAssets::myaanalyzericonv1_png,MyaBrandAssets::myaanalyzericonv1_pngSize);bool starting;
 juce::TextButton proceed{"Continuar"};
public:
 std::function<void()> onContinue;
 explicit MyaStartupPanel(bool realStartup):starting(realStartup){setSize(420,360);proceed.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff18242d));proceed.setColour(juce::TextButton::textColourOffId,juce::Colour(0xff00e5ed));setTitle("Inicio / "+MyaIdentity::name());addAndMakeVisible(proceed);proceed.onClick=[this]{if(onContinue)onContinue();};}
 bool assetValid()const{return icon.isValid()&&icon.hasAlphaChannel();}
 void resized()override{proceed.setBounds(getWidth()/2-58,getHeight()-40,116,26);}
 void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xff11181d));g.drawImageWithin(icon,getWidth()/2-64,20,128,128,juce::RectanglePlacement::centred);g.setColour(juce::Colour(0xffe5edf4));g.setFont(25.f);g.drawText(MyaIdentity::brand(),10,168,getWidth()-20,32,juce::Justification::centred);g.setColour(juce::Colour(0xff00e5ed));g.setFont(18.f);g.drawText(MyaIdentity::descriptor(),10,203,getWidth()-20,26,juce::Justification::centred);g.setColour(juce::Colour(0xffa9bac8));g.setFont(12.f);g.drawText(MyaIdentity::version(),10,248,getWidth()-20,22,juce::Justification::centred);g.drawText(starting?"Iniciando analizador...":"Vista previa del inicio",10,277,getWidth()-20,22,juce::Justification::centred);}
};
class MyaStartupWindow : public juce::DocumentWindow {
public:
 explicit MyaStartupWindow(bool realStartup):DocumentWindow(realStartup?"Inicio":"Vista previa del inicio",juce::Colour(0xff11181d),juce::DocumentWindow::closeButton){setUsingNativeTitleBar(true);auto* content=new MyaStartupPanel(realStartup);juce::Component::SafePointer<MyaStartupWindow> safe(this);content->onContinue=[safe]{if(safe)safe->setVisible(false);};setContentOwned(content,true);setResizable(false,false);centreWithSize(420,360);setVisible(true);}
 void closeButtonPressed()override{setVisible(false);}
};
inline void showMyaStartupPreview(){juce::DialogWindow::LaunchOptions options;auto content=std::make_unique<MyaStartupPanel>(false);auto* panel=content.get();panel->onContinue=[safe=juce::Component::SafePointer<MyaStartupPanel>(panel)]{if(safe)if(auto* window=dynamic_cast<juce::DialogWindow*>(safe->getTopLevelComponent()))window->exitModalState(0);};options.dialogTitle="Vista previa del inicio";options.dialogBackgroundColour=juce::Colour(0xff11181d);options.useNativeTitleBar=true;options.resizable=false;options.content.setOwned(content.release());options.launchAsync();}

// Approved icon15, embedded without redesign. This panel owns no audio objects.
class MyaAboutPanel : public juce::Component {
 juce::Image icon=juce::ImageCache::getFromMemory(MyaBrandAssets::myaanalyzericonv1_png,MyaBrandAssets::myaanalyzericonv1_pngSize);juce::TextButton preview{"Ver pantalla de inicio"};
public:
 MyaAboutPanel(){addAndMakeVisible(preview);preview.onClick=[]{showMyaStartupPreview();};setSize(420,340);setTitle(MyaIdentity::name());}
 void resized()override{preview.setBounds(getWidth()/2-105,getHeight()-40,210,28);}
 bool assetValid()const{return icon.isValid()&&icon.hasAlphaChannel()&&icon.getPixelAt(0,0).getAlpha()==0;}
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff222427));g.drawImageWithin(icon,getWidth()/2-48,12,96,96,juce::RectanglePlacement::centred);
  g.setColour(juce::Colour(0xffe5edf4));g.setFont(23.f);g.drawText(MyaIdentity::brand(),12,118,getWidth()-24,30,juce::Justification::centred);
  g.setColour(juce::Colour(0xff00e5ed));g.setFont(17.f);g.drawText(MyaIdentity::descriptor(),12,151,getWidth()-24,25,juce::Justification::centred);
  g.setColour(juce::Colour(0xffa9bac8));g.setFont(12.f);g.drawFittedText(MyaIdentity::version()+juce::String::fromUTF8("\nEntradas dBFS con Play explícito\nSPL calibrado / TF M/R, IR, RTA y espectrograma reales"),14,190,getWidth()-28,94,juce::Justification::centred,5);
 }
};
inline void showMyaAbout(){juce::DialogWindow::LaunchOptions options;options.dialogTitle="Acerca de / "+MyaIdentity::name();options.dialogBackgroundColour=juce::Colour(0xff222427);options.useNativeTitleBar=true;options.resizable=false;options.content.setOwned(new MyaAboutPanel());options.launchAsync();}
