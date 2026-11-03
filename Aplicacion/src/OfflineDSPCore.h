#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <complex>
#include <optional>
#include <unordered_map>
#include <vector>
#include "OfflineDSPFilters.h"

// Offline-only transformation of a local impulse response. No audio device or
// callback is referenced here; the loaded source remains unchanged.
class OfflineDSPCore {
public:
    static constexpr int channelCount = 8;
    using Crossover = OfflineDSPFilters::Crossover;
    using EQBand = OfflineDSPFilters::EQBand;
    using AllPass = OfflineDSPFilters::AllPass;
    using Chingonizer = OfflineDSPFilters::Chingonizer;
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
        item.sourceSpectrumCache.clear();
        item.state.sampleRate = sampleRate;
        item.state.capacity = capacity;
        item.state.loaded = true;
        item.state.delaySamples = std::min(item.state.delaySamples, capacity - 1);
        updateDelay(item.state);
        if (!OfflineDSPFilters::valid(item.highPass,sampleRate)) item.highPass={};
        if (!OfflineDSPFilters::valid(item.lowPass,sampleRate)) item.lowPass={};
        for(auto& band:item.bands)if(band.enabled&&!OfflineDSPFilters::valid(band,sampleRate))band.enabled=false;
        for(auto& ap:item.allPass)if(ap.enabled&&!OfflineDSPFilters::valid(ap,sampleRate))ap.enabled=false;
        if(item.chingonizer.enabled&&!OfflineDSPFilters::valid(item.chingonizer,sampleRate))item.chingonizer.enabled=false;
        rebuild(item);
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

    bool setCrossover(int channel, bool highPass, Crossover value) {
        if (!valid(channel)) return false;
        auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded || !OfflineDSPFilters::valid(value,item.state.sampleRate)) return false;
        auto sections=OfflineDSPFilters::crossover(value,highPass,item.state.sampleRate);
        if ((value.family!=OfflineDSPFilters::Family::None && sections.empty()) || !OfflineDSPFilters::allStable(sections)) return false;
        (highPass?item.highPass:item.lowPass)=value;
        rebuild(item);
        return true;
    }
    Crossover crossover(int channel,bool highPass) const {
        if (!valid(channel)) return {};
        const auto& item=channels[(std::size_t)channel];return highPass?item.highPass:item.lowPass;
    }
    bool setBand(int channel,int band,EQBand value) {
        if (!valid(channel)||band<0||band>=20)return false;
        auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded||!OfflineDSPFilters::valid(value,item.state.sampleRate)||
            !OfflineDSPFilters::eq(value,item.state.sampleRate).stable())return false;
        item.bands[(std::size_t)band]=value;rebuild(item);return true;
    }
    EQBand band(int channel,int index) const {
        return valid(channel)&&index>=0&&index<20?channels[(std::size_t)channel].bands[(std::size_t)index]:EQBand{};
    }
    bool setAllPass(int channel,int index,AllPass value) {
        if (!valid(channel)||index<0||index>=2)return false;
        auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded||!OfflineDSPFilters::valid(value,item.state.sampleRate)||
            !OfflineDSPFilters::allPass(value,item.state.sampleRate).stable())return false;
        item.allPass[(std::size_t)index]=value;rebuild(item);return true;
    }
    AllPass allPass(int channel,int index) const {
        return valid(channel)&&index>=0&&index<2?channels[(std::size_t)channel].allPass[(std::size_t)index]:AllPass{};
    }
    bool setChingonizer(int channel,Chingonizer value) {
        if (!valid(channel))return false;
        auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded||!OfflineDSPFilters::valid(value,item.state.sampleRate))return false;
        auto low=OfflineDSPFilters::eq({true,OfflineDSPFilters::EQType::LowShelf,value.lowFrequency,value.lowGainDb,0.7071067811865476},item.state.sampleRate);
        auto high=OfflineDSPFilters::eq({true,OfflineDSPFilters::EQType::HighShelf,value.highFrequency,value.highGainDb,0.7071067811865476},item.state.sampleRate);
        if (!low.stable()||!high.stable())return false;
        item.chingonizer=value;rebuild(item);return true;
    }
    Chingonizer chingonizer(int channel) const {return valid(channel)?channels[(std::size_t)channel].chingonizer:Chingonizer{};}
    std::complex<double> filterResponse(int channel,double frequency) const {
        if (!valid(channel))return {0,0};const auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded||frequency<0||frequency>item.state.sampleRate*0.5)return {0,0};
        return OfflineDSPFilters::response(item.sections,frequency,item.state.sampleRate);
    }
    std::complex<double> eqResponse(int channel,double frequency) const {
        if (!valid(channel))return {0,0};const auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded||frequency<0||frequency>item.state.sampleRate*0.5)return {0,0};
        return OfflineDSPFilters::response(item.eqSections,frequency,item.state.sampleRate);
    }
    double groupDelayMs(int channel,double frequency) const {
        if (!valid(channel)||!state(channel).loaded)return 0;
        const double fs=state(channel).sampleRate,step=std::max(0.01,frequency*0.0001);
        const double a=std::max(0.0,frequency-step),b=std::min(fs*0.5,frequency+step);
        if (b<=a)return 0;
        const auto phase=std::arg(filterResponse(channel,b)*std::conj(filterResponse(channel,a)));
        return -phase/(2*OfflineDSPFilters::pi*(b-a))*1000.0+state(channel).delayMs;
    }
    std::complex<double> complexResponse(int channel,double frequency) const {
        if (!valid(channel))return {0,0};const auto& item=channels[(std::size_t)channel];
        if (!item.state.loaded||frequency<0||frequency>item.state.sampleRate*0.5)return {0,0};
        auto cached=item.sourceSpectrumCache.find(frequency);
        std::complex<double> sourceResponse{0,0};
        if(cached!=item.sourceSpectrumCache.end())sourceResponse=cached->second;
        else{
            for(std::size_t i=0;i<item.source.size();++i)
                sourceResponse+=static_cast<double>(item.source[i])*std::polar(1.0,-2*OfflineDSPFilters::pi*frequency*(double)i/item.state.sampleRate);
            if(item.sourceSpectrumCache.size()>2048)item.sourceSpectrumCache.clear();
            item.sourceSpectrumCache.emplace(frequency,sourceResponse);
        }
        const double scale=std::pow(10.0,item.state.gainDb/20.0)*(item.state.inverted?-1.0:1.0);
        return sourceResponse*filterResponse(channel,frequency)*scale*
            std::polar(1.0,-2*OfflineDSPFilters::pi*frequency*(double)item.state.delaySamples/item.state.sampleRate);
    }
    std::optional<std::complex<double>> vectorSum(const std::vector<int>& active,double frequency) const {
        double fs=0;std::complex<double> total{0,0};bool any=false;
        for(int channel:active){
            if(!valid(channel)||!state(channel).loaded)return std::nullopt;
            const double rate=state(channel).sampleRate;
            if(any&&std::abs(rate-fs)>1.e-6)return std::nullopt;
            fs=rate;any=true;total+=complexResponse(channel,frequency);
        }
        return any?std::optional<std::complex<double>>(total):std::nullopt;
    }
    std::vector<int> loadedChannels() const {
        std::vector<int> active;for(int i=0;i<channelCount;++i)if(state(i).loaded)active.push_back(i);return active;
    }

    std::vector<float> render(int channel) const {
        if (!valid(channel)) return {};
        const auto& item = channels[(std::size_t) channel];
        if (!item.state.loaded) return {};
        std::vector<float> output(item.state.capacity, 0.0f);
        const float scale = static_cast<float>(std::pow(10.0, item.state.gainDb / 20.0)
                                               * (item.state.inverted ? -1.0 : 1.0));
        const auto available = item.state.capacity - item.state.delaySamples;
        std::vector<float> filtered(available,0.0f);
        std::copy_n(item.source.begin(),std::min(item.source.size(),available),filtered.begin());
        for(const auto& section:item.sections){
            double x1=0,x2=0,y1=0,y2=0;
            for(auto& sample:filtered){const double x=sample,y=section.b0*x+section.b1*x1+section.b2*x2-section.a1*y1-section.a2*y2;
                sample=(float)y;x2=x1;x1=x;y2=y1;y1=y;}
        }
        for (std::size_t i = 0; i < filtered.size(); ++i)
            output[i + item.state.delaySamples] = filtered[i] * scale;
        return output;
    }

