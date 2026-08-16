#pragma once
#include "config.h"
#include <Bounce2.h>

extern Bounce bpAuto;
extern Bounce bpManu;
extern Bounce bpMaint;
extern Bounce bpMarche;
extern Bounce bpArret;
extern Bounce potPin; // Not exactly for Bounce, but we read it.

extern bool arretUrgenceActif;
extern unsigned long bp4PressStartTime;
extern bool bp4LongPressHandled;
extern unsigned long comboStartTime;

void initButtons();
void updateButtons();
int getSmoothedPotValue();
