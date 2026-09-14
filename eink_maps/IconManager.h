#pragma once
#include <Arduino.h>

// Funzione helper per recuperare la bitmap associata a un hash
const uint8_t* getIconBitmap(const String& hash);

// Funzione per recuperare l'icona di splash
const uint8_t* getSplashIcon();