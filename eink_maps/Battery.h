#pragma once
#include <Arduino.h>
#include "Config.h"

// --- GESTIONE BATTERIA ---
inline int getBatteryPercentage() {
  uint32_t pinMv = analogReadMilliVolts(BAT_ADC);
  float batteryVoltage = (pinMv * 2.0) / 1000.0;
  
  if (batteryVoltage >= 4.1) return 100;
  if (batteryVoltage <= 3.0) return 0;
  
  return (int)(((batteryVoltage - 3.0) / (4.1 - 3.0)) * 100);
}