#include "OfflineDSPPlot.h"
#include "GraphCursor.h"
#include <algorithm>
#include <cmath>

namespace {
juce::Rectangle<float> frequencyArea(const juce::Component& component){return component.getLocalBounds().withTrimmedLeft(40).withTrimmedRight(8).withTrimmedTop(22).withTrimmedBottom(22).toFloat();}
juce::Rectangle<float> impulseArea(const juce::Component& component){return component.getLocalBounds().withTrimmedLeft(52).withTrimmedRight(8).withTrimmedTop(18).withTrimmedBottom(18).toFloat();}
double minFrequency(){return 20.0;}
double maxFrequency(const OfflineDSPCore* core,int engine){return core&&core->state(engine).loaded?std::min(20000.0,core->state(engine).sampleRate*0.49):20000.0;}
float xFor(double frequency,juce::Rectangle<float> r,double maximum){return r.getX()+r.getWidth()*(float)(std::log(frequency/minFrequency())/std::log(maximum/minFrequency()));}
double frequencyFor(float x,juce::Rectangle<float> r,double maximum){return minFrequency()*std::pow(maximum/minFrequency(),std::clamp((double)(x-r.getX())/r.getWidth(),0.0,1.0));}
float yFor(double db,juce::Rectangle<float> r){return r.getY()+r.getHeight()*(float)((15.0-std::clamp(db,-60.0,15.0))/75.0);}
double dbFor(float y,juce::Rectangle<float> r){return std::clamp(15.0-75.0*(double)(y-r.getY())/r.getHeight(),-15.0,15.0);}
double dbOf(std::complex<double> value){return 20*std::log10(std::max(1.e-12,std::abs(value)));}
}

void OfflineDSPPlot::setPrediction(std::vector<float> source, std::vector<float> processed, double sampleRate) {
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0 || source.empty() || processed.empty()) {
        clear();
        return;
    }
    original = std::move(source);
    prediction = std::move(processed);
    rate = sampleRate;
    impulsePeak=1.0e-9f;
    for(float value:original)impulsePeak=std::max(impulsePeak,std::abs(value));
    for(float value:prediction)impulsePeak=std::max(impulsePeak,std::abs(value));
    cursorVisible=false;
    cursorSample.reset();
    if(onCursorReadout)onCursorReadout({});
    repaint();
}

void OfflineDSPPlot::clear() {
    original.clear();
    prediction.clear();
    rate = 0.0;
    impulsePeak=1.0e-9f;
    cursorVisible=false;
    cursorSample.reset();
    if(onCursorReadout)onCursorReadout({});
    repaint();
}

void OfflineDSPPlot::clearCursor(){cursorVisible=false;cursorSample.reset();if(onCursorReadout)onCursorReadout({});repaint();}

void OfflineDSPPlot::updateCursor(juce::Point<float> point) {
    const auto area=frequencyView?frequencyArea(*this):impulseArea(*this);
    if(!area.contains(point)||area.getWidth()<=0.f){clearCursor();return;}
    CursorSample reading;
    if(frequencyView){
        if(!core||!core->state(engine).loaded){clearCursor();return;}
        reading.frequencyDomain=true;
        reading.x=frequencyFor(point.x,area,maxFrequency(core,engine));
        auto response=core->complexResponse(engine,reading.x);
        if(!std::isfinite(response.real())||!std::isfinite(response.imag())||std::abs(response)<=1.e-12){clearCursor();return;}
        reading.magnitudeDb=dbOf(response);
        reading.phaseDegrees=std::arg(response)*180.0/OfflineDSPFilters::pi;
        cursorY=phaseView?area.getY()+area.getHeight()*(float)((180.0-*reading.phaseDegrees)/360.0):yFor(*reading.magnitudeDb,area);
    }else{
        if(prediction.empty()||rate<=0.0){clearCursor();return;}
        const double sample=(point.x-area.getX())/area.getWidth()*(prediction.size()-1);
        const auto lo=(size_t)std::floor(sample),hi=std::min(lo+1,prediction.size()-1);
        auto interpolated=GraphCursor::interpolate(prediction[lo],prediction[hi],sample-lo);
        if(!interpolated){clearCursor();return;}
        const double value=*interpolated;
        reading.x=sample*1000.0/rate;
        reading.amplitude=value;
        cursorY=area.getCentreY()-(float)(value/impulsePeak)*area.getHeight()*0.45f;
    }
    cursorX=point.x;cursorY=juce::jlimit(area.getY(),area.getBottom()-1.f,cursorY);
    cursorVisible=true;cursorSample=reading;
    if(onCursorReadout)onCursorReadout(cursorSample);
    repaint();
}

