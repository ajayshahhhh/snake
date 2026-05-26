#pragma once
#include <Arduino.h>

// Direction constants (match snake game dirs)
#define JOY_NONE  -1
#define JOY_UP     0
#define JOY_RIGHT  1
#define JOY_DOWN   2
#define JOY_LEFT   3

// Initialize joystick pins
void joyInit();

// Raw analog reads (0–1023, center ≈ 512)
int joyRawX();
int joyRawY();

// True if the stick is deflected past the deadzone threshold
bool joyDeflectedX();
bool joyDeflectedY();

// Returns JOY_UP/DOWN/LEFT/RIGHT, or JOY_NONE if centered
int joyGetDir();

// True while the button is held (active-low)
bool joyButtonHeld();

// True only on the falling edge (press event, not hold)
bool joyButtonPressed();
