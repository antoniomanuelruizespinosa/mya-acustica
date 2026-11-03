#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MyaBrandAssets.h"

namespace PremiumWindowSkin {
inline void paint(juce::Graphics& g, juce::Rectangle<int> bounds) {
    g.fillAll(juce::Colour(0xff11191f));
    auto area=bounds.toFloat().reduced(4.f);
    g.setColour(juce::Colour(0xff1c2a33));
    g.fillRoundedRectangle(area,15.f);
    auto logo=juce::ImageCache::getFromMemory(MyaBrandAssets::myaanalyzericonv1_png,MyaBrandAssets::myaanalyzericonv1_pngSize);
    if(logo.isValid()){
        const int size=juce::jmin(190,juce::jmin(bounds.getWidth(),bounds.getHeight())*2/3);
        g.setOpacity(.065f);
        g.drawImageWithin(logo,bounds.getCentreX()-size/2,bounds.getCentreY()-size/2,size,size,juce::RectanglePlacement::centred);
        g.setOpacity(1.f);
    }
    g.setColour(juce::Colour(0xff405865));
    g.drawRoundedRectangle(area,15.f,1.f);
}
}
