// Arduino Flappy Bird homage by augustzf@gmail.com
//
// Dependencies: 
// 1. Arduino board hooked up to a push button
// 2. LED matrix and MAX7219 driver chip (cheap on eBay)
// 3. LedControl Library (http://playground.arduino.cc/Main/LedControl)
// 4. Timer Library (http://playground.arduino.cc/Code/Timer)

#include "LedControl.h"
#include "Timer.h"
#include "types.h"
#include "digits.h"
#include "letters.h"

// this pin should be connected to push button with a pull-down resistor
#define BUTTON_PIN 8
#define ON_BOARD_LED_PIN 13

// these are the pins that are connected to the MAX7219.
// make sure you set these values according to your own setup
#define DIN 7
#define CLK 6
#define CS 5
#define CHIPS 1

LedControl gMatrix = LedControl(DIN, CLK, CS, CHIPS);

// gravity
const float kG = 0.005;

// button gives this much lift
const float kLift = -0.05;

Game gGame;

// this bird is stuck on col 1 & 2
const byte kXpos = 1;

// the global timer
Timer gTimer;

// timer events 
int gUpdateEvent;
int gMoveWallOneEvent;
int gMoveWallTwoEvent;

void setup() 
{
  gMatrix.shutdown(0, false);
  gMatrix.setIntensity(0, 8);
  gMatrix.clearDisplay(0);
  pinMode (BUTTON_PIN, OUTPUT);
  randomSeed(analogRead(0));
  gGame.state = STOPPED;
  gTimer.every(30, reactToUserInput);
  startGame(true);
}

void startGame(boolean doit)
{
  if (doit) {
    gGame.score = 0;
    gGame.state = STARTED;
    gGame.birdY = 0.5;
    gGame.wallOne.xpos = 7;
    gGame.wallOne.bricks = generateWall();
    gGame.wallTwo.xpos = 7;
    gGame.wallTwo.bricks = generateWall();

    gUpdateEvent = gTimer.every(50, updateBirdPosition);
    gTimer.after(2500, startWallOne);
    gTimer.after(3300, startWallTwo);
  } 
  else {
    gGame.state = STOPPED;
    gTimer.stop(gUpdateEvent);
    gTimer.stop(gMoveWallOneEvent);
    gTimer.stop(gMoveWallTwoEvent);
  }
}  

void startWallOne()
{
  gMoveWallOneEvent = gTimer.every(200, moveWallOne);
}

void startWallTwo()
{
  gMoveWallTwoEvent = gTimer.every(200, moveWallTwo);
}

void moveWallOne()
{
  moveWall(&gGame.wallOne);
}

void moveWallTwo()
{
  moveWall(&gGame.wallTwo);
}

void moveWall(Wall *wall)
{
  if (wall->xpos == 255) {
    // wall has come past screen
    eraseWall(wall, 0);
    wall->bricks = generateWall();
    wall->xpos = 7;
  }
  else if (wall->xpos < 7) {
    eraseWall(wall, wall->xpos + 1);    
  }

  drawWall(wall, wall->xpos);

  // check if the wall just slammed into the bird.
  if (wall->xpos == 2) {
    byte ypos = 7 * gGame.birdY;  
    if (wall->bricks & (0x80 >> ypos)) {
      // Boom! Game over. 
      explode();
      gameOver();
    }
    else {
      // no collision: score!
      gGame.score++;
    }
  }
  wall->xpos = wall->xpos - 1;
}

void drawWall(Wall *wall, byte x) 
{
  for (byte row = 0; row < 8; row++) {
    if (wall->bricks & (0x80 >> row)) {
      gMatrix.setLed(0, row, x, HIGH);
    }
  }
}

void eraseWall(Wall *wall, byte x) 
{
  for (byte row = 0; row < 8; row++) {
    if (wall->bricks & (0x80 >> row)) {
      gMatrix.setLed(0, row, x, LOW);
    }
  }
}

