#include "RTAPanel.h"
#include "PlotPresentation.h"
#include <cmath>

RTAPanel::RTAPanel(DataProvider provider) : dataProvider(std::move(provider)) {
 speed.addItemList({"Fast","Slow","Prom.16"},1);speed.setSelectedId(1);speed.setTooltip("Respuesta temporal de la curva RTA");speed.onChange=[this]{history.clear();lastUpdateMs=0;};addAndMakeVisible(speed);
}

void RTAPanel::setActive(bool active) {
 if (active && !isTimerRunning()) { timerCallback(); startTimerHz(30); }
 else if (!active) {stopTimer();traces.clear();history.clear();lastUpdateMs=0;repaint();}
}

void RTAPanel::visibilityChanged() {
 if (!isShowing()) stopTimer();
}

void RTAPanel::timerCallback() {
 auto incoming=dataProvider?dataProvider():std::vector<Trace>{};
 double now=juce::Time::getMillisecondCounterHiRes(),dt=lastUpdateMs>0?juce::jlimit(0.0,1.0,(now-lastUpdateMs)/1000.):0.;lastUpdateMs=now;
 double tau=speed.getSelectedId()==2?1.:speed.getSelectedId()==3?5.4:.125;
 double alpha=dt>0?1.-std::exp(-dt/tau):1.;
 std::map<int,std::vector<float>> next;
 for(auto& trace:incoming){auto& values=next[trace.id];values=trace.dbfs;auto old=history.find(trace.id);if(old!=history.end()&&old->second.size()==values.size())for(size_t i=0;i<values.size();++i)if(std::isfinite(values[i])&&std::isfinite(old->second[i]))values[i]=(float)(old->second[i]+alpha*(values[i]-old->second[i]));trace.dbfs=values;}
 history=std::move(next);traces=std::move(incoming);
 repaint();
}

void RTAPanel::resized(){speed.setBounds(juce::jmax(8,getWidth()-145),8,130,23);}

void RTAPanel::paint(juce::Graphics& g) {
 using namespace juce;
 const Colour plot(0xff080a0c), line(0xff626e78), muted(0xffa9bac8), green(0xff54ee22), orange(0xffffac24);
 auto bounds = getLocalBounds(); g.fillAll(plot);
 auto p = bounds.reduced(55, 42); if (p.isEmpty()) return;
 g.setColour(line); g.drawRect(p);
 for (int db = -120; db <= 0; db += 20) {
  float y = (float)(p.getBottom() - (db + 120.) / 120. * p.getHeight());
  g.setColour(line.withAlpha(.55f)); g.drawHorizontalLine((int)y, (float)p.getX(), (float)p.getRight());
  g.setColour(muted); g.setFont(10.f); g.drawText(String(db), 5, (int)y - 9, 43, 18, Justification::centredLeft);
 }
 for (double hz : PlotPresentation::frequencyTicks) {
  float x = (float)(p.getX() + std::log(hz / 20.) / std::log(1000.) * p.getWidth());
  g.setColour(line.withAlpha(.45f)); g.drawVerticalLine((int)x, (float)p.getY(), (float)p.getBottom());
  g.setColour(muted); g.setFont(10.f);
  g.drawText(hz >= 1000 ? String(hz / 1000., hz == 1000 ? 0 : 1) + "k" : String(hz, 0), (int)x - 24, p.getBottom() + 5, 48, 18, Justification::centred);
 }
 for (const auto& trace : traces) {
  if (trace.dbfs.empty()) continue;
  Path path; bool started = false;
  for (size_t i = 0; i < trace.dbfs.size(); ++i) {
   if (!std::isfinite(trace.dbfs[i])) { started = false; continue; }
   float x = (float)p.getX() + (float)i / (float)(trace.dbfs.size() - 1) * (float)p.getWidth();
   float y = (float)(p.getBottom() - jlimit(0.f, 120.f, trace.dbfs[i] + 120.f) / 120.f * p.getHeight());
   if (!started) { path.startNewSubPath(x, y); started = true; } else path.lineTo(x, y);
  }
  g.setColour(trace.colour); g.strokePath(path, PathStrokeType(trace.selected ? 2.2f : 1.3f));
 }
 g.setColour(traces.empty() ? orange : green); g.setFont(13.f);
 g.drawText(traces.empty() ? "RTA sin señal: pulsa Play en una TF configurada" : "RTA en directo / 64 FPPO", 10, 7, bounds.getWidth() - 160, 23, Justification::centredLeft);
}
