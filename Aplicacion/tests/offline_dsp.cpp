#include "../src/OfflineDSPCore.h"
#include <cassert>
#include <cmath>
#include <vector>

static bool near(double a, double b, double tolerance = 1.0e-6) {
    return std::abs(a - b) < tolerance;
}

int main() {
    OfflineDSPCore core;
    assert(!core.load(-1, {1.0f}, 48000.0));
    assert(!core.load(0, {}, 48000.0));
    assert(!core.load(0, {1.0f}, 0.0));
    assert(!core.load(0, {1.0f, 0.0f}, 48000.0, 1));
    for (int i = 0; i < OfflineDSPCore::channelCount; ++i)
        assert(core.load(i, {1.0f, 0.5f}, 48000.0, 5));
    assert(core.setGainDb(0, 6.26));
    assert(near(core.state(0).gainDb, 6.5));
    assert(core.setGainDb(0, 100.0));
    assert(near(core.state(0).gainDb, 30.0));
    assert(core.setGainDb(0, -100.0));
    assert(near(core.state(0).gainDb, -30.0));
    assert(core.setGainDb(0, 0.0));
    assert(core.setPolarity(0, true));
    assert(core.setDelayMs(0, 1000.0 / 48000.0));
    assert(core.state(0).delaySamples == 1);
    assert(near(core.state(0).delayMs, 1000.0 / 48000.0));
    const auto result = core.render(0);
    assert(result.size() == 5);
    assert(result[0] == 0.0f && result[1] == -1.0f && result[2] == -0.5f);
    assert(result[3] == 0.0f && result[4] == 0.0f);
    assert(core.source(0)[0] == 1.0f);
    for (int i = 1; i < OfflineDSPCore::channelCount; ++i) {
        assert(!core.state(i).inverted && core.state(i).delaySamples == 0);
        assert(core.render(i)[0] == 1.0f);
    }
    assert(core.setDelaySamples(0, 100));
    assert(core.state(0).delaySamples == 4);
    assert(core.render(0)[0] == 0.0f && core.render(0)[4] == -1.0f);
    core.unload(0);
    assert(!core.state(0).loaded && core.render(0).empty());

    using namespace OfflineDSPFilters;
    for (auto family : {Family::Butterworth, Family::Bessel, Family::LinkwitzRiley}) {
        for (int order=1;order<=8;++order) {
            if (family==Family::LinkwitzRiley && order%2) continue;
            Crossover x{family,order,1000};
            assert(core.setCrossover(1,false,x));
            assert(core.setCrossover(1,true,x));
            auto low=crossover(x,false,48000.0),high=crossover(x,true,48000.0);
            assert(allStable(low) && allStable(high));
            assert(std::abs(response(low,0,48000.0))>0.99);
            assert(std::abs(response(high,24000,48000.0))>0.99);
            if (family==Family::Bessel){
                assert(near(std::abs(response(low,1000,48000.0)),std::sqrt(0.5),0.02));
                assert(near(std::abs(response(high,1000,48000.0)),std::sqrt(0.5),0.02));
            }
        }
    }
    assert(!core.setCrossover(1,false,{Family::LinkwitzRiley,3,1000}));
    auto lrSix=crossover({Family::LinkwitzRiley,6,1000},false,48000.0);
    assert(lrSix.size()==4);
    assert(near(std::abs(response(lrSix,1000,48000.0)),0.5,0.001));
    assert(!core.setCrossover(1,false,{Family::Butterworth,2,30000}));
    assert(core.setCrossover(1,false,{Family::None,2,1000}));
    assert(core.setCrossover(1,true,{Family::None,2,1000}));
    assert(core.load(4,{1.0f},48000.0,64));
    assert(core.setCrossover(4,false,{Family::Butterworth,2,1000}));
    const auto filteredImpulse=core.render(4);
    assert(std::abs(filteredImpulse[1])>1.e-6f);
    assert(std::abs(filteredImpulse[10])>1.e-6f);
    assert(std::isfinite(core.groupDelayMs(4,1000)));
    assert(core.setBand(1,0,{true,EQType::Bell,1000,6,2}));
    assert(std::abs(core.eqResponse(1,1000))>1.5);
    assert(!core.setBand(1,0,{true,EQType::Bell,1000,16,2}));
    for(int i=0;i<20;++i)assert(core.setBand(1,i,{false,EQType::Bell,1000,0,128}));
    assert(core.setAllPass(1,0,{true,1,1000,0.7}));
    assert(core.setAllPass(1,1,{true,2,1000,0.7}));
    assert(near(std::abs(core.eqResponse(1,3000)),1.0,0.001));
    assert(core.setChingonizer(1,{true,100,8000,3,-3}));
    assert(std::isfinite(core.groupDelayMs(1,1000)));
    assert(core.setAllPass(1,0,{false,1,1000,0.7}));
    assert(core.setAllPass(1,1,{false,2,1000,0.7}));
    assert(core.setChingonizer(1,{false,100,8000,0,0}));
    assert(core.setPolarity(1,true));
    assert(core.load(2,{1.0f,0.5f},48000.0,5));
    assert(core.setPolarity(2,false));
    auto cancelled=core.vectorSum({1,2},0);
    assert(cancelled && near(std::abs(*cancelled),0));
    auto cancelledAtFrequency=core.vectorSum({1,2},1379);
    assert(cancelledAtFrequency && near(std::abs(*cancelledAtFrequency),0));
    assert(core.load(3,{1.0f},44100.0,3));
    assert(!core.vectorSum({1,3},1000));
}
