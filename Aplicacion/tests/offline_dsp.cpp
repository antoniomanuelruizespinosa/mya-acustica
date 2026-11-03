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
}
