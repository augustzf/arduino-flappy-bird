// Arduino Flappy Bird homage by augustzf
//
// Dependencies: 
// 1. Arduino board hooked up to a push button
// 2. LED matrix and MAX7219 driver chip (cheap on eBay)
// 3. LedControl Library (http://playground.arduino.cc/Main/LedControl)
// 4. Timer Library (http://playground.arduino.cc/Code/Timer)

#include <stdio.h>
#include "LedControl.h"
#include "Timer.h"

// this pin should be connected to push button with a pull-down resistor
#define BUTTON_PIN 8

// these are the pins that are connected to the MAX7219.
// make sure you set these values according to your own setup
#define DIN 7
#define CLK 6
#define CS 5
#define CHIPS 1

LedControl gMatrix = LedControl(DIN, CLK, CS, CHIPS);

// gravity
const float kG = 0.01;

// button gives this much lift
const float kLift = -0.005;

// y velocity
float gVy = 0;

// the screen Y position of the particle (ie bird)
byte gYpos;

// this bird is stuck on col 2
const byte kXpos = 2;

Timer gTimer;

// timer events 
int gInputEvent;
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
  startGame(true);
}

void startGame(boolean doit)
{
  if (doit) {
    gInputEvent = gTimer.every(10, reactToUserInput);
    gUpdateEvent = gTimer.every(50, updateParticlePosition);
    gTimer.after(2500, startWallOne);
    gTimer.after(4500, startWallTwo);
  } 
  else {
    gTimer.stop(gInputEvent);
    gTimer.stop(gUpdateEvent);
    gTimer.stop(gMoveWallOneEvent);
    gTimer.stop(gMoveWallTwoEvent);
  }
}  

void startWallOne()
{
  gMoveWallOneEvent = gTimer.every(500, moveWallOne);
}

void startWallTwo()
{
  gMoveWallTwoEvent = gTimer.every(500, moveWallTwo);
}

void moveWallOne()
{
  static byte wall;  
  static byte xpos;
  moveWall(&wall, &xpos);
}

void moveWallTwo()
{
  static byte wall;  
  static byte xpos;
  moveWall(&wall, &xpos);
}

void moveWall(byte *wall, byte *xpos)
{
  if (!*wall || *xpos == 8) {
    // wall has come past screen
    gMatrix.setColumn(0, *xpos - 1, 0);
    *wall = generateWall();
    *xpos = 0;
  }

  // erase previous wall
  if (*xpos > 0) {
    gMatrix.setColumn(0, *xpos - 1, 0);
  }
  // draw wall at new position
  gMatrix.setColumn(0, *xpos, *wall);    

  // check if the wall just slammed into the bird.
  if (*xpos == 5) {
    if (*wall & (1 << gYpos)) {
      // Boom! Game over. Reset Arduino to play again.
      explode();
      startGame(false);
    }
  }

  *xpos = *xpos + 1;
}

byte generateWall()
{
  // size of the hole in the wall
  byte gap = random(2, 7);

  // the hole expressed as bits
  byte punch = (1 << gap) - 1;

  // the hole's offset
  byte slide = random(1, 8 - gap);

  // the wall without the hole
  return 0xff & ~(punch << slide);
}

void reactToUserInput()
{
  boolean flap = digitalRead(BUTTON_PIN);
  if (flap) {
    gVy = gVy + kLift;
  }
  digitalWrite(13, flap);
}

void updateParticlePosition()
{
  // initial position (simulated screen size 0..1)
  static float y = 0.5;

  // apply gravity
  gVy = gVy + kG;

  // calculate new y position
  y = y + gVy;

  // peg y to top or bottom
  if (y > 1) {
    y = 1;
    gVy = 0;
  }
  else if (y < 0) {
    y = 0;
    gVy = 0;
  }

  // convert to screen position
  gYpos = 7 * y;  

  // note: x and y are flipped and inverted. you will need 
  // to change this depending on the orientation of your led matrix
  moveTo(7 - gYpos, 7 - kXpos);
}

void moveTo(byte x, byte y) 
{  
  static byte cx, cy;

  // erase it from old position
  gMatrix.setLed(0, cx, cy, LOW);

  // draw it in new position
  gMatrix.setLed(0, x, y, HIGH);

  // remember current position
  cx = x;
  cy = y;
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

void allOn(boolean on) 
{
  byte val = on ? 0xff : 0;
  for (byte n = 0; n < 8; ++n) {
    gMatrix.setRow(0, n, val);
  }
}

void loop() 
{
  gTimer.update();
}

