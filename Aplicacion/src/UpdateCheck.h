#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

namespace MyaUpdate {
inline constexpr const char* apiUrl="https://api.github.com/repos/antoniomanuelruizespinosa/mya-acustica/releases/latest";
inline constexpr const char* releasePrefix="https://github.com/antoniomanuelruizespinosa/mya-acustica/releases/";

struct Version {int major=0,minor=0,patch=0;};
inline bool parseVersion(juce::String value,Version& out){
 value=value.trim();if(value.startsWithChar('v'))value=value.substring(1);
 auto parts=juce::StringArray::fromTokens(value,".","");if(parts.size()!=3)return false;
 int numbers[3]{};
 for(int i=0;i<3;++i){
  auto part=parts[i];if(part.isEmpty()||part.length()>8||part.containsOnly("0123456789")==false)return false;
  numbers[i]=part.getIntValue();
 }
 out={numbers[0],numbers[1],numbers[2]};return true;
}
inline bool newer(Version candidate,Version current){
 if(candidate.major!=current.major)return candidate.major>current.major;
 if(candidate.minor!=current.minor)return candidate.minor>current.minor;
 return candidate.patch>current.patch;
}
enum class State {NewVersion,Current,NoRelease,NetworkError,InvalidResponse};
struct Result {State state=State::NetworkError;juce::String version,url;};
inline Result interpret(int status,const juce::String& body,const juce::String& installed){
 if(status==404)return {State::NoRelease,{},{}};
 if(status!=200)return {State::NetworkError,{},{}};
 auto data=juce::JSON::parse(body);
 if(!data.isObject()||bool(data["draft"])||bool(data["prerelease"]))return {State::InvalidResponse,{},{}};
 auto tag=data["tag_name"].toString(),url=data["html_url"].toString();
 Version remote,local;
 if(!parseVersion(tag,remote)||!parseVersion(installed,local)||!url.startsWith(releasePrefix)||url.containsAnyOf("?#\r\n"))
  return {State::InvalidResponse,{},{}};
 return {newer(remote,local)?State::NewVersion:State::Current,tag,url};
}
inline Result fetch(const juce::String& installed){
 int status=0;
 auto options=juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
  .withConnectionTimeoutMs(5000)
  .withStatusCode(&status)
  .withExtraHeaders("Accept: application/vnd.github+json\r\nUser-Agent: Mya-Acustica-Analyzer\r\n");
 auto stream=juce::URL(apiUrl).createInputStream(options);
 if(!stream)return interpret(status,{},installed);
 auto body=stream->readEntireStreamAsString();
 if(body.getNumBytesAsUTF8()>256*1024)return {State::InvalidResponse,{},{}};
 return interpret(status,body,installed);
}
class Checker : private juce::Thread {
 std::function<void(Result,bool)> onResult;
 juce::String installed;
 bool manual=false;
 void run() override{
  auto result=fetch(installed);
  auto callback=onResult;
  auto wasManual=manual;
  juce::MessageManager::callAsync([callback,result,wasManual]{callback(result,wasManual);});
 }
public:
 explicit Checker(std::function<void(Result,bool)> callback):juce::Thread("Mya update check"),onResult(std::move(callback)){}
 ~Checker() override{stopThread(7000);}
 bool check(const juce::String& current,bool requestedManually){
  if(isThreadRunning())return false;
  installed=current;manual=requestedManually;return startThread();
 }
};
}
