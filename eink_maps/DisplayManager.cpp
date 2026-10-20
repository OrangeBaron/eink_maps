#include "DisplayManager.h"
#include "Config.h"
#include "State.h"
#include "IconManager.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <vector>

// Inizializza il display e-paper 1.54" con driver SSD1681
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// --- HELPER DI INTERFACCIA GRAFICA ---
namespace UIHelpers {
    void printWithMargin(String text, int x, int y, int maxWidth, int defaultLineHeight) {
        std::vector<String> lines;
        String currentLine = "";
        int startIdx = 0;
        
        while (startIdx < text.length()) {
            if (lines.size() >= 2) {
                String remainingText = text.substring(startIdx);
                currentLine = (currentLine.length() == 0) ? remainingText : currentLine + " " + remainingText;
                break;
            }
            
            int spaceIdx = text.indexOf(' ', startIdx);
            if (spaceIdx == -1) spaceIdx = text.length();
            
            String word = text.substring(startIdx, spaceIdx);
            String testLine = (currentLine.length() == 0) ? word : currentLine + " " + word;
            
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(testLine, 0, 0, &x1, &y1, &w, &h);
            
            if (w > maxWidth && currentLine.length() > 0) {
                lines.push_back(currentLine);
                currentLine = word;
            } else {
                currentLine = testLine;
            }
            
            startIdx = spaceIdx + 1;
        }
        
        if (currentLine.length() > 0) {
            lines.push_back(currentLine);
        }
        
        int actualLineHeight = defaultLineHeight;
        int currentY = y;
        
        if (lines.size() == 3) {
            actualLineHeight -= 5;
            currentY -= 5;
        }
        
        for (const String& line : lines) {
            display.setCursor(x, currentY);
            display.print(line);
            currentY += actualLineHeight;
        }
    }
}

void setupDisplay() {
    pinMode(EPD_PWR_EN, OUTPUT);
    digitalWrite(EPD_PWR_EN, LOW); 
    delay(100);

    SPI.begin(EPD_SCLK, -1, EPD_MOSI, -1);
    display.epd2.selectSPI(SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    display.init(0, false, 2, false);
    display.setRotation(0);
}

// --- FUNZIONI DI DISEGNO SPECIFICHE ---

void drawBatteryInfo(int batteryPercent) {
    display.drawBitmap(DIST_BOX_X, DIST_BOX_Y, getBatteryIcon(batteryPercent), DIST_BOX_W, DIST_BOX_H, GxEPD_BLACK);
}

void drawDistanceInfo(String distText, bool isBattLow) {
    if (!isBattLow) {
        // --- LAYOUT STANDARD ---
        display.setFont(&FreeSansBold18pt7b);
        
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(distText, 0, 0, &tbx, &tby, &tbw, &tbh);
        
        uint16_t x = DIST_BOX_X + ((DIST_BOX_W - tbw) / 2) - tbx;
        uint16_t y = DIST_BOX_Y + ((DIST_BOX_H - tbh) / 2) - tby;
        
        display.setCursor(x, y);
        display.print(distText);
    } else {
        // --- LAYOUT BATTERIA SCARICA ---
        int16_t tbx, tby; uint16_t tbw, tbh;

        display.setFont(&FreeSansBold18pt7b);
        display.getTextBounds(distText, 0, 0, &tbx, &tby, &tbw, &tbh);
        
        uint16_t xDist = DIST_BOX_X + ((DIST_BOX_W - tbw) / 2) - tbx;
        uint16_t yDist = DIST_BOX_Y + 45;
        display.setCursor(xDist, yDist);
        display.print(distText);

        display.setFont(&FreeSansBold9pt7b);
        String battText = "BATT LOW";
        display.getTextBounds(battText, 0, 0, &tbx, &tby, &tbw, &tbh);
        
        uint16_t xBatt = DIST_BOX_X + ((DIST_BOX_W - tbw) / 2) - tbx;
        uint16_t yBatt = DIST_BOX_Y + 80;
        display.setCursor(xBatt, yBatt);
        display.print(battText);
    }
}

void drawDistanceContent() {
    if (!renderState.isNavigating) {
        drawBatteryInfo(renderState.batteryPercent);
    } else {
        bool isBattLow = (renderState.batteryPercent <= 20);
        drawDistanceInfo(renderState.distance, isBattLow);
    }
}

void drawTripInfoContent() {
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(TRIP_CURSOR_X, TRIP_CURSOR_Y);
    display.print(renderState.tripInfo);
}

void drawDirectionContent() {
    display.setFont(&FreeSansBold12pt7b);
    UIHelpers::printWithMargin(renderState.direction, DIR_CURSOR_X, DIR_CURSOR_Y, DIR_TEXT_MAX_WIDTH, DIR_LINE_HEIGHT);
}

// --- LOGICA DI AGGIORNAMENTO ---

void updateDisplay(bool fullUpdate) {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, getIconBitmap(renderState.iconHash), ICON_SIZE, ICON_SIZE, GxEPD_BLACK);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(false);

    drawDistanceContent();
    drawDirectionContent();
    drawTripInfoContent();

    if (fullUpdate) {
        display.display(false); 
    } else {
        display.displayWindow(DIST_BOX_X, DIST_BOX_Y, DIST_BOX_W, DIST_BOX_H);
        display.displayWindow(TRIP_BOX_X, TRIP_BOX_Y, TRIP_BOX_W, TRIP_BOX_H);
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

// --- TASK FREERTOS ---
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
                if (partialRefreshCount >= MAX_PARTIAL_REFRESHES) {
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