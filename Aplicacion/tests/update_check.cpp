#include "../src/UpdateCheck.h"
#include "../src/MyaIdentity.h"
#include <cassert>

int main(){
 using namespace MyaUpdate;
 assert(MyaIdentity::version()=="0.3.1");
 Version a,b;
 assert(parseVersion("v0.3.2",a)&&parseVersion("0.3.1",b)&&newer(a,b));
 assert(!parseVersion("v0.3.2-beta",a)&&!parseVersion("0.3",a));
 auto response=[](const char* tag){
  return juce::String("{\"tag_name\":\"")+tag+"\",\"html_url\":\"https://github.com/antoniomanuelruizespinosa/mya-acustica/releases/tag/"+tag+"\",\"draft\":false,\"prerelease\":false}";
 };
 auto latest=interpret(200,response("v0.3.2"),MyaIdentity::version());
 assert(latest.state==State::NewVersion&&latest.version=="v0.3.2"&&latest.url.startsWith(releasePrefix));
 assert(interpret(200,response("v0.3.1"),MyaIdentity::version()).state==State::Current);
 assert(interpret(200,response("v0.3.0"),MyaIdentity::version()).state==State::Current);
 assert(interpret(0,{},MyaIdentity::version()).state==State::NetworkError);
 assert(interpret(503,{},MyaIdentity::version()).state==State::NetworkError);
 assert(interpret(404,{},MyaIdentity::version()).state==State::NoRelease);
 assert(interpret(200,"{bad",MyaIdentity::version()).state==State::InvalidResponse);
 assert(interpret(200,"{\"tag_name\":\"v0.3.2\",\"html_url\":\"https://evil.example/release\"}",MyaIdentity::version()).state==State::InvalidResponse);
}
