#pragma once
#include <Arduino.h>
#include "Config.h"

// --- SOGLIE VOLTAGGIO ---
constexpr float BATT_VOLTAGE_MAX = 4.0;
constexpr float BATT_VOLTAGE_MIN = 3.2;

// --- GESTIONE BATTERIA ---
inline int getBatteryPercentage() {
  uint32_t pinMv = analogReadMilliVolts(BAT_ADC);
  float batteryVoltage = (pinMv * 2.0) / 1000.0;

  if (batteryVoltage >= BATT_VOLTAGE_MAX) return 100;
  if (batteryVoltage <= BATT_VOLTAGE_MIN) return 0;

  return (int)(((batteryVoltage - BATT_VOLTAGE_MIN) / (BATT_VOLTAGE_MAX - BATT_VOLTAGE_MIN)) * 100);
}