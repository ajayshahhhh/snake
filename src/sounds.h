#pragma once
#include "menu.h"

// Send sound commands to the computer over serial
void soundInit();

// Start background game music
void soundMusicStart();

// Stop background music
void soundMusicStop();

// Play death/sad sound
void soundDie();

// Play eat/collect sound
void soundEat();

// Menu audio
void soundMenuIntro();
void soundModeName(GameMode mode);
void soundModeSelect();
