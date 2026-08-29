#pragma once
#include <Arduino.h>
#include <RTClib.h>

// Modes
extern int Mode;

extern bool etatCapteurBas;
extern bool etatCapteurHaut;
extern bool incoherenceCapteurs;


extern bool finitionRemplissageActive;

void setRelayState(int state);
void initPump();
void verifierCoherenceCapteurs();
void modeAUTO(DateTime now);
void modeMANU();
void modeMAINT();
void gererLogiquePompe(DateTime now);
