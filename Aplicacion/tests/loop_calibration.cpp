#include "../src/LoopCalibrationCore.h"
#include "../src/LoopPink.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main(){
 constexpr int lag=317;
 LoopPink pink;
 std::vector<float> output(LoopCalibrationCore::Capture::required),input(output.size());
 for(auto& sample:output)sample=pink.next();
 for(size_t i=lag;i<input.size();++i)input[i]=output[i-lag]*.5f;
 LoopCalibrationCore::Capture capture;
 for(size_t at=0;at<output.size();at+=512){auto count=(int)std::min<size_t>(512,output.size()-at);assert(capture.feed(output.data()+at,input.data()+at,count,48000));}
 assert(capture.ready());auto result=capture.calculate();
 assert(result&&std::abs(result->delayMs-1000.*lag/48000)<.03&&std::abs(result->gainDb-6.0206)<.05&&result->correlation>.99);
 capture.reset();std::fill(input.begin(),input.end(),0.f);assert(capture.feed(output.data(),input.data(),(int)output.size(),48000));assert(!capture.calculate());
 capture.reset();input[200]=1.f;assert(!capture.feed(output.data(),input.data(),(int)output.size(),48000));assert(capture.failed());
 capture.reset();input[200]=0.f;assert(capture.feed(output.data(),input.data(),512,48000));assert(!capture.feed(output.data()+512,input.data()+512,512,44100));assert(capture.failed());
 std::cout<<"PASS loop delay/gain, silence, clipping, rate discontinuity\n";
}
