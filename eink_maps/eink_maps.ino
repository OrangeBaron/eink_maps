#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "icons.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Definizione Pin SPI per WaveShare ESP32-S3-ePaper-1.54 V2
#define EPD_CS       11
#define EPD_DC       10
#define EPD_RST      9
#define EPD_BUSY     8
#define EPD_SCLK     12
#define EPD_MOSI     13
#define EPD_PWR_EN   6

// UUID configurati su Tasker
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- COSTANTI DI LAYOUT ---
constexpr uint16_t DIST_BOX_X = 100;
constexpr uint16_t DIST_BOX_Y = 0;
constexpr uint16_t DIST_BOX_W = 100;
constexpr uint16_t DIST_BOX_H = 100;
constexpr uint16_t DIST_CURSOR_X = 110;
constexpr uint16_t DIST_CURSOR_Y = 55;

constexpr uint16_t DIR_CURSOR_X = 10;
constexpr uint16_t DIR_CURSOR_Y = 125;
constexpr uint16_t DIR_LINE_HEIGHT = 20;

constexpr uint16_t TRIP_BOX_X = 0;
constexpr uint16_t TRIP_BOX_Y = 166;
constexpr uint16_t TRIP_BOX_W = 200;
constexpr uint16_t TRIP_BOX_H = 34;
constexpr uint16_t TRIP_CURSOR_X = 10;
constexpr uint16_t TRIP_CURSOR_Y = 188;

// Inizializza il display e-paper 1.54" con driver SSD1681 (GDEH0154D67)
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// Struttura per gestire lo stato dell'interfaccia
struct NavState {
  String distance = "Pronto";
  String direction = "Attesa di connessione";
  String iconHash = "searching";
  String tripInfo = "Avvio in corso...";

  bool requiresFullUpdate(const NavState& previous) const {
    return (iconHash != previous.iconHash) || (direction != previous.direction);
  }

  bool hasChanged(const NavState& other) const {
    return (distance != other.distance) ||
           (direction != other.direction) ||
           (iconHash != other.iconHash) ||
           (tripInfo != other.tripInfo);
  }
};

NavState currentState;
NavState renderState;
NavState previousState;

SemaphoreHandle_t stateMutex;
TaskHandle_t displayTaskHandle = NULL;

unsigned long lastDisconnectTime = 0;
bool isConnected = false;

// --- GESTIONE ICONE ---
struct IconMapping {
  const char* hashName;
  const uint8_t* bitmap;
};

const IconMapping iconDictionary[] = {
  {"missing", ic_missing},
  {"searching", ic_searching},
  {"connected", ic_connected},
  {"disabled", ic_disabled},
  {"depart", ic_depart},
  {"destination", ic_destination},
  {"destination left", ic_destination_left},
  {"destination right", ic_destination_right},
  {"fork left", ic_fork_left},
  {"fork right", ic_fork_right},
  {"merge", ic_merge},
  {"merge left", ic_merge_left},
  {"merge right", ic_merge_right},
  {"roundabout exit", ic_roundabout_exit},
  {"roundabout left", ic_roundabout_left},
  {"roundabout right", ic_roundabout_right},
  {"roundabout sharp left", ic_roundabout_sharp_left},
  {"roundabout sharp right", ic_roundabout_sharp_right},
  {"roundabout slight left", ic_roundabout_slight_left},
  {"roundabout slight right", ic_roundabout_slight_right},
  {"roundabout straight", ic_roundabout_straight},
  {"roundabout u turn", ic_roundabout_u_turn},
  {"straight", ic_straight},
  {"turn left", ic_turn_left},
  {"turn right", ic_turn_right},
  {"turn sharp left", ic_turn_sharp_left},
  {"turn sharp right", ic_turn_sharp_right},
  {"turn slight left", ic_turn_slight_left},
  {"turn slight right", ic_turn_slight_right},
  {"turn u turn", ic_turn_u_turn}
};
const int numIcons = sizeof(iconDictionary) / sizeof(iconDictionary[0]);

