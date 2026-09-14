#include "BLEManager.h"
#include "Config.h"
#include "State.h"
#include "Utils.h"
#include "Battery.h"
#include "DisplayManager.h" // Necessario per chiamare updateDisplay() nel setup iniziale

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

bool isConnected = false;
unsigned long lastDisconnectTime = 0;
String myMacAddress = "";

// --- CALLBACK CONNESSIONE/DISCONNESSIONE BLE ---
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
      isConnected = true;
      if (xSemaphoreTake(stateMutex, (TickType_t)10) == pdTRUE) {
        currentState.distance = String(getBatteryPercentage()) + "%";
        currentState.direction = "Connesso";
        currentState.iconHash = "connected"; 
        currentState.tripInfo = "Buon viaggio!";
        xSemaphoreGive(stateMutex);
        
        if (displayTaskHandle != NULL) {
          xTaskNotifyGive(displayTaskHandle);
        }
      }
    }

    void onDisconnect(BLEServer* pServer) override {
      isConnected = false;
      lastDisconnectTime = millis();      
      if (xSemaphoreTake(stateMutex, (TickType_t)10) == pdTRUE) {
        currentState.distance = String(getBatteryPercentage()) + "%";
        currentState.direction = "Disconnesso";
        currentState.iconHash = "disabled"; 
        currentState.tripInfo = myMacAddress;
        xSemaphoreGive(stateMutex);
        
        if (displayTaskHandle != NULL) {
          xTaskNotifyGive(displayTaskHandle);
        }
      }
      BLEDevice::startAdvertising();
    }
};

// --- CALLBACK RICEZIONE DATI BLE ---
class BLEDataCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      String data = pCharacteristic->getValue();
      
      if (data.length() > 0) {
        // Parsing del payload: "distanza|indicazione|codice icona|info viaggio"
        int p1 = data.indexOf('|');
        int p2 = data.indexOf('|', p1 + 1);
        int p3 = data.indexOf('|', p2 + 1);
        
        if (p1 != -1 && p2 != -1 && p3 != -1) {
          String newDistance = data.substring(0, p1);
          String newDirection = replaceAccents(data.substring(p1 + 1, p2));
          String newIconHash = data.substring(p2 + 1, p3);
          String newTripInfo = data.substring(p3 + 1);

          if (xSemaphoreTake(stateMutex, (TickType_t)10) == pdTRUE) {
            currentState.distance = newDistance;
            currentState.direction = newDirection;
            currentState.iconHash = newIconHash;
            currentState.tripInfo = newTripInfo;
            xSemaphoreGive(stateMutex);
            
            if (displayTaskHandle != NULL) {
              xTaskNotifyGive(displayTaskHandle);
            }
          }
        }
      }
    }
};

// --- SETUP BLE ---
void setupBLE() {
  BLEDevice::init("E-INK_MAPS");
  
  myMacAddress = BLEDevice::getAddress().toString().c_str();
  myMacAddress.toUpperCase();
  
  // Imposta lo stato iniziale
  currentState.distance = String(getBatteryPercentage()) + "%";
  currentState.direction = "Attesa di connessione";
  currentState.iconHash = "searching";
  currentState.tripInfo = myMacAddress;
  
  // Primo aggiornamento forzato
  renderState = currentState;
  updateDisplay(true);
  previousState = renderState;

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );

  pCharacteristic->setCallbacks(new BLEDataCallback());
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}