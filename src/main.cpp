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
#include "FirebaseManager.h"
#include "WifiConnectionManager.h"

extern bool defautSecurite;

Preferences preferences;
RTC_DS3231 rtc;

extern TwoWire I2C_BUS_1;

String getTimestamp(DateTime now) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d/%02d %02d:%02d", now.day(), now.month(),
           now.hour(), now.minute());
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- DEMARRAGE SYSTEME (V2 - SANS BLUETOOTH) ---");

  preferences.begin("pompe_config", false);
  currentTheme = preferences.getInt("theme", 0);

  if (currentTheme > 3)
    currentTheme = 0;

  // Initialisation du reseau et Firebase (Portail captif si pas de WiFi)
  WifiConnectionManager::getInstance().begin();
  FirebaseManager::getInstance().begin();

  initDisplays();

  if (!rtc.begin(&I2C_BUS_1))
    Serial.println("ERREUR RTC");
  else {
    // Synchronisation automatique de l'heure via Internet (NTP) pour le Sénégal
    // (UTC+0)
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Attente de l'heure NTP...");
    time_t nowSecs = time(nullptr);
    int retry = 0;
    while (nowSecs < 100000 && retry < 10) { // attend max 5 sec
      delay(500);
      Serial.print(".");
      nowSecs = time(nullptr);
      retry++;
    }
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                          timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min,
                          timeinfo.tm_sec));
      Serial.println("\nHeure RTC mise à jour via Internet !");
    } else {
      Serial.println("\nEchec NTP, on conserve l'heure du module RTC.");
    }
  }

  initLedStrip();
  initPump();
  initButtons();

  pinMode(Led_AUTO, OUTPUT);
  pinMode(Led_MANU, OUTPUT);
  pinMode(Led_MAINT, OUTPUT);

  Mode = 0; // Default to AUTO Mode

  finitionRemplissageActive = false;

  // Initialisation du Watchdog (30 secondes)
  // Note: API v2.x du core Arduino ESP32 (secondes, pas de config struct)
  // car platformio.ini force espressif32 @ ^6.5.0 pour compatibilite avec
  // la librairie Firebase ESP32 Client.
  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);

  FirebaseManager::getInstance().addLog(
      "Système", "Démarrage du boîtier de contrôle ESP32", "info",
      getTimestamp(rtc.now()));
}

void loop() {
  DateTime now = rtc.now();
  esp_task_wdt_reset(); // Reset du Watchdog

  if (millis() - lastBlinkTime > 400) {
    lastBlinkTime = millis();
    blinkState = !blinkState;
  }

  updateButtons();

  verifierCoherenceCapteurs();

  if (bpAuto.fell()) {
    Mode = 0;
    FirebaseManager::getInstance().addLog("Mode", "Passage en mode Automatique",
                                          "info", getTimestamp(now));
  }
  if (bpManu.fell()) {
    Mode = 1;
    FirebaseManager::getInstance().addLog("Mode", "Passage en mode Manuel",
                                          "info", getTimestamp(now));
  }
  if (bpMaint.fell()) {
    Mode = 2;
    defautSecurite = false; // Acquittement du défaut de sécurité
    FirebaseManager::getInstance().setFault("NONE");
    FirebaseManager::getInstance().addLog("Mode", "Passage en mode Entretien",
                                          "info", getTimestamp(now));
  }

  gererLedPhysiques();
  gererLogiquePompe(now);
  gererBandeLed();
  gererMenuSecondaire(now);
  updateDisplays(now);

  // Envoi de la telemetrie Firebase (throttle a 1x/seconde en interne)
  FirebaseManager::getInstance().update();
}