#include "DisplayManager.h"
#include "Config.h"
#include "State.h"
#include "IconManager.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// Inizializza il display e-paper 1.54" con driver SSD1681
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setupDisplay() {
  pinMode(EPD_PWR_EN, OUTPUT);
  digitalWrite(EPD_PWR_EN, LOW); 
  delay(100);

  SPI.begin(EPD_SCLK, -1, EPD_MOSI, -1);
  display.epd2.selectSPI(SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  display.init(0, false, 2, false);
  display.setRotation(0);
}

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
    drawPartialElement(drawDistanceContent, DIST_BOX_X, DIST_BOX_Y, DIST_BOX_W, DIST_BOX_H); 
    drawPartialElement(drawTripInfoContent, TRIP_BOX_X, TRIP_BOX_Y, TRIP_BOX_W, TRIP_BOX_H);
  }
}

void showSplashAndHibernate() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, getSplashIcon(), 200, 200, GxEPD_BLACK);
  } while (display.nextPage());

  display.hibernate();
}

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