#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#if JUCE_MAC
void installMyaNativeTitleBrand(juce::Component&);
double myaNativeTitleRequiredWidth();
#else
// Native Windows title/icon placement is managed by the OS; no non-client override.
inline void installMyaNativeTitleBrand(juce::Component&){}
#endif
