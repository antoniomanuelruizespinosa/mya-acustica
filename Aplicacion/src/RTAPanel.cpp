#include "RTAPanel.h"
#include "PlotPresentation.h"
#include <cmath>

RTAPanel::RTAPanel(DataProvider provider) : dataProvider(std::move(provider)) {}

void RTAPanel::setActive(bool active) {
 if (active && !isTimerRunning()) { timerCallback(); startTimerHz(30); }
 else if (!active) stopTimer();
}

void RTAPanel::visibilityChanged() {
 if (!isShowing()) stopTimer();
}

void RTAPanel::timerCallback() {
 if (dataProvider) traces = dataProvider();
 repaint();
}

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
 g.drawText(traces.empty() ? "RTA sin señal: pulsa Play en una TF configurada" : "RTA en directo / dBFS por bin FFT", 10, 7, bounds.getWidth() - 20, 23, Justification::centredLeft);
}
