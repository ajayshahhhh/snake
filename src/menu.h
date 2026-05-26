#pragma once
#include <Arduino.h>

enum GameMode {
  MODE_NORMAL     = 0,
  MODE_ENDLESS    = 1,
  MODE_MULTI_APPLE = 2,
  MODE_SPEED_RUN  = 3,
};
#define MODE_COUNT 4

// Blocks until the player selects a mode; returns chosen GameMode
GameMode menuRun();
