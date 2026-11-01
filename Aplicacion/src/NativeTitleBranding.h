#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#if JUCE_MAC
void installMyaNativeTitleBrand(juce::Component&);
double myaNativeTitleRequiredWidth();
void attachMyaFloatingWindow(juce::Component& main,juce::Component& floating);
#else
// Native Windows title/icon placement is managed by the OS; no non-client override.
inline void installMyaNativeTitleBrand(juce::Component&){}
inline void attachMyaFloatingWindow(juce::Component&,juce::Component&){}
#endif
