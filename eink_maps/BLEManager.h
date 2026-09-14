#pragma once
#include <Arduino.h>

extern bool isConnected;
extern unsigned long lastDisconnectTime;
extern String myMacAddress;

void setupBLE();