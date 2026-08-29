#include "ButtonHandler.h"
#include <Arduino.h>

Bounce bpAuto = Bounce();
Bounce bpManu = Bounce();
Bounce bpMaint = Bounce();
Bounce bpMarche = Bounce();
Bounce bpArret = Bounce();

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
}

void updateButtons() {
  bpAuto.update();
  bpManu.update();
  bpMaint.update();
  bpMarche.update();
  bpArret.update();
}
