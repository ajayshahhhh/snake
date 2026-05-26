#include "joystick.h"

#define PIN_VRX    A4
#define PIN_VRY    A5
#define PIN_SW     0
#define DEADZONE   200   // analog units from center (512) to register a move
#define CENTER     512

static bool lastButtonState = HIGH;

void joyInit() {
  pinMode(PIN_SW, INPUT_PULLUP);
  // A4, A5 are analog-in by default; no pinMode needed
}

int joyRawX() { return analogRead(PIN_VRX); }
int joyRawY() { return analogRead(PIN_VRY); }

bool joyDeflectedX() { return abs(joyRawX() - CENTER) > DEADZONE; }
bool joyDeflectedY() { return abs(joyRawY() - CENTER) > DEADZONE; }

int joyGetDir() {
  int x = joyRawX();
  int y = joyRawY();
  int dx = x - CENTER;
  int dy = y - CENTER;

  // Prefer the dominant axis
  if (abs(dx) >= abs(dy)) {
    if (dx >  DEADZONE) return JOY_RIGHT;
    if (dx < -DEADZONE) return JOY_LEFT;
  } else {
    if (dy >  DEADZONE) return JOY_DOWN;
    if (dy < -DEADZONE) return JOY_UP;
  }
  return JOY_NONE;
}

bool joyButtonHeld() { return digitalRead(PIN_SW) == LOW; }

bool joyButtonPressed() {
  bool current = digitalRead(PIN_SW);
  bool pressed = (lastButtonState == HIGH && current == LOW);
  lastButtonState = current;
  return pressed;
}