void OfflineDSPPlot::mouseMove(const juce::MouseEvent& event){updateCursor(event.position);}
void OfflineDSPPlot::mouseExit(const juce::MouseEvent&){clearCursor();}

void OfflineDSPPlot::mouseDown(const juce::MouseEvent& event){
    dragBand=-1;
    if(!frequencyView||!graphicEQ||!core||!core->state(engine).loaded)return;
    auto r=frequencyArea(*this);const double maximum=maxFrequency(core,engine);
    for(int i=0;i<20;++i){auto b=core->band(engine,i);if(!b.enabled)continue;
        auto pos=juce::Point<float>(xFor(b.frequency,r,maximum),yFor(b.gainDb,r));
        if(pos.getDistanceFrom(event.position)<12){dragBand=i;break;}}
}

void OfflineDSPPlot::mouseDrag(const juce::MouseEvent& event){
    if(dragBand<0||!core||!onBandDragged)return;
    auto r=frequencyArea(*this);auto band=core->band(engine,dragBand);
    band.frequency=std::clamp(frequencyFor(event.position.x,r,maxFrequency(core,engine)),10.0,core->state(engine).sampleRate*0.49-0.01);
    band.gainDb=dbFor(event.position.y,r);
    onBandDragged(engine,dragBand,band);
    repaint();
}

