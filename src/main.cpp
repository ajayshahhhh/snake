// main.cpp — Snake game entry point
//
// Overall flow (state machine):
//   Power on → menuRun() lets player pick a game mode
//   → initGame() sets up the snake and food
//   → loop() runs the game until the snake dies
//   → ledScrollGameOver() scrolls "GAME OVER", waits for double press
//   → initGame() restarts in the same mode (no menu again until reset)

#include <Arduino.h>
#include "led_driver.h"
#include "joystick.h"
#include "sounds.h"
#include "menu.h"

// -----------------------------------------------------------------------
// Tuning constants
// -----------------------------------------------------------------------
#define MAX_LEN      64    // maximum snake length (ring buffer size)
#define BASE_SPEED   300   // milliseconds between game ticks (normal speed)
#define MIN_SPEED    100   // fastest the snake can go (speed run mode)
#define SPEED_STEP   20    // ms shaved off per apple in speed run
#define MAX_FOOD     3     // maximum simultaneous apples (multi-apple mode)
#define INPUT_DEBOUNCE 150 // ms to ignore repeat direction inputs

// -----------------------------------------------------------------------
// Game state
// -----------------------------------------------------------------------
static GameMode currentMode;   // chosen once in the menu, never changes mid-game

// Snake is stored as a circular ring buffer so we never shift array elements.
// snakeHead points to the newest segment; older segments are behind it.
static byte snakeRow[MAX_LEN];
static byte snakeCol[MAX_LEN];
static int  snakeHead = 0;
static int  snakeLen  = 3;

static int dir     = JOY_RIGHT;  // direction currently moving
static int nextDir = JOY_RIGHT;  // direction queued by the player

// Food — up to MAX_FOOD apples depending on mode
static byte foodRow[MAX_FOOD];
static byte foodCol[MAX_FOOD];
static int  numFood = 1;

static byte          pattern[8][8];       // pixel buffer sent to the LED matrix
static bool          gameOver  = false;
static unsigned long lastTick  = 0;       // timestamp of the last game tick
static unsigned long lastInput = 0;       // timestamp of the last direction change
static unsigned long gameSpeed = BASE_SPEED;

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

// Returns true if a given grid cell is occupied by any part of the snake
static bool occupies(byte r, byte c) {
  for (int i = 0; i < snakeLen; i++) {
    int idx = ((snakeHead - i) + MAX_LEN) % MAX_LEN;
    if (snakeRow[idx] == r && snakeCol[idx] == c) return true;
  }
  return false;
}

// Picks a random empty cell for food slot f (avoids snake and other apples)
static void placeFood(int f) {
  byte r, c;
  bool blocked;
  do {
    r = random(8);
    c = random(8);
    blocked = occupies(r, c);
    if (!blocked) {
      for (int i = 0; i < numFood; i++) {
        if (i != f && foodRow[i] == r && foodCol[i] == c) { blocked = true; break; }
      }
    }
  } while (blocked);
  foodRow[f] = r;
  foodCol[f] = c;
}

static byte headRow() { return snakeRow[snakeHead]; }
static byte headCol() { return snakeCol[snakeHead]; }

// -----------------------------------------------------------------------
// Game init — called at start and after every death
// -----------------------------------------------------------------------
static void initGame() {
  snakeLen  = 3;
  snakeHead = 2;
  dir       = JOY_RIGHT;
  nextDir   = JOY_RIGHT;
  gameOver  = false;
  gameSpeed = BASE_SPEED;

  // Place the initial three segments in a row on the middle-left
  snakeRow[0] = 3; snakeCol[0] = 0;
  snakeRow[1] = 3; snakeCol[1] = 1;
  snakeRow[2] = 3; snakeCol[2] = 2;

  numFood = (currentMode == MODE_MULTI_APPLE) ? 3 : 1;
  for (int f = 0; f < numFood; f++) placeFood(f);

  lastTick = millis();
  soundMusicStart();
}

