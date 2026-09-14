#include <Arduino.h>
#include "Config.h"
#include "State.h"
#include "DisplayManager.h"
#include "BLEManager.h"

// --- DEFINIZIONE VARIABILI GLOBALI (dichiarate extern) ---
NavState currentState;
NavState renderState;
NavState previousState;

SemaphoreHandle_t stateMutex = NULL;
TaskHandle_t displayTaskHandle = NULL;

void setup() {
  // Decommentare se serve il debug seriale
  // Serial.begin(115200);

  // Inizializzazione pin di controllo e lettura batteria
  pinMode(BAT_CTRL, OUTPUT);
  digitalWrite(BAT_CTRL, HIGH);
  pinMode(BAT_ADC, INPUT);
  
  delay(500);
  
  // Creazione del semaforo Mutex per accedere in sicurezza allo stato (FreeRTOS)
  stateMutex = xSemaphoreCreateMutex();
  
  // Inizializzazione moduli
  setupDisplay();
  
  // Avvio del Task dedicato all'aggiornamento dell'E-Paper (Core 1)
  xTaskCreatePinnedToCore(
    displayTask,        // Funzione del task (in DisplayManager)
    "Display_Task",     // Nome del task
    8192,               // Dimensione dello Stack
    NULL,               // Parametri del task
    1,                  // Priorità
    &displayTaskHandle, // Handle
    1                   // Esegui sul Core 1
  );

  // Inizializzazione e advertising BLE
  setupBLE();
}

void loop() {
  // Controllo timeout disconnessione: se disconnesso per più di 60s, spegni tutto.
  if (!isConnected && (millis() - lastDisconnectTime > 60000)) {
    
    // Mostra l'icona splash e manda in ibernazione il driver GxEPD2
    showSplashAndHibernate();
    
    delay(100);

    // Togli alimentazione alle periferiche esterne per risparmio energetico
    digitalWrite(EPD_PWR_EN, HIGH); 
    digitalWrite(BAT_CTRL, LOW);
    
    // Entra in deep sleep hardware
    esp_deep_sleep_start();
  }

  // Delay di sistema non bloccante 
  vTaskDelay(pdMS_TO_TICKS(1000)); 
}