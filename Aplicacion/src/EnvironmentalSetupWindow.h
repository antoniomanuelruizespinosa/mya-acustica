#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "MyaBrandAssets.h"

// Visual preview only. Values never enter the acoustic or audio engines.
class EnvironmentalSetupPanel : public juce::Component {
    juce::Image logo=juce::ImageCache::getFromMemory(MyaBrandAssets::myaanalyzericonv1_png,MyaBrandAssets::myaanalyzericonv1_pngSize);
    juce::Label title,temperatureLabel,humidityLabel,note;
    juce::TextEditor temperature,humidity;
    juce::TextButton close;
    bool spanish=true;
public:
    std::function<void()> onClose;
    explicit EnvironmentalSetupPanel(bool useSpanish):spanish(useSpanish){
        setSize(440,280);
        for(auto* label:{&title,&temperatureLabel,&humidityLabel,&note}){
            label->setColour(juce::Label::textColourId,juce::Colour(0xffe8f1f5));
            label->setInterceptsMouseClicks(false,false);
            addAndMakeVisible(*label);
        }
        title.setFont(juce::Font(juce::FontOptions(17.f)).boldened());
        note.setFont(juce::Font(juce::FontOptions(11.f)));
        note.setJustificationType(juce::Justification::centred);
        for(auto* field:{&temperature,&humidity}){
            field->setColour(juce::TextEditor::backgroundColourId,juce::Colours::transparentBlack);
            field->setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);
            field->setColour(juce::TextEditor::focusedOutlineColourId,juce::Colour(0xff00d8e6));
            field->setColour(juce::TextEditor::textColourId,juce::Colours::white);
            field->setJustification(juce::Justification::centredLeft);
            field->setSelectAllWhenFocused(true);
            field->setInputRestrictions(7,"0123456789-.,");
            addAndMakeVisible(*field);
        }
        temperature.setText("20",false);humidity.setText("50",false);
        close.onClick=[this]{if(onClose)onClose();};
        close.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff243640));
        close.setColour(juce::TextButton::textColourOffId,juce::Colour(0xffe8f1f5));
        addAndMakeVisible(close);
        updateComponentLanguage(spanish);
    }
    void updateComponentLanguage(bool useSpanish){
        spanish=useSpanish;
        title.setText(spanish?juce::String::fromUTF8("Configuración ambiental"):"Environmental Setup",juce::dontSendNotification);
        temperatureLabel.setText(spanish?juce::String::fromUTF8("Temperatura (°C)"):juce::String::fromUTF8("Temperature (°C)"),juce::dontSendNotification);
        humidityLabel.setText(spanish?"Humedad relativa (%)":"Relative humidity (%)",juce::dontSendNotification);
        note.setText(spanish?juce::String::fromUTF8("Vista previa: los valores aún no modifican el cálculo acústico."):"Preview: these values do not yet affect acoustic calculations.",juce::dontSendNotification);
        close.setButtonText(spanish?"Cerrar":"Close");
        repaint();
    }
    void resized() override{
        const int x=30,w=getWidth()-60;
        title.setBounds(x,20,w,28);
        temperatureLabel.setBounds(x,69,w,20);temperature.setBounds(x+4,94,w-8,34);
        humidityLabel.setBounds(x,143,w,20);humidity.setBounds(x+4,168,w-8,34);
        note.setBounds(x,214,w,25);close.setBounds(getWidth()-126,getHeight()-39,96,27);
    }
    void paint(juce::Graphics& g) override{
        g.fillAll(juce::Colour(0xff11191f));
        auto area=getLocalBounds().toFloat().reduced(5.f);
        g.setColour(juce::Colour(0xff1c2a33));g.fillRoundedRectangle(area,15.f);
        if(logo.isValid()){
            g.setOpacity(.075f);
            g.drawImageWithin(logo,getWidth()/2-95,getHeight()/2-95,190,190,juce::RectanglePlacement::centred);
            g.setOpacity(1.f);
        }
        g.setColour(juce::Colour(0xff405865));g.drawRoundedRectangle(area,15.f,1.f);
        g.setColour(juce::Colour(0xff263c47));
        g.fillRoundedRectangle(juce::Rectangle<float>(34.f,94.f,(float)getWidth()-68.f,34.f),9.f);
        g.fillRoundedRectangle(juce::Rectangle<float>(34.f,168.f,(float)getWidth()-68.f,34.f),9.f);
    }
};

class EnvironmentalSetupWindow : public juce::DocumentWindow {
    EnvironmentalSetupPanel* panel=nullptr;
public:
    explicit EnvironmentalSetupWindow(bool spanish):juce::DocumentWindow(spanish?juce::String::fromUTF8("Configuración ambiental"):"Environmental Setup",juce::Colour(0xff11191f),juce::DocumentWindow::closeButton){
        setUsingNativeTitleBar(true);
        panel=new EnvironmentalSetupPanel(spanish);
        panel->onClose=[this]{setVisible(false);};
        setContentOwned(panel,true);
        setResizable(false,false);
        centreWithSize(440,280);
    }
    void updateComponentLanguage(bool spanish){
        setName(spanish?juce::String::fromUTF8("Configuración ambiental"):"Environmental Setup");
        if(panel)panel->updateComponentLanguage(spanish);
    }
    void closeButtonPressed() override{setVisible(false);}
};
