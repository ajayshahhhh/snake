// led_driver.cpp — controls the 8×8 LED matrix
//
// The matrix has 8 row pins (anodes) and 8 column pins (cathodes).
// Only one row is powered at a time; we cycle through all 8 rows so fast
// (~800µs each) that the eye sees them all lit simultaneously.
// This technique is called multiplexing.

#include "led_driver.h"

// Row pins source current (active LOW via PNP transistors)
// Column pins sink current (active LOW = LED on)
static const byte PLUS_PINS[8]  = {13, 12, 11, 10, 9, 8, 7, 6};
static const byte MINUS_PINS[8] = {A3, A2, A1, A0, 5, 4, 3, 2};

// -----------------------------------------------------------------------
// "GAME OVER" scroll font
// Each letter is 5 columns wide. Each byte is one column:
//   bit 7 = top row (row 0), bit 0 = bottom row (row 7)
// -----------------------------------------------------------------------
static const byte FONT_G[] = {0b00111110, 0b01000001, 0b01001001, 0b01001001, 0b00111010};
static const byte FONT_A[] = {0b00111111, 0b01001000, 0b01001000, 0b01001000, 0b00111111};
static const byte FONT_M[] = {0b01111111, 0b00100000, 0b00010000, 0b00100000, 0b01111111};
static const byte FONT_E[] = {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b01000001};
static const byte FONT_O[] = {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00111110};
static const byte FONT_V[] = {0b01110000, 0b00001100, 0b00000011, 0b00001100, 0b01110000};
static const byte FONT_R[] = {0b01111111, 0b01001000, 0b01001000, 0b01001000, 0b00110111};

#define SCROLL_BUF_MAX 80
static byte scrollBuf[SCROLL_BUF_MAX];
static int  scrollBufLen = 0;

#define BLINK_PERIOD 300  // ms between food blink on/off

// -----------------------------------------------------------------------
// Pin helpers
// -----------------------------------------------------------------------
static void rowOff(int row) { digitalWrite(PLUS_PINS[row], HIGH); }
static void rowOn(int row)  { digitalWrite(PLUS_PINS[row], LOW);  }
static void colSet(int col, bool lit) {
  digitalWrite(MINUS_PINS[col], lit ? LOW : HIGH);
}

// -----------------------------------------------------------------------
// Scroll buffer helpers — append font data and blank spacer columns
// -----------------------------------------------------------------------
static void appendCols(const byte* cols, int n) {
  for (int i = 0; i < n && scrollBufLen < SCROLL_BUF_MAX; i++)
    scrollBuf[scrollBufLen++] = cols[i];
}
static void appendBlanks(int n) {
  for (int i = 0; i < n && scrollBufLen < SCROLL_BUF_MAX; i++)
    scrollBuf[scrollBufLen++] = 0;
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void ledInit() {
  for (int i = 0; i < 8; i++) {
    pinMode(PLUS_PINS[i],  OUTPUT); rowOff(i);
    pinMode(MINUS_PINS[i], OUTPUT); colSet(i, false);
  }
}

// Displays one row of the pattern for 800µs, then turns it off.
// pat[row][col] > 0 means that LED is on.
void ledScanRow(int row, byte pat[8][8]) {
  for (int col = 0; col < 8; col++) {
    colSet(col, pat[row][col] > 0);
  }
  rowOn(row);
  delayMicroseconds(800);
  rowOff(row);
}

// Scans all 8 rows once. Call this as fast as possible in the game loop
// to keep the display looking solid.
void ledDisplay(byte pat[8][8]) {
  for (int row = 0; row < 8; row++) ledScanRow(row, pat);
}

// Converts a column-major bit buffer (used for scrolling) into a pat array
// and displays it. Each byte is one column; bit 7 = top LED.
void ledDisplayCols(byte* cols) {
  byte pat[8][8];
  for (int col = 0; col < 8; col++)
    for (int row = 0; row < 8; row++)
      pat[row][col] = (cols[col] >> (7 - row)) & 1 ? 1 : 0;
  ledDisplay(pat);
}

void ledClear() {
  for (int col = 0; col < 8; col++) colSet(col, false);
  for (int row = 0; row < 8; row++) rowOff(row);
}

// Returns true during the "on" phase of the food blink cycle
bool ledFoodVisible() {
  return ((millis() / BLINK_PERIOD) & 1) == 1;
}

// Builds the full scroll buffer for "GAME OVER" with leading/trailing blanks
void ledBuildScrollBuf() {
  scrollBufLen = 0;
  appendBlanks(8);
  appendCols(FONT_G, 5); appendBlanks(1);
  appendCols(FONT_A, 5); appendBlanks(1);
  appendCols(FONT_M, 5); appendBlanks(1);
  appendCols(FONT_E, 5); appendBlanks(3);
  appendCols(FONT_O, 5); appendBlanks(1);
  appendCols(FONT_V, 5); appendBlanks(1);
  appendCols(FONT_E, 5); appendBlanks(1);
  appendCols(FONT_R, 5);
  appendBlanks(8);
}

// Scrolls "GAME OVER" repeatedly until the player double-presses the button.
// Double-press state machine:
//   WAIT_PRESS1 → button goes down → WAIT_RELEASE
//   WAIT_RELEASE → button comes up → WAIT_PRESS2
//   WAIT_PRESS2 → button goes down again (within 600ms) → return true
bool ledScrollGameOver() {
  ledBuildScrollBuf();

  enum { WAIT_PRESS1, WAIT_RELEASE, WAIT_PRESS2 } state = WAIT_PRESS1;
  unsigned long stateStart = 0;

  while (true) {
    for (int offset = 0; offset <= scrollBufLen - 8; offset++) {
      unsigned long frameStart = millis();
      while (millis() - frameStart < 80) {
        ledDisplayCols(&scrollBuf[offset]);

        bool btnDown = (digitalRead(0) == LOW);
        unsigned long now = millis();

        if (state == WAIT_PRESS1 && btnDown) {
          state = WAIT_RELEASE;
          stateStart = now;
        } else if (state == WAIT_RELEASE && !btnDown) {
          state = WAIT_PRESS2;
          stateStart = now;
        } else if (state == WAIT_PRESS2) {
          if (btnDown) return true;
          if (now - stateStart > 600) state = WAIT_PRESS1;
        }
      }
    }
  }
}
