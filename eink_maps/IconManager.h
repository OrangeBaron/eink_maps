#pragma once
#include <Arduino.h>

const uint8_t* getIconBitmap(const String& hash);
const uint8_t* getSplashIcon();
const uint8_t* getBatteryIcon(int percentage);