byte generateWall()
{
  // size of the hole in the wall
  byte gap = random(4, 7);

  // the hole expressed as bits
  byte punch = (1 << gap) - 1;

  // the hole's offset
  byte slide = random(1, 8 - gap);

  // the wall without the hole
  return 0xff & ~(punch << slide);
}

void reactToUserInput()
{
  static boolean old = false;
  //static unsigned long lastMillis = 0;

  boolean buttonPressed = digitalRead(BUTTON_PIN);

  if (buttonPressed) {
    if (gGame.state == STARTED) {
      //unsigned long dt = millis() - lastMillis;
      if (!old) {
        // button was not pressed last time we checked
        if (gGame.vy > 0) {
          // initial bounce
          gGame.vy = kLift;  
        }
        else {
          // keep adding lift
          gGame.vy += kLift;   
        }        
      }      
    }
    else {
      // game is not playing. start it.
      transition();
      startGame(true); 
    }
  }
  old = buttonPressed;
  digitalWrite(ON_BOARD_LED_PIN, buttonPressed);
}

void updateBirdPosition()
{
  // initial position (simulated screen size 0..1)
  static float y = 0.5;

  // apply gravity
  gGame.vy = gGame.vy + kG;

  float oldY = gGame.birdY;

  // calculate new y position
  gGame.birdY = gGame.birdY + gGame.vy;

  // peg y to top or bottom
  if (gGame.birdY > 1) {
    gGame.birdY = 1;
    gGame.vy = 0;
  }
  else if (gGame.birdY < 0) {
    gGame.birdY = 0;
    gGame.vy = 0;
  }

  // convert to screen position
  byte ypos = 7 * gGame.birdY;  

  Direction direction;
  if (abs(oldY - gGame.birdY) < 0.01) {
    direction = STRAIGHT;
  }
  else if (oldY < gGame.birdY) {
    direction = UP;
  }
  else {
    direction = DOWN;
  }
  
  drawBird(direction, ypos);
}

void drawBird(Direction direction, byte yHead)
{
  // previous position of tail and head (one pixel each)
  static byte cTail, cHead;
  byte yTail;
  yTail = constrain(yHead - direction, 0, 7);

  // erase it from old position
  gMatrix.setLed(0, cTail, kXpos, LOW);
  gMatrix.setLed(0, cHead, kXpos + 1, LOW);

  // draw it in new position
  gMatrix.setLed(0, yTail, kXpos, HIGH);
  gMatrix.setLed(0, yHead, kXpos + 1, HIGH);

  // remember current position 
  cTail = yTail;
  cHead = yHead;
}

void explode()
{
  for (int i = 0; i < 15; ++i) {
    allOn(true);
    delay(20);
    allOn(false);
    delay(20);
  }
}

void gameOver()
{
  showScore(gGame.score);
  startGame(false);
}

void allOn(boolean on) 
{
  byte val = on ? 0xff : 0;
  for (byte n = 0; n < 8; ++n) {
    gMatrix.setRow(0, n, val);
  }
}

void showScore(byte value)
{
  if (value > 99) {
    error();
    return ;
  }
  byte tens = value / 10;
  byte ones = value % 10;
  for (int row = 0; row < 8; row++) {
    byte composite = kDigits[tens][row] | (kDigits[ones][row] >> 4);
    updateFrameRow(row, composite);    
  }
}

void loop() 
{
  gTimer.update();
}

// drag out the framebuffer sideways
void transition()
{
  for (int step = 0; step < 8; step++) {
    for (int row = 0; row < 8; row++) {
      byte pixels = gGame.framebuffer[row];
      if (row % 2) {
        pixels = pixels >> 1;        
      }
      else {
        pixels = pixels << 1;      
      }
      updateFrameRow(row, pixels);
    }
    delay(50);
  }  
}

void updateFrameRow(byte row, byte pixels) 
{
  gMatrix.setRow(0, row, pixels);
  gGame.framebuffer[row] = pixels;
}

// show the letter E to signal error condition
void error()
{
  for (int row = 0; row < 8; row++) {
    gMatrix.setRow(0, row, kLetters[0][row]);
  }
}
