#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
// Paint only: every view supplies its own configured/alarm/pulse colour.
inline void paintMeterSurface(juce::Graphics& g,juce::Rectangle<float> r,juce::Colour base,float radius){
 juce::ColourGradient depth(base.interpolatedWith(juce::Colours::white,.075f),0,r.getY(),base.darker(.22f),0,r.getBottom(),false);depth.addColour(.38,base);g.setGradientFill(depth);g.fillRoundedRectangle(r,radius);
 g.setColour(juce::Colours::black.withAlpha(.60f));g.drawRoundedRectangle(r,radius,.9f);
 auto inner=r.reduced(1.25f);g.setColour(juce::Colours::white.withAlpha(.13f));g.drawRoundedRectangle(inner,juce::jmax(1.f,radius-1),.6f);
 g.setColour(juce::Colours::white.withAlpha(.17f));g.drawLine(r.getX()+radius,r.getY()+1.5f,r.getRight()-radius,r.getY()+1.5f,.7f);
 g.setColour(juce::Colours::black.withAlpha(.24f));g.drawLine(r.getX()+radius,r.getBottom()-2,r.getRight()-radius,r.getBottom()-2,1.2f);
}
