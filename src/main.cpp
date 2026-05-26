#include <Arduino.h>
#include "led_driver.h"
#include "joystick.h"
#include "sounds.h"

// ---- Snake config ----
#define MAX_LEN    64
#define GAME_SPEED 300  // ms per game tick

static byte snakeRow[MAX_LEN];
static byte snakeCol[MAX_LEN];
static int  snakeHead = 0;
static int  snakeLen  = 3;

static int dir     = JOY_RIGHT;
static int nextDir = JOY_RIGHT;

static byte foodRow, foodCol;
static byte pattern[8][8];
static bool gameOver = false;
static unsigned long lastTick  = 0;
static unsigned long lastInput = 0;

#define INPUT_DEBOUNCE 150  // ms — minimum time between direction changes

// ---- Food ----
static void placeFood() {
  byte r, c;
  bool occupied;
  do {
    r = random(8);
    c = random(8);
    occupied = false;
    for (int i = 0; i < snakeLen; i++) {
      int idx = ((snakeHead - i) + MAX_LEN) % MAX_LEN;
      if (snakeRow[idx] == r && snakeCol[idx] == c) { occupied = true; break; }
    }
  } while (occupied);
  foodRow = r;
  foodCol = c;
}

// ---- Snake segment queries ----
static byte headRow() { return snakeRow[snakeHead]; }
static byte headCol() { return snakeCol[snakeHead]; }

static bool occupies(byte r, byte c) {
  for (int i = 0; i < snakeLen; i++) {
    int idx = ((snakeHead - i) + MAX_LEN) % MAX_LEN;
    if (snakeRow[idx] == r && snakeCol[idx] == c) return true;
  }
  return false;
}

// ---- Init ----
static void initGame() {
  snakeLen  = 3;
  snakeHead = 2;
  dir       = JOY_RIGHT;
  nextDir   = JOY_RIGHT;
  gameOver  = false;

  snakeRow[0] = 3; snakeCol[0] = 0;
  snakeRow[1] = 3; snakeCol[1] = 1;
  snakeRow[2] = 3; snakeCol[2] = 2;

  placeFood();
  lastTick = millis();
  soundMusicStart();
}

// ---- Draw game state into pattern buffer ----
static void clearPattern() { memset(pattern, 0, sizeof(pattern)); }

static void drawSnake() {
  for (int i = 0; i < snakeLen; i++) {
    int idx = ((snakeHead - i) + MAX_LEN) % MAX_LEN;
    byte brightness = 15 - i;
    if (brightness < 4) brightness = 4;
    pattern[snakeRow[idx]][snakeCol[idx]] = brightness;
  }
}

static void drawFood() {
  if (ledFoodVisible()) {
    pattern[foodRow][foodCol] = 10;
  }
}

static void buildPattern() {
  clearPattern();
  drawSnake();
  drawFood();
}

// ---- Movement ----
static void applyDir(byte& r, byte& c, int d) {
  if      (d == JOY_UP)    r--;
  else if (d == JOY_RIGHT) c++;
  else if (d == JOY_DOWN)  r++;
  else if (d == JOY_LEFT)  c--;
}

static bool isOpposite(int a, int b) {
  return (a == JOY_UP   && b == JOY_DOWN)  ||
         (a == JOY_DOWN && b == JOY_UP)    ||
         (a == JOY_LEFT && b == JOY_RIGHT) ||
         (a == JOY_RIGHT && b == JOY_LEFT);
}

// ---- Game tick ----
static void gameTick() {
  dir = nextDir;

  byte nr = headRow();
  byte nc = headCol();
  applyDir(nr, nc, dir);

  // Wall check (byte underflow → 255 ≥ 8)
  if (nr >= 8 || nc >= 8) { gameOver = true; soundDie(); return; }

  // Self-collision check
  if (occupies(nr, nc)) { gameOver = true; soundDie(); return; }

  bool ate = (nr == foodRow && nc == foodCol);

  snakeHead = (snakeHead + 1) % MAX_LEN;
  snakeRow[snakeHead] = nr;
  snakeCol[snakeHead] = nc;

  if (ate) {
    soundEat();
    snakeLen++;
    if (snakeLen >= MAX_LEN) { gameOver = true; soundDie(); return; }
    placeFood();
  }
}

// ---- Input ----
static void readInput() {
  int d = joyGetDir();
  if (d == JOY_NONE) return;
  if (isOpposite(d, dir)) return;
  if (millis() - lastInput < INPUT_DEBOUNCE) return;

  nextDir   = d;
  lastInput = millis();
}

// ---- Setup ----
void setup() {
  Serial.begin(115200);
  ledInit();
  joyInit();
  soundInit();
  randomSeed(analogRead(A4));
  initGame();
  Serial.println("Snake! Joystick to move, button to restart on game over.");
}

// ---- Loop ----
void loop() {
  readInput();

  if (!gameOver) {
    if (millis() - lastTick >= GAME_SPEED) {
      lastTick = millis();
      gameTick();
      if (gameOver) {
        soundMusicStop();
        bool restart = ledScrollGameOver();
        if (restart) initGame();
        return;
      }
    }
    buildPattern();
    ledDisplay(pattern);
  }
}
