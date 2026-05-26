// menu.cpp — game mode selection screen
//
// Runs once at power-on. Shows a preview pattern for each mode on the LED
// matrix. The player pushes the joystick left/right to cycle through modes
// and presses the button to confirm. The chosen GameMode is returned to
// setup() and never changes until the Arduino resets.

#include "menu.h"
#include "led_driver.h"
#include "joystick.h"
#include "sounds.h"
#include <Arduino.h>

// -----------------------------------------------------------------------
// One preview pattern per game mode
// -----------------------------------------------------------------------

// Normal — solid border shows the walls the snake must avoid
static void buildNormalPattern(byte pat[8][8]) {
  memset(pat, 0, 64);
  for (int i = 0; i < 8; i++) {
    pat[0][i] = 1; pat[7][i] = 1;  // top and bottom rows
    pat[i][0] = 1; pat[i][7] = 1;  // left and right columns
  }
}

// Endless — dashed border hints that walls wrap around instead of killing you
static void buildEndlessPattern(byte pat[8][8]) {
  memset(pat, 0, 64);
  for (int i = 0; i < 8; i++) {
    if (i % 2 == 0) { pat[0][i] = 1; pat[i][0] = 1; }
    else             { pat[7][i] = 1; pat[i][7] = 1; }
  }
}

// Multi-apple — three scattered dots represent the multiple apples on screen
static void buildMultiApplePattern(byte pat[8][8]) {
  memset(pat, 0, 64);
  pat[1][2] = 1;
  pat[3][5] = 1;
  pat[5][2] = 1;
}

// Speed Run — right-pointing arrow hints the snake speeds up
static void buildSpeedRunPattern(byte pat[8][8]) {
  memset(pat, 0, 64);
  pat[3][1] = pat[3][2] = pat[3][3] = pat[3][4] = pat[3][5] = 1;  // shaft
  pat[2][4] = 1; pat[1][5] = 1;  // top arrowhead
  pat[4][4] = 1; pat[5][5] = 1;  // bottom arrowhead
}

static void buildModePattern(int modeIdx, byte pat[8][8]) {
  switch (modeIdx) {
    case MODE_NORMAL:      buildNormalPattern(pat);     break;
    case MODE_ENDLESS:     buildEndlessPattern(pat);    break;
    case MODE_MULTI_APPLE: buildMultiApplePattern(pat); break;
    case MODE_SPEED_RUN:   buildSpeedRunPattern(pat);   break;
  }
}

// -----------------------------------------------------------------------
// Menu loop
// -----------------------------------------------------------------------

GameMode menuRun() {
  int  modeIdx = 0;
  byte menuPat[8][8];

  // Play the intro voice clip ("move left or right to look at game modes")
  soundMenuIntro();

  // Keep the screen blank while the intro plays
  memset(menuPat, 0, 64);
  unsigned long introStart = millis();
  while (millis() - introStart < 2500) {
    ledDisplay(menuPat);
  }

  // Show the first mode and announce its name
  buildModePattern(modeIdx, menuPat);
  soundModeName((GameMode)modeIdx);

  unsigned long lastMove = 0;

  while (true) {
    ledDisplay(menuPat);

    int  dir = joyGetDir();
    unsigned long now = millis();

    // Cycle modes on joystick push (400ms debounce so it doesn't fly past)
    if (dir != JOY_NONE && now - lastMove > 400) {
      lastMove = now;
      if (dir == JOY_RIGHT || dir == JOY_DOWN) {
        modeIdx = (modeIdx + 1) % MODE_COUNT;
      } else if (dir == JOY_LEFT || dir == JOY_UP) {
        modeIdx = (modeIdx + MODE_COUNT - 1) % MODE_COUNT;
      }
      buildModePattern(modeIdx, menuPat);
      soundModeName((GameMode)modeIdx);
    }

    // Button press confirms the selection
    if (joyButtonHeld()) {
      soundModeSelect();
      while (joyButtonHeld()) { ledDisplay(menuPat); }  // wait for release
      return (GameMode)modeIdx;
    }
  }
}
