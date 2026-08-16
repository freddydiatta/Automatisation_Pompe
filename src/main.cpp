#include <Arduino.h>
#include <Preferences.h>
#include <RTClib.h>
#include <Wire.h>

#include "ButtonHandler.h"
#include "DisplayManager.h"
#include "PumpControl.h"
#include "config.h"
#include <esp_task_wdt.h>

// Network & Remote
#include "NetworkManager.h"
#include "FirebaseManager.h"

extern bool defautSecurite;

Preferences preferences;
RTC_DS3231 rtc;

const char *motDePasseList[] = {"maman", "papa",   "oiseau",
                                "fleur", "soleil", "riviere"};
const int nbMotsDePasse = 6;
int passwordIndex = 0;
bool physicalResetJustHappened = false;

extern TwoWire I2C_BUS_1;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- DEMARRAGE SYSTEME (V2 - SANS BLUETOOTH) ---");

  preferences.begin("pompe_config", false);
  currentTheme = preferences.getInt("theme", 0);
  declenchementDuree = preferences.getInt("duree", 30);
  dernierNettoyageUnix = preferences.getULong("lastMaint", 0);
  maintenanceRequise = preferences.getBool("maintReq", false);
  passwordIndex = preferences.getInt("pwdIndex", 0);
  if (passwordIndex < 0 || passwordIndex >= nbMotsDePasse)
    passwordIndex = 0;
  if (currentTheme > 3)
    currentTheme = 0;

  // Initialisation du reseau et Firebase (Portail captif si pas de WiFi)
  NetworkManager::getInstance().begin();
  FirebaseManager::getInstance().begin();

  initDisplays();

  if (!rtc.begin(&I2C_BUS_1))
    Serial.println("ERREUR RTC");

  if (dernierNettoyageUnix == 0 && rtc.now().year() > 2020) {
    resetMaintenanceCounter(rtc.now(), false);
    Serial.println("Initialisation du compteur de maintenance.");
  }

  initLedStrip();
  initPump();
  initButtons();

  pinMode(Led_AUTO, OUTPUT);
  pinMode(Led_MANU, OUTPUT);
  pinMode(Led_MAINT, OUTPUT);

  if (maintenanceRequise) {
    Mode = 2;
  } else {
    Mode = 2; // Default to MAINTENANCE Mode (was AUTO)
  }

  arretUrgenceActif = false;
  physicalResetJustHappened = false;
  cycleHoraireEnCours = false;
  tempsDebutCycle = 0;
  finitionRemplissageActive = false;

  // Initialisation du Watchdog (30 secondes)
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 30000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&twdt_config);
  esp_task_wdt_add(NULL);
}

void loop() {
  DateTime now = rtc.now();
  esp_task_wdt_reset(); // Reset du Watchdog

  if (millis() - lastBlinkTime > 400) {
    lastBlinkTime = millis();
    blinkState = !blinkState;
  }

  updateButtons();
  verifierMaintenance(now);
  verifierCoherenceCapteurs();

  // Gestion Arret urgence reset (Appui long BP4)
  if (arretUrgenceActif) {
    if (bpMarche.read() == LOW) {
      if (bp4PressStartTime == 0)
        bp4PressStartTime = millis();
      else if (millis() - bp4PressStartTime > 1000 && !bp4LongPressHandled) {
        arretUrgenceActif = false;
        physicalResetJustHappened = true;
        bp4LongPressHandled = true;
        display1.clearDisplay();
        display1.setCursor(0, 0);
        display1.print("RESET URG OK");
        display1.display();
      }
    } else {
      bp4PressStartTime = 0;
      bp4LongPressHandled = false;
    }
  }

  // Gestion Combo maintenance secrete
  if (maintenanceRequise && bpMaint.read() == LOW && bpArret.read() == LOW) {
    if (comboStartTime == 0) {
      comboStartTime = millis();
      Serial.println("Debut combo secrete...");
    } else if (millis() - comboStartTime > 5000) {
      resetMaintenanceCounter(now, true);
      comboStartTime = 0;
    }
  } else {
    comboStartTime = 0;
  }

  // Changement de Mode si pas en maintenance bloquee
  if (!maintenanceRequise || Mode == 2) {
    if (bpAuto.fell()) {
      Mode = 0;
    }
    if (bpManu.fell()) {
      Mode = 1;
    }
    if (bpMaint.fell()) {
      Mode = 2;
      defautSecurite = false; // Acquittement du défaut de sécurité
    }
  }

  physicalResetJustHappened = false;

  gererLedPhysiques();
  gererLogiquePompe(now);
  gererBandeLed();
  gererMenuSecondaire(now);
  updateDisplays(now);

  // Envoi de la telemetrie Firebase
  FirebaseManager::getInstance().update();
  FirebaseManager::getInstance().setMode(Mode);
  // (Note: setRelayState et setWaterLevels peuvent etre appeles a l'interieur de gererLogiquePompe pour eviter d'envoyer a chaque boucle si inchangé)
}