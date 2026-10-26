#pragma once
#include "Commands.h"
#include <juce_core/juce_core.h>
namespace MyaCommands {
inline juce::String commandText(const Command& c,const State& state,Language language){
 auto value=juce::String::fromUTF8(c.name.get(language).c_str());
 if(!c.shortcut.empty())value+=" ["+juce::String::fromUTF8(c.shortcut.c_str())+"]";
 if(c.capability==Capability::Demo)value+=juce::String::fromUTF8(" · demo");
 if(!state.reason.get(language).empty())value+=juce::String::fromUTF8(" — ")+juce::String::fromUTF8(state.reason.get(language).c_str());
 return value;
}
}
