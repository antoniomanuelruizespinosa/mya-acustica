#pragma once
#include <juce_audio_devices/juce_audio_devices.h>

struct AudioEndpoint {
 juce::String key, name, backend;
 juce::StringArray inputs, outputs;
};
inline std::vector<AudioEndpoint> discoverAudioEndpoints(){
 juce::AudioDeviceManager manager;
 juce::OwnedArray<juce::AudioIODeviceType> types;
 manager.createAudioDeviceTypes(types);
 std::vector<AudioEndpoint> endpoints;
 for(auto* type:types){
  type->scanForDevices();
  auto ins=type->getDeviceNames(true), outs=type->getDeviceNames(false);
  auto names=ins; names.addArray(outs);names.removeDuplicates(false);
  for(const auto& name:names){
   std::unique_ptr<juce::AudioIODevice> device(type->createDevice(outs.contains(name)?name:juce::String(),ins.contains(name)?name:juce::String()));
   if(device)endpoints.push_back({type->getTypeName()+" / "+name,name,type->getTypeName(),device->getInputChannelNames(),device->getOutputChannelNames()});
  }
 }
 return endpoints;
}
inline const AudioEndpoint* findEndpoint(const std::vector<AudioEndpoint>& devices,const juce::String& key){
 for(const auto& d:devices)if(d.key==key)return &d;
 return nullptr;
}

// Keep backend listeners alive so connect/disconnect notifications reach the UI.
class AudioDeviceWatcher : public juce::ChangeBroadcaster, private juce::AudioIODeviceType::Listener {
 juce::OwnedArray<juce::AudioIODeviceType> types;
 void audioDeviceListChanged() override {sendChangeMessage();}
public:
 AudioDeviceWatcher(){juce::AudioDeviceManager manager;manager.createAudioDeviceTypes(types);for(auto* type:types){type->scanForDevices();type->addListener(this);}}
 ~AudioDeviceWatcher() override {for(auto* type:types)type->removeListener(this);}
};
