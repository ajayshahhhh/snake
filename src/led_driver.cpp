#include "led_driver.h"

static const byte PLUS_PINS[8]  = {13, 12, 11, 10, 9, 8, 7, 6};
static const byte MINUS_PINS[8] = {A3, A2, A1, A0, 5, 4, 3, 2};

// ---- Font: bit7 = top row, bit0 = bottom row, 5 cols each ----
static const byte FONT_G[] = {0b00111110, 0b01000001, 0b01001001, 0b01001001, 0b00111010};
static const byte FONT_A[] = {0b01111110, 0b01001000, 0b01001000, 0b01001000, 0b01111110};
static const byte FONT_M[] = {0b01111111, 0b00100000, 0b00010000, 0b00100000, 0b01111111};
static const byte FONT_E[] = {0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b01000001};
static const byte FONT_O[] = {0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00111110};
static const byte FONT_V[] = {0b01000011, 0b00101100, 0b00010000, 0b00101100, 0b01000011};
static const byte FONT_R[] = {0b01111111, 0b01001000, 0b01001000, 0b01001000, 0b00110111};

#define SCROLL_BUF_MAX 80
static byte scrollBuf[SCROLL_BUF_MAX];
static int  scrollBufLen = 0;

// ---- Blink state ----
#define BLINK_PERIOD 300  // ms between on/off toggle

// ---- Pin helpers ----
static void rowOff(int row)  { digitalWrite(PLUS_PINS[row],  HIGH); }
static void rowOn(int row)   { digitalWrite(PLUS_PINS[row],  LOW);  }
static void colSet(int col, bool lit) {
  digitalWrite(MINUS_PINS[col], lit ? LOW : HIGH);
}

// ---- Scroll buffer helpers ----
static void appendCols(const byte* cols, int n) {
  for (int i = 0; i < n && scrollBufLen < SCROLL_BUF_MAX; i++)
    scrollBuf[scrollBufLen++] = cols[i];
}
static void appendBlanks(int n) {
  for (int i = 0; i < n && scrollBufLen < SCROLL_BUF_MAX; i++)
    scrollBuf[scrollBufLen++] = 0;
}

// ---- Public API ----
void ledInit() {
  for (int i = 0; i < 8; i++) {
    pinMode(PLUS_PINS[i],  OUTPUT); rowOff(i);
    pinMode(MINUS_PINS[i], OUTPUT); colSet(i, false);
  }
}

void ledScanRow(int row, byte pat[8][8]) {
  // PWM brightness via multi-pass: for each brightness level (1-15),
  // light LEDs that exceed that level. Creates perceived brightness via duty cycle.
  for (int cycle = 0; cycle < 15; cycle++) {
    for (int col = 0; col < 8; col++) {
      byte brightness = pat[row][col];
      colSet(col, brightness > cycle);
    }
    rowOn(row);
    delayMicroseconds(60);
    rowOff(row);
  }
}

void ledDisplay(byte pat[8][8]) {
  for (int row = 0; row < 8; row++) ledScanRow(row, pat);
  ledUpdateBlink();
}

void ledDisplayCols(byte* cols) {
  byte pat[8][8];
  for (int col = 0; col < 8; col++)
    for (int row = 0; row < 8; row++)
      pat[row][col] = (cols[col] >> (7 - row)) & 1 ? 15 : 0;
  ledDisplay(pat);
}

void ledClear() {
  for (int col = 0; col < 8; col++) colSet(col, false);
  for (int row = 0; row < 8; row++) rowOff(row);
}

void ledUpdateBlink() {
  // no-op; blinking is now time-based
}

bool ledFoodVisible() {
  unsigned long now = millis();
  unsigned long phase = (now / BLINK_PERIOD) & 1;
  return phase == 1;
}

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

bool ledScrollGameOver() {
  ledBuildScrollBuf();
  for (int offset = 0; offset <= scrollBufLen - 8; offset++) {
    unsigned long start = millis();
    while (millis() - start < 80) {
      ledDisplayCols(&scrollBuf[offset]);
    }
    if (digitalRead(0) == LOW) return true;
  }
  return false;
}
