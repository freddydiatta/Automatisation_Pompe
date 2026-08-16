#include "PumpControl.h"
#include "config.h"
#include "ButtonHandler.h"
#include <Preferences.h>

extern Preferences preferences;

int Mode = 0;
bool maintenanceRequise = false;
unsigned long dernierNettoyageUnix = 0;
extern int passwordIndex;

bool etatCapteurBas = HIGH;
bool etatCapteurHaut = HIGH;
bool incoherenceCapteurs = false;

// Sécurité matérielle
bool defautSecurite = false;
const unsigned long DUREE_REMPLISSAGE_MAX = 900000; // 15 min
unsigned long tempsMarchePompe = 0;
const unsigned long DELAI_MIN_OFF = 30000; // 30 sec
unsigned long lastPumpOffTime = 0;

unsigned long timerStabiliteHaut = 0;
bool signalHautStable = HIGH;

unsigned long timerStabiliteBas = 0;
bool signalBasStable = HIGH;

const unsigned long DELAI_FILTRE = 3000;

bool finitionRemplissageActive = false;
unsigned long debutFinitionTime = 0;
const unsigned long DUREE_FINITION = 60000;

int declenchementHeure = 7;
int declenchementMinute = 0;
int declenchementDuree = 30;
bool timerActif = false;
bool cycleHoraireEnCours = false;
unsigned long tempsDebutCycle = 0;

unsigned long lastMaintenanceCheckTime = 0;
extern bool physicalResetJustHappened;

bool pompeManuelleActive = false;

void setRelayState(int state) {
  if (state == RELAY_ON) {
    if (defautSecurite) return; // Verrouillage de sécurité
    // Anti court-cycle
    if (lastPumpOffTime > 0 && (millis() - lastPumpOffTime < DELAI_MIN_OFF)) {
      return; // On ignore la demande d'allumage tant que le délai n'est pas écoulé
    }
    if (digitalRead(relayPin) != RELAY_ON) {
      digitalWrite(relayPin, RELAY_ON);
      tempsMarchePompe = millis(); // Démarrage du chrono de sécurité
    }
  } else {
    if (digitalRead(relayPin) != RELAY_OFF) {
      digitalWrite(relayPin, RELAY_OFF);
      lastPumpOffTime = millis();
    }
  }
}

void initPump() {
  pinMode(relayPin, OUTPUT);
  pinMode(Capteur_Niveau_Bas, INPUT_PULLUP);
  pinMode(Capteur_Niveau_Haut, INPUT_PULLUP);
  digitalWrite(relayPin, RELAY_OFF);
  lastPumpOffTime = millis(); // Init pour permettre un démarrage immédiat ou différé selon logique
}

void verifierCoherenceCapteurs() {
  bool lectureBasInstable = digitalRead(Capteur_Niveau_Bas);
  bool lectureHautInstable = digitalRead(Capteur_Niveau_Haut);

  // Filtre anti-parasite Haut
  if (lectureHautInstable == LOW && signalHautStable == HIGH) {
    if (timerStabiliteHaut == 0) {
      timerStabiliteHaut = millis();
    } else if (millis() - timerStabiliteHaut > DELAI_FILTRE) {
      signalHautStable = LOW;
      timerStabiliteHaut = 0;
    }
  } else if (lectureHautInstable == HIGH) {
    signalHautStable = HIGH;
    timerStabiliteHaut = 0;
  }

  // Filtre anti-parasite Bas
  if (lectureBasInstable == LOW && signalBasStable == HIGH) {
    if (timerStabiliteBas == 0) {
      timerStabiliteBas = millis();
    } else if (millis() - timerStabiliteBas > DELAI_FILTRE) {
      signalBasStable = LOW;
      timerStabiliteBas = 0;
    }
  } else if (lectureBasInstable == HIGH) {
    signalBasStable = HIGH;
    timerStabiliteBas = 0;
  }

  etatCapteurHaut = signalHautStable;
  etatCapteurBas = signalBasStable;

  if (etatCapteurHaut == LOW && etatCapteurBas == HIGH)
    incoherenceCapteurs = true;
  else
    incoherenceCapteurs = false;
}

void verifierMaintenance(DateTime now) {
  if (millis() - lastMaintenanceCheckTime > 60000) {
    lastMaintenanceCheckTime = millis();
    if (!maintenanceRequise && now.year() > 2020 && dernierNettoyageUnix > 0) {
      unsigned long tempsEcoule = now.unixtime() - dernierNettoyageUnix;
      if (tempsEcoule >= MAINTENANCE_INTERVAL_SEC) {
        maintenanceRequise = true;
        preferences.putBool("maintReq", true);
        Mode = 2;
        setRelayState(RELAY_OFF);
        Serial.println("ALERTE: Maintenance requise !");
      }
    }
  }
}

