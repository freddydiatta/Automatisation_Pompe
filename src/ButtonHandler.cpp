#include "ButtonHandler.h"
#include <Arduino.h>

Bounce bpAuto = Bounce();
Bounce bpManu = Bounce();
Bounce bpMaint = Bounce();
Bounce bpMarche = Bounce();
Bounce bpArret = Bounce();

bool arretUrgenceActif = false;
unsigned long bp4PressStartTime = 0;
bool bp4LongPressHandled = false;
unsigned long comboStartTime = 0;

float smoothedPot1Val = 0;
#define SMOOTHING_FACTOR 0.2

void initButtons() {
  bpAuto.attach(BP1_AUTO, INPUT_PULLUP);
  bpAuto.interval(25); 

  bpManu.attach(BP2_MANU, INPUT_PULLUP);
  bpManu.interval(25);

  bpMaint.attach(BP3_MAINT, INPUT_PULLUP);
  bpMaint.interval(25);

  bpMarche.attach(BP4_MARCHE, INPUT_PULLUP);
  bpMarche.interval(25);

  bpArret.attach(BP5_ARRET, INPUT_PULLUP);
  bpArret.interval(25);
  
  pinMode(Pot1, INPUT);
  smoothedPot1Val = analogRead(Pot1);
}

void updateButtons() {
  bpAuto.update();
  bpManu.update();
  bpMaint.update();
  bpMarche.update();
  bpArret.update();
}

int getSmoothedPotValue() {
  int rawValue = analogRead(Pot1);
  smoothedPot1Val = (SMOOTHING_FACTOR * rawValue) + ((1.0 - SMOOTHING_FACTOR) * smoothedPot1Val);
  return (int)smoothedPot1Val;
}
