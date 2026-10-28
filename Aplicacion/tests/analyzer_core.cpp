#include "../src/DelayFinderCore.h"
#include "../src/SpectrumCore.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>

int main(){
 constexpr double rate=48000.;constexpr int delay=731;
 DelayFinderCore::Finder finder;finder.configure(.5);std::mt19937 rng(42);std::uniform_real_distribution<float> noise(-.5f,.5f);std::vector<float> reference(30000),measurement(30000);
 for(auto& v:reference)v=noise(rng);for(size_t i=delay;i<measurement.size();++i)measurement[i]=reference[i-delay];
 for(size_t at=0;at<reference.size();at+=512){TransferFunctionCore::RawBlock b;b.sequence=at/512+1;b.sampleRate=rate;b.count=(int)std::min<size_t>(512,reference.size()-at);for(int i=0;i<b.count;++i){b.ref[(size_t)i]=reference[at+i];b.mic[(size_t)i]=measurement[at+i];}finder.feed(b);}
 auto found=finder.calculate(100.);assert(found.valid);assert(std::abs(found.samples-delay)<=1);assert(std::abs(found.milliseconds-delay*1000./rate)<.03);
 SpectrumCore::Analyzer spectrum;SpectrumCore::Settings settings;settings.fftSize=32768;settings.framesPerSecond=20;settings.average=SpectrumCore::Average::None;spectrum.configure(settings);std::vector<float> tone(96000);for(size_t i=0;i<tone.size();++i)tone[i]=(float)(.25*std::sin(2*3.14159265358979323846*1000*i/rate));spectrum.feed(tone.data(),(int)tone.size(),rate);assert(spectrum.ready());auto one=spectrum.at(1000),quiet=spectrum.at(200);assert(one&&quiet&&*one>-14&&*one< -10&&*quiet<*one-50);assert(!spectrum.spectrogram().empty());
 std::cout<<"PASS real delay finder and configurable spectrum core\n";
}
