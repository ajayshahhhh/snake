#pragma once
#include <Arduino.h>

// Initialize all row/col pins
void ledInit();

// Light one row with PWM brightness (0–15); higher = brighter
// pat[row][col] = 0..15 brightness, 0 = off
void ledScanRow(int row, byte pat[8][8]);

// Multiplex all 8 rows once through with PWM brightness control
void ledDisplay(byte pat[8][8]);

// Display from column-major scroll buffer (cols[c] bit7=row0, bit0=row7, binary)
void ledDisplayCols(byte* cols);

// Clear all LEDs immediately
void ledClear();

// Blink timer for food; call once per game loop
void ledUpdateBlink();

// Returns true if food should be visible (blinking on/off)
bool ledFoodVisible();

// Build the "GAME OVER" scroll buffer internally
void ledBuildScrollBuf();

// Scroll "GAME OVER" until animation completes; returns true if restart requested
bool ledScrollGameOver();
