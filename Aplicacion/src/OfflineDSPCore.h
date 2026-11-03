#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

// Offline-only transformation of a local impulse response. No audio device or
// callback is referenced here; the loaded source remains unchanged.
class OfflineDSPCore {
public:
    static constexpr int channelCount = 8;
    struct State {
        double gainDb = 0.0;
        bool inverted = false;
        double delayMs = 0.0;
        std::size_t delaySamples = 0;
        double sampleRate = 0.0;
        std::size_t capacity = 0;
        bool loaded = false;
    };

    bool load(int channel, std::vector<float> impulse, double sampleRate,
              std::size_t capacity = 0) {
        if (!valid(channel) || impulse.empty() || !std::isfinite(sampleRate) || sampleRate <= 0.0
            || !std::all_of(impulse.begin(), impulse.end(), [](float x) { return std::isfinite(x); }))
            return false;
        if (capacity == 0) capacity = impulse.size();
        if (capacity < impulse.size()) return false;
        auto& item = channels[(std::size_t) channel];
        item.source = std::move(impulse);
        item.state.sampleRate = sampleRate;
        item.state.capacity = capacity;
        item.state.loaded = true;
        item.state.delaySamples = std::min(item.state.delaySamples, capacity - 1);
        updateDelay(item.state);
        return true;
    }

    void unload(int channel) {
        if (!valid(channel)) return;
        channels[(std::size_t) channel] = {};
    }

    bool setGainDb(int channel, double gainDb) {
        if (!valid(channel) || !std::isfinite(gainDb)) return false;
        channels[(std::size_t) channel].state.gainDb =
            std::clamp(std::round(gainDb * 2.0) / 2.0, -30.0, 30.0);
        return true;
    }

    bool setPolarity(int channel, bool inverted) {
        if (!valid(channel)) return false;
        channels[(std::size_t) channel].state.inverted = inverted;
        return true;
    }

    bool setDelaySamples(int channel, std::size_t samples) {
        if (!valid(channel)) return false;
        auto& state = channels[(std::size_t) channel].state;
        if (!state.loaded) return false;
        state.delaySamples = std::min(samples, state.capacity - 1);
        updateDelay(state);
        return true;
    }

    bool setDelayMs(int channel, double milliseconds) {
        if (!valid(channel) || !std::isfinite(milliseconds) || milliseconds < 0.0) return false;
        const auto& state = channels[(std::size_t) channel].state;
        if (!state.loaded) return false;
        const auto samples = static_cast<std::size_t>(std::min(
            std::round(milliseconds * state.sampleRate / 1000.0),
            static_cast<double>(state.capacity - 1)));
        return setDelaySamples(channel, samples);
    }

    State state(int channel) const {
        return valid(channel) ? channels[(std::size_t) channel].state : State{};
    }

    const std::vector<float>& source(int channel) const {
        static const std::vector<float> empty;
        return valid(channel) ? channels[(std::size_t) channel].source : empty;
    }

    std::vector<float> render(int channel) const {
        if (!valid(channel)) return {};
        const auto& item = channels[(std::size_t) channel];
        if (!item.state.loaded) return {};
        std::vector<float> output(item.state.capacity, 0.0f);
        const float scale = static_cast<float>(std::pow(10.0, item.state.gainDb / 20.0)
                                               * (item.state.inverted ? -1.0 : 1.0));
        const auto available = item.state.capacity - item.state.delaySamples;
        for (std::size_t i = 0; i < std::min(item.source.size(), available); ++i)
            output[i + item.state.delaySamples] = item.source[i] * scale;
        return output;
    }

private:
    struct Channel { State state; std::vector<float> source; };
    std::array<Channel, channelCount> channels{};
    static bool valid(int channel) { return channel >= 0 && channel < channelCount; }
    static void updateDelay(State& state) {
        state.delayMs = static_cast<double>(state.delaySamples) * 1000.0 / state.sampleRate;
    }
};