// Funzione helper per recuperare la bitmap
const uint8_t* getIconBitmap(const String& hash) {
  const char* targetHash = hash.c_str();
  for (int i = 0; i < numIcons; i++) {
    if (strcmp(targetHash, iconDictionary[i].hashName) == 0) {
      return iconDictionary[i].bitmap;
    }
  }
  return ic_missing;
}

// --- CALLBACK CONNESSIONE/DISCONNESSIONE BLE ---
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
      isConnected = true;
      Serial.println("Dispositivo connesso");
      
      if (xSemaphoreTake(stateMutex, (TickType_t)10) == pdTRUE) {
        currentState.distance = "OK!";
        currentState.direction = "Connessione attiva";
        currentState.iconHash = "connected"; 
        currentState.tripInfo = "Buon viaggio!";
        xSemaphoreGive(stateMutex);
        
        if (displayTaskHandle != NULL) {
          xTaskNotifyGive(displayTaskHandle);
        }
      } else {
        Serial.println("Impossibile acquisire il mutex in onConnect");
      }
    }

    void onDisconnect(BLEServer* pServer) override {
      isConnected = false;
      lastDisconnectTime = millis();
      Serial.println("Dispositivo disconnesso");
      
      if (xSemaphoreTake(stateMutex, (TickType_t)10) == pdTRUE) {
        currentState.distance = "Errore";
        currentState.direction = "Connessione persa";
        currentState.iconHash = "disabled"; 
        currentState.tripInfo = "Riavvio advertising";
        xSemaphoreGive(stateMutex);
        
        if (displayTaskHandle != NULL) {
          xTaskNotifyGive(displayTaskHandle);
        }
      } else {
        Serial.println("Impossibile acquisire il mutex in onDisconnect");
      }
      BLEDevice::startAdvertising();
    }
};

// --- CALLBACK RICEZIONE DATI BLE ---
class BLEDataCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
      // Recupera direttamente come Arduino String
      String data = pCharacteristic->getValue();
      
      if (data.length() > 0) {
        Serial.println("Ricevuto: " + data);
        
        // Parsing del payload: "distanza|indicazione|codice icona|info viaggio"
        int p1 = data.indexOf('|');
        int p2 = data.indexOf('|', p1 + 1);
        int p3 = data.indexOf('|', p2 + 1);
        
        if (p1 != -1 && p2 != -1 && p3 != -1) {
          String newDistance = data.substring(0, p1);
          String newDirection = data.substring(p1 + 1, p2);
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
          } else {
            Serial.println("Impossibile acquisire il mutex, payload ignorato");
          }
        } else {
          Serial.println("Formato non valido: mancano delimitatori");
        }
      }
    }
};

// --- FUNZIONI DI DISEGNO ---
void printWithMargin(String text, int x, int y, int maxWidth, int lineHeight) {
  String currentLine = "";
  int currentY = y;
  int startIdx = 0;
  
  while (startIdx < text.length()) {
    int spaceIdx = text.indexOf(' ', startIdx);
    if (spaceIdx == -1) spaceIdx = text.length();
    
    String word = text.substring(startIdx, spaceIdx);
    String testLine = (currentLine.length() == 0) ? word : currentLine + " " + word;
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(testLine, 0, 0, &x1, &y1, &w, &h);
    
    if (w > maxWidth && currentLine.length() > 0) {
      display.setCursor(x, currentY);
      display.print(currentLine);
      currentLine = word;
      currentY += lineHeight;
    } else {
      currentLine = testLine;
    }
    
    startIdx = spaceIdx + 1;
  }
  
  if (currentLine.length() > 0) {
    display.setCursor(x, currentY);
    display.print(currentLine);
  }
}

void drawPartialElement(void (*drawFunc)(), uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  display.setPartialWindow(x, y, w, h);
  display.firstPage();
  do { drawFunc(); } while (display.nextPage());
}

