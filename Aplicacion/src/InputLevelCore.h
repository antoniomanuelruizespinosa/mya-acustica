#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace InputLevels {
constexpr int maxChannels=256;
struct Route {std::string device;int mic=0,ref=0;};
struct Source {std::string device;int channel=0;};
struct Device {std::string key;int channels=0;};
struct Group {std::string key;std::array<bool,maxChannels> enabled{};};
struct Plan {std::vector<Group> groups;std::string error;bool valid()const{return error.empty()&&!groups.empty();}};
inline Plan plan(const std::array<Route,4>& routes,unsigned mask,const std::vector<Source>& observers,const std::vector<Device>& devices){
 Plan result;auto include=[&](const std::string& key,int channel)->bool{auto found=std::find_if(devices.begin(),devices.end(),[&](const Device& d){return d.key==key;});if(found==devices.end()){result.error="Dispositivo no disponible: "+key;return false;}if(channel<1||channel>found->channels||channel>maxChannels){result.error="Canal de entrada no disponible: "+key+" / "+std::to_string(channel);return false;}auto g=std::find_if(result.groups.begin(),result.groups.end(),[&](const Group& group){return group.key==key;});if(g==result.groups.end()){result.groups.push_back({key,{}});g=std::prev(result.groups.end());}g->enabled[(size_t)channel-1]=true;return true;};
 for(size_t i=0;i<4;++i)if(mask&(1u<<i)){const auto& r=routes[i];if(r.device.empty()&&r.mic==0&&r.ref==0)continue;if(r.device.empty()||!include(r.device,r.mic)||!include(r.device,r.ref)){if(result.error.empty())result.error="Ruta M/R incompleta TF "+std::to_string(i+1);return result;}}
 for(const auto& source:observers)if(!source.device.empty()&&!include(source.device,source.channel))return result;
 if(result.groups.empty())result.error="Asigna al menos una pareja M/R o una entrada dBFS antes de Play";return result;
}
inline bool available(const Plan& p,const std::vector<Device>& devices){for(const auto& g:p.groups){auto d=std::find_if(devices.begin(),devices.end(),[&](const Device& item){return item.key==g.key;});if(d==devices.end())return false;for(int i=0;i<maxChannels;++i)if(g.enabled[(size_t)i]&&i>=d->channels)return false;}return true;}
struct Peak {float amplitude=0;bool valid=false,clipped=false;double sumSquares=0;std::uint64_t samples=0;};
inline Peak peak(const float* samples,int count){if(!samples||count<=0)return {};Peak p;p.valid=true;for(int i=0;i<count;++i){float value=samples[i];if(!std::isfinite(value))return {};p.sumSquares+=(double)value*(double)value;++p.samples;value=std::abs(value);p.amplitude=std::max(p.amplitude,value);p.clipped=p.clipped||value>=1.f;}return p;}
inline std::optional<double> dbfs(const Peak& p){if(!p.valid)return {};if(p.amplitude<=0)return -INFINITY;return 20.*std::log10((double)p.amplitude);}
struct Frame {std::uint64_t sequence=0;double timeSeconds=0;std::array<Peak,maxChannels> channels{};};
// JUCE supplies enabled channels packed in physical ascending order.
inline Frame capture(const float* const* input,int inputs,int samples,const std::array<int,maxChannels>& physical,int count,std::uint64_t sequence){Frame frame;frame.sequence=sequence;if(inputs!=count||count<1||count>maxChannels||samples<=0)return frame;for(int i=0;i<count;++i)if(physical[(size_t)i]>=0&&physical[(size_t)i]<maxChannels)frame.channels[(size_t)physical[(size_t)i]]=peak(input?input[i]:nullptr,samples);return frame;}
template<size_t capacity=64> class Queue {
 std::array<Frame,capacity+1> items{};std::atomic<size_t> write{0},read{0};
public:
 bool push(const Frame& f)noexcept{auto w=write.load(std::memory_order_relaxed),next=(w+1)%(capacity+1);if(next==read.load(std::memory_order_acquire))return false;items[w]=f;write.store(next,std::memory_order_release);return true;}
 bool pop(Frame& f)noexcept{auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;f=items[r];read.store((r+1)%(capacity+1),std::memory_order_release);return true;}
};
struct Window {std::array<Peak,maxChannels> channels{};bool any=false,gap=false;std::uint64_t lastSequence=0;
 void merge(const Frame& f){if(any&&f.sequence!=lastSequence+1)gap=true;for(size_t i=0;i<channels.size();++i){if(!any)channels[i]=f.channels[i];else{channels[i].valid=channels[i].valid&&f.channels[i].valid;channels[i].amplitude=std::max(channels[i].amplitude,f.channels[i].amplitude);channels[i].clipped=channels[i].clipped||f.channels[i].clipped;channels[i].sumSquares+=f.channels[i].sumSquares;channels[i].samples+=f.channels[i].samples;}}any=true;lastSequence=f.sequence;}
};
}
