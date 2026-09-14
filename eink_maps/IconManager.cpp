#include "IconManager.h"
#include "icons.h"

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

const uint8_t* getIconBitmap(const String& hash) {
  const char* targetHash = hash.c_str();
  for (int i = 0; i < numIcons; i++) {
    if (strcmp(targetHash, iconDictionary[i].hashName) == 0) {
      return iconDictionary[i].bitmap;
    }
  }
  return ic_missing;
}

const uint8_t* getSplashIcon() {
  return ic_splash;
}