void OfflineDSPPlot::paint(juce::Graphics& g) {
    using namespace juce;
    g.fillAll(Colours::black);
    auto drawCursor=[&](Rectangle<float> area){if(!cursorVisible||!area.contains(cursorX,cursorY))return;g.setColour(Colour(0xff00cdda).withAlpha(.75f));g.drawVerticalLine(roundToInt(cursorX),area.getY(),area.getBottom());g.drawHorizontalLine(roundToInt(cursorY),area.getX(),area.getRight());};
    if(frequencyView){
        auto r=frequencyArea(*this);if(r.isEmpty())return;
        const double maximum=maxFrequency(core,engine);
        g.setColour(Colour(0xff142b38));
        for(double frequency:{20.,31.5,63.,125.,250.,500.,1000.,2000.,4000.,8000.,16000.}){
            if(frequency>maximum)continue;const int x=roundToInt(xFor(frequency,r,maximum));
            g.drawVerticalLine(x,r.getY(),r.getBottom());g.setColour(Colour(0xff26bad4));
            g.drawText(frequency>=1000?String(frequency/1000,frequency<1001?0:1)+"k":String(frequency,frequency>31&&frequency<32?1:0),x-22,roundToInt(r.getBottom())+3,44,14,Justification::centred);
            g.setColour(Colour(0xff142b38));
        }
        for(double axis:phaseView?std::vector<double>{-180.,-90.,0.,90.,180.}:std::vector<double>{-60.,-45.,-30.,-15.,0.,15.}){
            int y=roundToInt(phaseView?r.getY()+r.getHeight()*(float)((180.-axis)/360.):yFor(axis,r));
            g.drawHorizontalLine(y,r.getX(),r.getRight());g.setColour(Colour(0xff26bad4));
            g.drawText(String(axis,0),0,y-7,36,14,Justification::centredRight);g.setColour(Colour(0xff142b38));}
        if(!core||!core->state(engine).loaded){g.setColour(Colours::white);g.drawText(spanishLanguage?"MAGNITUD DSP - SIN IR":"DSP MAGNITUDE - NO IR",r.toNearestInt(),Justification::centred);return;}
        auto trace=[&](Colour colour,float thickness,const std::function<std::optional<std::complex<double>>(double)>& sample){
            Path path;bool started=false;double previousPhase=0;
            for(int i=0;i<160;++i){double f=minFrequency()*std::pow(maximum/minFrequency(),(double)i/159);
                auto value=sample(f);if(!value){started=false;continue;}
                const double phase=std::arg(*value)*180.0/OfflineDSPFilters::pi;
                float x=xFor(f,r,maximum),y=phaseView?r.getY()+r.getHeight()*(float)((180.-phase)/360.):yFor(dbOf(*value),r);
                if(phaseView&&started&&std::abs(phase-previousPhase)>180.)started=false;
                if(!started){path.startNewSubPath(x,y);started=true;}else path.lineTo(x,y);
                previousPhase=phase;
            }
            g.setColour(colour);g.strokePath(path,PathStrokeType(thickness));
        };
        trace(Colour(0xff60e4ff),1.8f,[&](double f){return std::optional<std::complex<double>>(core->eqResponse(engine,f));});
        trace(Colour(0xff67f239),1.5f,[&](double f){return std::optional<std::complex<double>>(core->complexResponse(engine,f));});
        if(sumEnabled){const auto active=core->loadedChannels();
            trace(Colours::white,3.0f,[&](double f){return core->vectorSum(active,f);});}
        if(graphicEQ&&!phaseView){g.setFont(10.f);for(int i=0;i<20;++i){auto b=core->band(engine,i);if(!b.enabled||b.frequency>maximum)continue;
            const float x=xFor(b.frequency,r,maximum),y=yFor(b.gainDb,r);g.setColour(Colour(0xff60e4ff));g.fillEllipse(x-9,y-9,18,18);g.setColour(Colours::black);g.drawText(String(i+1),roundToInt(x)-9,roundToInt(y)-8,18,16,Justification::centred);}}
        g.setColour(Colours::white);g.setFont(11.f);g.drawText(spanishLanguage?(phaseView?juce::String::fromUTF8("FASE DSP · EQ azul / vía verde / suma blanca"):juce::String::fromUTF8("MAGNITUD DSP · EQ azul / vía verde / suma blanca")):(phaseView?juce::String::fromUTF8("DSP PHASE · blue EQ / green channel / white sum"):juce::String::fromUTF8("DSP MAGNITUDE · blue EQ / green channel / white sum")),40,2,getWidth()-80,18,Justification::centredLeft);
        drawCursor(r);
        return;
    }
    auto plot = impulseArea(*this);
    if (plot.isEmpty()) return;
    g.setColour(Colour(0xff142b38));
    for (int i = 0; i <= 8; ++i) {
        const float x = plot.getX() + plot.getWidth() * (float)i / 8.0f;
        g.drawVerticalLine(roundToInt(x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 4; ++i) {
        if(plot.getHeight()<100.f&&i%2!=0)continue;
        const int percent = 100 - i * 50;
        const float y = plot.getCentreY() - (float)percent * plot.getHeight() * 0.0045f;
        g.drawHorizontalLine(roundToInt(y), plot.getX(), plot.getRight());
        g.setColour(Colour(0xff26bad4));g.setFont(10.0f);
        g.drawFittedText((percent>0?String("+"):String{})+String(percent)+"%",20,roundToInt(y)-7,42,14,Justification::centredRight,1);
        g.setColour(Colour(0xff142b38));
    }
    g.setColour(Colour(0xff26bad4));g.setFont(10.0f);
    const String axis="Level (%)";const int step=plot.getHeight()<100.f?8:10;const int top=roundToInt(plot.getCentreY())-axis.length()*step/2;g.setFont(plot.getHeight()<100.f?8.f:10.f);for(int n=0;n<axis.length();++n)g.drawFittedText(axis.substring(n,n+1),2,top+n*step,12,step,Justification::centred,1);
    g.setColour(Colour(0xffdcebf0));
    g.setFont(12.0f);
    if (prediction.empty() || rate <= 0.0) {
        g.drawText(spanishLanguage?juce::String::fromUTF8("IR - SIN MEDICIÓN"):"IR - NO MEASUREMENT", plot.toNearestInt(), Justification::centred);
        return;
    }
    const auto count = prediction.size();
    auto draw = [&](const std::vector<float>& samples, Colour colour) {
        Path path;
        for (std::size_t i = 0; i < count; ++i) {
            const float value = i < samples.size() ? samples[i] : 0.0f;
            const float x = plot.getX() + plot.getWidth() * (float)i / (float)std::max<std::size_t>(1, count - 1);
            const float y = plot.getCentreY() - (value / impulsePeak) * plot.getHeight() * 0.45f;
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(colour);
        g.strokePath(path, PathStrokeType(1.5f));
    };
    draw(original, Colour(0xff19d7ea));
    draw(prediction, Colour(0xff6df23d));
    g.setColour(Colour(0xffdcebf0));
    g.drawText((spanishLanguage?juce::String::fromUTF8("IR en frío · "):juce::String::fromUTF8("Offline IR · ")) + String((double)count * 1000.0 / rate, 2) + " ms", 34, 1, getWidth() - 68, 16, Justification::centredLeft);
    drawCursor(plot);
}