void drawDistanceContent() {
  display.fillRect(DIST_BOX_X, DIST_BOX_Y, DIST_BOX_W, DIST_BOX_H, GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(DIST_CURSOR_X, DIST_CURSOR_Y);
  display.print(renderState.distance);
}

void drawTripInfoContent() {
  display.fillRect(TRIP_BOX_X, TRIP_BOX_Y, TRIP_BOX_W, TRIP_BOX_H, GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(TRIP_CURSOR_X, TRIP_CURSOR_Y);
  display.print(renderState.tripInfo);
}

void updateDisplay(bool fullUpdate) {
  if (fullUpdate) {
    Serial.println("Aggiornamento totale display");
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      
      display.drawBitmap(0, 0, getIconBitmap(renderState.iconHash), 100, 100, GxEPD_BLACK);
      
      display.setTextColor(GxEPD_BLACK);
      display.setFont(&FreeSansBold12pt7b);
      display.setCursor(DIST_CURSOR_X, DIST_CURSOR_Y);
      display.print(renderState.distance);
      
      display.setFont(&FreeSans9pt7b);
      printWithMargin(renderState.direction, DIR_CURSOR_X, DIR_CURSOR_Y, 180, DIR_LINE_HEIGHT);
      
      display.setCursor(TRIP_CURSOR_X, TRIP_CURSOR_Y);
      display.print(renderState.tripInfo);
    } while (display.nextPage());
  } else {
    Serial.println("Aggiornamento parziale display");
    drawPartialElement(drawDistanceContent, DIST_BOX_X, DIST_BOX_Y, DIST_BOX_W, DIST_BOX_H); 
    drawPartialElement(drawTripInfoContent, TRIP_BOX_X, TRIP_BOX_Y, TRIP_BOX_W, TRIP_BOX_H);
  }
}

// --- TASK FREERTOS PER IL DISPLAY ---
void displayTask(void *pvParameters) {
  int partialRefreshCount = 0; 

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    bool toUpdate = false;
    
    if (xSemaphoreTake(stateMutex, portMAX_DELAY)) {
      if (currentState.hasChanged(previousState)) {
        renderState = currentState;
        toUpdate = true;
      }
      xSemaphoreGive(stateMutex);
    }

    if (toUpdate) {
      bool requiresFull = renderState.requiresFullUpdate(previousState);
      
      if (!requiresFull) {
        partialRefreshCount++;
        if (partialRefreshCount >= 20) {
          requiresFull = true;
          Serial.println("Forzato aggiornamento totale");
        }
      } 
      
      if (requiresFull) {
        partialRefreshCount = 0;
      }

      updateDisplay(requiresFull);
      
      previousState = renderState;
    }
  }
}

// --- SETUP HARDWARE E MODULI ---
void setupDisplay() {
  pinMode(EPD_PWR_EN, OUTPUT);
  digitalWrite(EPD_PWR_EN, LOW); 
  delay(100);

  SPI.begin(EPD_SCLK, -1, EPD_MOSI, -1);
  display.epd2.selectSPI(SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  display.init(115200, true, 2, false);
  display.setRotation(0);
}

void setupBLE() {
  BLEDevice::init("E-INK_MAPS");
  String macAddress = BLEDevice::getAddress().toString().c_str();
  macAddress.toUpperCase();
  
  currentState.tripInfo = macAddress;
  renderState = currentState;
  updateDisplay(true);
  previousState = renderState;

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE_NR
  );

  pCharacteristic->setCallbacks(new BLEDataCallback());
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Attivo. MAC: " + macAddress);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  stateMutex = xSemaphoreCreateMutex();
  
  setupDisplay();
  
  xTaskCreatePinnedToCore(
    displayTask,        // Funzione del task
    "Display_Task",     // Nome del task
    8192,               // Dimensione dello Stack (in words)
    NULL,               // Parametri del task
    1,                  // Priorità
    &displayTaskHandle, // Handle
    1                   // Esegui sul Core 1
  );

  setupBLE();
}

void loop() {
  if (!isConnected && (millis() - lastDisconnectTime > 60000)) {
    Serial.println("Spegnimento per inattività");

    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(0, 0, ic_splash, 200, 200, GxEPD_BLACK);
    } while (display.nextPage());

    display.hibernate();
    delay(100);

    digitalWrite(EPD_PWR_EN, HIGH); 

    Serial.flush();
    esp_deep_sleep_start();
  }

  vTaskDelay(pdMS_TO_TICKS(1000)); 
}