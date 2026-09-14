#pragma once
#include <Arduino.h>

void setupDisplay();
void displayTask(void *pvParameters);
void showSplashAndHibernate();
void updateDisplay(bool fullUpdate);