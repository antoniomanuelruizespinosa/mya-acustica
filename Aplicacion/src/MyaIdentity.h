#pragma once
#include <juce_core/juce_core.h>
namespace MyaIdentity {
inline constexpr const char* officialUtf8="Mya Acústica Analyzer";
inline juce::String name(){return juce::String::fromUTF8(officialUtf8);}
inline juce::String brand(){return juce::String::fromUTF8("Mya Acústica");}
inline juce::String descriptor(){return "Analyzer";}
inline juce::String version(){return MYA_VERSION_STRING;}
}
