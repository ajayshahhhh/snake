// sounds.cpp — sends sound commands to the Mac over USB serial
//
// The Arduino has no speaker. Instead, every sound event sends a short text
// command (e.g. "SOUND:EAT") over the USB serial cable to the Mac, where
// sound_player.py receives it and plays the matching audio file via pygame.

#include "sounds.h"
#include <Arduino.h>

void soundInit() {
  // Serial is already started in setup(); nothing extra needed here.
}

void soundMusicStart() { Serial.println("MUSIC:START"); }
void soundMusicStop()  { Serial.println("MUSIC:STOP");  }
void soundDie()        { Serial.println("SOUND:DIE");   }
void soundEat()        { Serial.println("SOUND:EAT");   }

// Menu voice clips — sound_player.py plays the matching .m4a recording
void soundMenuIntro()              { Serial.println("SOUND:MENU_INTRO");  }
void soundModeSelect()             { Serial.println("SOUND:MODE_SELECT"); }

void soundModeName(GameMode mode) {
  switch (mode) {
    case MODE_NORMAL:      Serial.println("SOUND:MODE_NORMAL");  break;
    case MODE_ENDLESS:     Serial.println("SOUND:MODE_ENDLESS"); break;
    case MODE_MULTI_APPLE: Serial.println("SOUND:MODE_MULTI");   break;
    case MODE_SPEED_RUN:   Serial.println("SOUND:MODE_SPEED");   break;
  }
}
