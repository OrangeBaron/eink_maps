#pragma once
#include <Arduino.h>

// --- PIN HARDWARE (WaveShare ESP32-S3-ePaper-1.54 V2) ---
#define EPD_CS       11
#define EPD_DC       10
#define EPD_RST      9
#define EPD_BUSY     8
#define EPD_SCLK     12
#define EPD_MOSI     13
#define EPD_PWR_EN   6
#define BAT_CTRL     17
#define BAT_ADC      4

// --- UUID BLE ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- COSTANTI DI LAYOUT ---
constexpr uint16_t DIST_BOX_X = 100;
constexpr uint16_t DIST_BOX_Y = 0;
constexpr uint16_t DIST_BOX_W = 100;
constexpr uint16_t DIST_BOX_H = 100;

constexpr uint16_t DIR_CURSOR_X = 10;
constexpr uint16_t DIR_CURSOR_Y = 125;
constexpr uint16_t DIR_LINE_HEIGHT = 26;
constexpr uint16_t DIR_TEXT_MAX_WIDTH = 180;

constexpr uint16_t TRIP_BOX_X = 0;
constexpr uint16_t TRIP_BOX_Y = 166;
constexpr uint16_t TRIP_BOX_W = 200;
constexpr uint16_t TRIP_BOX_H = 34;
constexpr uint16_t TRIP_CURSOR_X = 10;
constexpr uint16_t TRIP_CURSOR_Y = 188;

constexpr uint16_t ICON_SIZE = 100;

// --- COSTANTI DISPLAY ---
constexpr int MAX_PARTIAL_REFRESHES = 20;

// --- TIMEOUT SISTEMA ---
constexpr unsigned long DISCONNECT_TIMEOUT_MS = 60000;