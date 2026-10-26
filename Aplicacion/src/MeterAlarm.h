#pragma once
#include "MeterState.h"
#include <optional>
class MeterAlarm {
public:
 enum class Level { None,Warning,Alarm };
 struct Reading {double value=0;int metric=0,weight=0;bool acquisitionValid=false,calibrated=false;juce::String deviceKey;int channel=0;};
 static Level evaluate(const MeterState::Card& c,const std::optional<Reading>& reading,Level previous=Level::None){
  if(!c.alarmEnabled||!reading||!reading->acquisitionValid||!std::isfinite(reading->value)||reading->deviceKey!=c.deviceKey||reading->channel!=c.channel||c.deviceKey.isEmpty()||c.channel<=0||reading->metric!=c.metric||reading->weight!=c.weight||c.alarmMetric!=c.metric||c.alarmWeight!=c.weight||(c.metric!=1&&!reading->calibrated))return Level::None;
  double value=reading->value;if(value>=c.alarm)return Level::Alarm;
  if(previous==Level::Alarm&&value>=c.alarm-c.hysteresis)return Level::Alarm;
  if(value>=c.warning)return Level::Warning;
  if(previous==Level::Warning&&value>=c.warning-c.hysteresis)return Level::Warning;
  return Level::None;
 }
};
