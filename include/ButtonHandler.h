#pragma once
#include "config.h"
#include <Bounce2.h>

extern Bounce bpAuto;
extern Bounce bpManu;
extern Bounce bpMaint;
extern Bounce bpMarche;
extern Bounce bpArret;

void initButtons();
void updateButtons();