private:
    struct Channel { State state; std::vector<float> source; Crossover highPass,lowPass;
        std::array<EQBand,20> bands{};std::array<AllPass,2> allPass{};Chingonizer chingonizer;
        std::vector<OfflineDSPFilters::Section> sections,eqSections;
        mutable std::unordered_map<double,std::complex<double>> sourceSpectrumCache;};
    std::array<Channel, channelCount> channels{};
    static bool valid(int channel) { return channel >= 0 && channel < channelCount; }
    static void updateDelay(State& state) {
        state.delayMs = static_cast<double>(state.delaySamples) * 1000.0 / state.sampleRate;
    }
    static void rebuild(Channel& item){
        const double fs=item.state.sampleRate;
        item.sections=OfflineDSPFilters::crossover(item.highPass,true,fs);
        auto low=OfflineDSPFilters::crossover(item.lowPass,false,fs);
        item.sections.insert(item.sections.end(),low.begin(),low.end());
        item.eqSections.clear();
        for(const auto& band:item.bands)if(band.enabled)item.eqSections.push_back(OfflineDSPFilters::eq(band,fs));
        for(const auto& ap:item.allPass)if(ap.enabled)item.eqSections.push_back(OfflineDSPFilters::allPass(ap,fs));
        if(item.chingonizer.enabled){const auto& c=item.chingonizer;
            item.eqSections.push_back(OfflineDSPFilters::eq({true,OfflineDSPFilters::EQType::LowShelf,c.lowFrequency,c.lowGainDb,0.7071067811865476},fs));
            item.eqSections.push_back(OfflineDSPFilters::eq({true,OfflineDSPFilters::EQType::HighShelf,c.highFrequency,c.highGainDb,0.7071067811865476},fs));}
        item.sections.insert(item.sections.end(),item.eqSections.begin(),item.eqSections.end());
    }
};
