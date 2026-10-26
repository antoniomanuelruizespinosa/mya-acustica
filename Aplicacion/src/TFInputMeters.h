#pragma once
#include "TFMeterModel.h"
#include <juce_gui_extra/juce_gui_extra.h>

class TFInputMeters : public juce::Component {
public:
 juce::Colour colour{0xff54ee22};TFMeterSettings settings;std::array<TFMeterChannel,2> channels;
 TFInputMeters(){setInterceptsMouseClicks(false,false);}
 void resetPeaks(){for(auto& c:channels)c.peak.reset();repaint();}
 void paint(juce::Graphics& g) override{
  for(int ch=0;ch<2;++ch){auto& c=channels[(size_t)ch];float y=(float)ch*28;juce::Rectangle<float> bar{17,y+1,(float)getWidth()-19,10};
   g.setColour(juce::Colour(0xff090d10));g.fillRoundedRectangle(bar,5);g.setColour(juce::Colour(0xff48525a));g.drawRoundedRectangle(bar,.0f+5,.7f);
   g.setFont(10.f);g.setColour(juce::Colour(0xffa9bac8));g.drawText(ch==0?"M":"R",0,(int)y,14,12,juce::Justification::centredLeft);
   if(c.level){
    // Clip each threshold section to the same capsule. The base colour never changes.
    float filledWidth=bar.getWidth()*(float)TFMeterChannel::fraction(*c.level);juce::Path capsule;capsule.addRoundedRectangle(bar.withWidth(filledWidth),juce::jmin(5.f,filledWidth*.5f));g.saveState();g.reduceClipRegion(capsule);
    float end=bar.getX()+filledWidth;
    auto section=[&](double a,double b,juce::Colour col){float left=bar.getX()+bar.getWidth()*(float)TFMeterChannel::fraction(a),right=juce::jmin(end,bar.getX()+bar.getWidth()*(float)TFMeterChannel::fraction(b));if(right>left){g.setColour(col);g.fillRect(left,bar.getY(),right-left,bar.getHeight());}};
    section(-60,settings.orangeDbFS,colour);section(settings.orangeDbFS,settings.redDbFS,juce::Colour(0xffffac24));section(settings.redDbFS,0,juce::Colour(0xffff4040));
    if(settings.gloss){g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.22f),0,bar.getY(),juce::Colours::transparentWhite,0,bar.getCentreY(),false));g.fillRect(bar.withWidth(juce::jmax(0.f,end-bar.getX())));}
    g.restoreState();
   }
   if(settings.peakOn&&c.peak){float x=juce::jlimit(bar.getX()+1,bar.getRight()-1,bar.getX()+bar.getWidth()*(float)TFMeterChannel::fraction(*c.peak));g.setColour(juce::Colours::white);g.drawLine(x,bar.getY()+1,x,bar.getBottom()-1,1.5f);}
   g.setFont(9.f);g.setColour(juce::Colour(0xffa9bac8));g.drawText("-60     -36     -12   0",17,(int)y+12,juce::jmax(1,getWidth()-97),12,juce::Justification::centredLeft);
   g.setColour(c.clipped()?juce::Colour(0xffff4040):juce::Colour(0xffe5edf4));g.drawText(c.level?juce::String(*c.level,1)+" dBFS":"-- dBFS",getWidth()-78,(int)y+12,76,12,juce::Justification::centredRight);
  }
 }
};

class TFMeterSettingsPanel : public juce::Component {
 juce::ToggleButton peak{"Pico retenido blanco"},gloss{"Brillo suave"};juce::ComboBox duration;
 juce::Slider orange,red;juce::Label note,orangeLabel,redLabel;juce::TextButton reset{"Restablecer picos"};
 TFMeterSettings state;std::function<void(TFMeterSettings)> changed;
 void apply(){auto s=state;s.peakOn=peak.getToggleState();s.gloss=gloss.getToggleState();s.holdSeconds=duration.getSelectedId()*.5;s.orangeDbFS=orange.getValue();s.redDbFS=red.getValue();if(s.valid()){state=s;changed(s);}else{orange.setValue(state.orangeDbFS,juce::dontSendNotification);red.setValue(state.redDbFS,juce::dontSendNotification);}}
public:
 TFMeterSettingsPanel(TFMeterSettings s,std::function<void(TFMeterSettings)> fn,std::function<void()> clear):state(s),changed(std::move(fn)){
  setSize(420,250);peak.setToggleState(s.peakOn,juce::dontSendNotification);gloss.setToggleState(s.gloss,juce::dontSendNotification);
  for(int i=1;i<=120;++i)duration.addItem(juce::String(i*.5,1)+" s",i);duration.setSelectedId(juce::jlimit(1,120,juce::roundToInt(s.holdSeconds*2)),juce::dontSendNotification);
  for(auto* slider:{&orange,&red}){slider->setRange(-60,0,.5);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,82,24);slider->setTextValueSuffix(" dBFS");slider->onValueChange=[this]{apply();};}
  orange.setValue(s.orangeDbFS,juce::dontSendNotification);red.setValue(s.redDbFS,juce::dontSendNotification);
  orangeLabel.setText("Tramo naranja",juce::dontSendNotification);redLabel.setText("Tramo rojo",juce::dontSendNotification);
  note.setText(juce::String::fromUTF8("M/R: entradas dBFS, sin niveles hasta adquirir audio.\nEl pico retenido no es un indicador de clip.\nUmbrales independientes de SPL; naranja < rojo ≤ 0 dBFS."),juce::dontSendNotification);note.setFont(juce::Font(juce::FontOptions(12.f)));
  reset.onClick=std::move(clear);peak.onClick=[this]{apply();};gloss.onClick=[this]{apply();};duration.onChange=[this]{apply();};
  for(auto* c:std::initializer_list<juce::Component*>{&peak,&gloss,&duration,&orange,&red,&note,&orangeLabel,&redLabel,&reset})addAndMakeVisible(c);
 }
 void resized() override{peak.setBounds(10,8,230,26);duration.setBounds(260,8,145,26);gloss.setBounds(10,40,230,26);orangeLabel.setBounds(10,78,120,26);orange.setBounds(130,78,275,26);redLabel.setBounds(10,112,120,26);red.setBounds(130,112,275,26);reset.setBounds(10,150,180,26);note.setBounds(10,184,395,62);}
};