void resetMaintenanceCounter(DateTime now, bool forcePhysical) {
  dernierNettoyageUnix = now.unixtime();
  maintenanceRequise = false;
  preferences.putULong("lastMaint", dernierNettoyageUnix);
  preferences.putBool("maintReq", maintenanceRequise);
  if (!forcePhysical) {
    passwordIndex = (passwordIndex + 1) % nbMotsDePasse;
    preferences.putInt("pwdIndex", passwordIndex);
  }
  Serial.println("Compteur maintenance remis a zero.");
}

void modeAUTO(DateTime now) {
  if (maintenanceRequise) {
    setRelayState(RELAY_OFF);
    return;
  }

  if (incoherenceCapteurs) {
    setRelayState(RELAY_OFF);
    if (cycleHoraireEnCours) {
      cycleHoraireEnCours = false;
      timerActif = false;
    }
    finitionRemplissageActive = false;
    return;
  }

  if (etatCapteurHaut == LOW) {
    if (!finitionRemplissageActive) {
      finitionRemplissageActive = true;
      debutFinitionTime = millis();
      setRelayState(RELAY_ON);
    } else {
      if (millis() - debutFinitionTime >= DUREE_FINITION) {
        setRelayState(RELAY_OFF);
        if (cycleHoraireEnCours) {
          cycleHoraireEnCours = false;
          timerActif = false;
        }
      } else {
        setRelayState(RELAY_ON);
      }
    }
    return;
  } else {
    finitionRemplissageActive = false;
  }

  if (timerActif) {
    if (!cycleHoraireEnCours && now.hour() == declenchementHeure && now.minute() == declenchementMinute) {
      setRelayState(RELAY_ON);
      cycleHoraireEnCours = true;
      tempsDebutCycle = millis();
    }
    if (cycleHoraireEnCours) {
      unsigned long dureeEnMillisecondes = declenchementDuree * 60000UL;
      if (millis() - tempsDebutCycle >= dureeEnMillisecondes) {
        setRelayState(RELAY_OFF);
        cycleHoraireEnCours = false;
        timerActif = false;
      }
      return;
    }
  }

  if (etatCapteurBas == HIGH) {
    setRelayState(RELAY_ON);
  } else if (digitalRead(relayPin) == RELAY_ON && etatCapteurBas == LOW) {
    // continue
  } else {
    setRelayState(RELAY_OFF);
  }
}

void modeMANU() {
  if (maintenanceRequise) {
    setRelayState(RELAY_OFF);
    pompeManuelleActive = false;
    return;
  }

  if (bpArret.read() == LOW) {
    setRelayState(RELAY_OFF);
    pompeManuelleActive = false;
    finitionRemplissageActive = false;
    return;
  }

  if (bpMarche.read() == LOW) {
    pompeManuelleActive = true;
  }

  if (etatCapteurHaut == LOW) {
    if (pompeManuelleActive || digitalRead(relayPin) == RELAY_ON) {
      if (!finitionRemplissageActive) {
        finitionRemplissageActive = true;
        debutFinitionTime = millis();
      }
      if (millis() - debutFinitionTime >= DUREE_FINITION) {
        setRelayState(RELAY_OFF);
        pompeManuelleActive = false;
        return;
      } else {
        setRelayState(RELAY_ON);
        return;
      }
    } else {
      setRelayState(RELAY_OFF);
      return;
    }
  } else {
    finitionRemplissageActive = false;
  }

  if (pompeManuelleActive)
    setRelayState(RELAY_ON);
  else
    setRelayState(RELAY_OFF);
}

void modeMAINT() {
  setRelayState(RELAY_OFF);
  pompeManuelleActive = false;
}

void gererLogiquePompe(DateTime now) {
  // 1. Gestion des arrêts d'urgence et défauts sécurités prioritaires
  if (arretUrgenceActif || defautSecurite) {
    setRelayState(RELAY_OFF);
    pompeManuelleActive = false;
    cycleHoraireEnCours = false;
    finitionRemplissageActive = false;
    return; // On bloque l'exécution des autres modes
  }
  
  // 2. Vérification du timeout matériel de la pompe
  if (digitalRead(relayPin) == RELAY_ON) {
    if (millis() - tempsMarchePompe > DUREE_REMPLISSAGE_MAX) {
      defautSecurite = true; // Latch le défaut
      Serial.println("DEFAUT SECURITE: Timeout de remplissage MAX atteint ! Pompe coupee.");
      setRelayState(RELAY_OFF);
      pompeManuelleActive = false;
      cycleHoraireEnCours = false;
      finitionRemplissageActive = false;
      return;
    }
  }

  // 3. Basculement si maintenance requise
  if (maintenanceRequise) {
    Mode = 2;
    setRelayState(RELAY_OFF);
  }

  // 4. Exécution du mode actif
  switch (Mode) {
  case 0:
    modeAUTO(now);
    break;
  case 1:
    modeMANU();
    break;
  case 2:
    modeMAINT();
    break;
  default:
    setRelayState(RELAY_OFF);
    Mode = 0;
    break;
  }
}
