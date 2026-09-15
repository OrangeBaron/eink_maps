#include <Arduino.h>
#include "Config.h"
#include "State.h"
#include "DisplayManager.h"
#include "BLEManager.h"

// --- DEFINIZIONE VARIABILI GLOBALI ---
NavState currentState;
NavState renderState;
NavState previousState;

SemaphoreHandle_t stateMutex = NULL;
TaskHandle_t displayTaskHandle = NULL;

void setup() {
  // Serial.begin(115200);

  pinMode(BAT_CTRL, OUTPUT);
  digitalWrite(BAT_CTRL, HIGH);
  pinMode(BAT_ADC, INPUT);
  delay(500);

  stateMutex = xSemaphoreCreateMutex();

  setupDisplay();

  xTaskCreatePinnedToCore(
    displayTask,        // Funzione del task (in DisplayManager)
    "Display_Task",     // Nome del task
    8192,               // Dimensione dello Stack
    NULL,               // Parametri del task
    1,                  // Priorita'
    &displayTaskHandle, // Handle
    1                   // Esegui sul Core 1
  );

  setupBLE();
}

void loop() {
  if (!isConnected && (millis() - lastDisconnectTime > DISCONNECT_TIMEOUT_MS)) {
    
    showSplashAndHibernate();
    
    delay(100);
    
    digitalWrite(EPD_PWR_EN, HIGH); 
    digitalWrite(BAT_CTRL, LOW);
    
    esp_deep_sleep_start();
  }

  vTaskDelay(pdMS_TO_TICKS(1000));
}