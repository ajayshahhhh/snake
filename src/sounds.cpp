#include "sounds.h"
#include <Arduino.h>

void soundInit() {
  // Serial already initialized in main.cpp
}

void soundMusicStart() {
  Serial.println("MUSIC:START");
}

void soundMusicStop() {
  Serial.println("MUSIC:STOP");
}

void soundDie() {
  Serial.println("SOUND:DIE");
}

void soundEat() {
  Serial.println("SOUND:EAT");
}
