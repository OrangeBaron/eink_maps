#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

struct NavState {
  bool isNavigating = false;
  String distance = "battery";
  String direction = "Attesa di connessione";
  String iconHash = "searching";
  String tripInfo = "MAC address";

  bool requiresFullUpdate(const NavState& previous) const {
    return (iconHash != previous.iconHash) || 
           (direction != previous.direction) ||
           (isNavigating != previous.isNavigating);
  }

  bool hasChanged(const NavState& other) const {
    return (isNavigating != other.isNavigating) ||
           (distance != other.distance) ||
           (direction != other.direction) ||
           (iconHash != other.iconHash) ||
           (tripInfo != other.tripInfo);
  }
};

extern NavState currentState;
extern NavState renderState;
extern NavState previousState;

extern SemaphoreHandle_t stateMutex;
extern TaskHandle_t displayTaskHandle;