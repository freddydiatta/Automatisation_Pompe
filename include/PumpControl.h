#pragma once
#include <Arduino.h>
#include <RTClib.h>

// Modes
extern int Mode; 
extern bool maintenanceRequise;
extern unsigned long dernierNettoyageUnix;

extern bool etatCapteurBas;
extern bool etatCapteurHaut;
extern bool incoherenceCapteurs;

extern int declenchementHeure;
extern int declenchementMinute;
extern int declenchementDuree;
extern bool timerActif;
extern bool cycleHoraireEnCours;
extern unsigned long tempsDebutCycle;

extern bool finitionRemplissageActive;

void initPump();
void verifierCoherenceCapteurs();
void verifierMaintenance(DateTime now);
void resetMaintenanceCounter(DateTime now, bool forcePhysical);
void modeAUTO(DateTime now);
void modeMANU();
void modeMAINT();
void gererLogiquePompe(DateTime now);