// -----------------------------------------------------------------------
// Drawing — builds the pixel pattern each frame
// -----------------------------------------------------------------------
static void clearPattern() { memset(pattern, 0, sizeof(pattern)); }

static void drawSnake() {
  for (int i = 0; i < snakeLen; i++) {
    int idx = ((snakeHead - i) + MAX_LEN) % MAX_LEN;
    pattern[snakeRow[idx]][snakeCol[idx]] = 1;
  }
}

// Food blinks on/off; ledFoodVisible() returns false during the "off" phase
static void drawFood() {
  if (!ledFoodVisible()) return;
  for (int f = 0; f < numFood; f++) {
    pattern[foodRow[f]][foodCol[f]] = 1;
  }
}

static void buildPattern() {
  clearPattern();
  drawSnake();
  drawFood();
}

// -----------------------------------------------------------------------
// Movement helpers
// -----------------------------------------------------------------------
static void applyDir(byte& r, byte& c, int d) {
  if      (d == JOY_UP)    r--;
  else if (d == JOY_RIGHT) c++;
  else if (d == JOY_DOWN)  r++;
  else if (d == JOY_LEFT)  c--;
}

// Prevents the snake from instantly reversing into itself
static bool isOpposite(int a, int b) {
  return (a == JOY_UP    && b == JOY_DOWN)  ||
         (a == JOY_DOWN  && b == JOY_UP)    ||
         (a == JOY_LEFT  && b == JOY_RIGHT) ||
         (a == JOY_RIGHT && b == JOY_LEFT);
}

// -----------------------------------------------------------------------
// Game tick — advances the snake one step
// -----------------------------------------------------------------------
static void gameTick() {
  dir = nextDir;

  byte nr = headRow();
  byte nc = headCol();
  applyDir(nr, nc, dir);

  // Wall collision: endless mode wraps around, all other modes die
  if (currentMode == MODE_ENDLESS) {
    nr = (nr + 8) % 8;
    nc = (nc + 8) % 8;
  } else {
    // byte underflow (0-1) produces 255, which is >= 8, so this catches both edges
    if (nr >= 8 || nc >= 8) { gameOver = true; soundDie(); return; }
  }

  // Self-collision
  if (occupies(nr, nc)) { gameOver = true; soundDie(); return; }

  // Check if head landed on any apple
  int eatenFood = -1;
  for (int f = 0; f < numFood; f++) {
    if (nr == foodRow[f] && nc == foodCol[f]) { eatenFood = f; break; }
  }

  // Advance the ring buffer head and write the new position
  snakeHead = (snakeHead + 1) % MAX_LEN;
  snakeRow[snakeHead] = nr;
  snakeCol[snakeHead] = nc;

  if (eatenFood >= 0) {
    soundEat();
    snakeLen++;
    if (snakeLen >= MAX_LEN) { gameOver = true; soundDie(); return; }
    placeFood(eatenFood);
    if (currentMode == MODE_SPEED_RUN && gameSpeed > MIN_SPEED) {
      gameSpeed -= SPEED_STEP;
    }
  }
}

// -----------------------------------------------------------------------
// Input — reads joystick each loop, queues direction changes
// -----------------------------------------------------------------------
static void readInput() {
  int d = joyGetDir();
  if (d == JOY_NONE)              return;
  if (isOpposite(d, dir))         return;
  if (millis() - lastInput < INPUT_DEBOUNCE) return;
  nextDir   = d;
  lastInput = millis();
}

// -----------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  ledInit();
  joyInit();
  soundInit();
  randomSeed(analogRead(A4));  // seed RNG from floating analog pin for true randomness

  currentMode = menuRun();     // blocks here until player picks a mode
  initGame();
}

void loop() {
  readInput();

  if (!gameOver) {
    if (millis() - lastTick >= gameSpeed) {
      lastTick = millis();
      gameTick();
      if (gameOver) {
        soundMusicStop();
        ledScrollGameOver();   // blocks until player double-presses to restart
        initGame();
        return;
      }
    }
    buildPattern();
    ledDisplay(pattern);
  }
}
