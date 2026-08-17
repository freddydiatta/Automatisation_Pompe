#include "DisplayManager.h"
#include "ButtonHandler.h"
#include "PumpControl.h"
#include "config.h"
#include <Preferences.h>

extern Preferences preferences;
extern bool defautSecurite;
extern RTC_DS3231 rtc;

Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,
                          -1); // Temporary Wire init, will use custom I2C buses
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

TwoWire I2C_BUS_1 = TwoWire(0);
TwoWire I2C_BUS_2 = TwoWire(1);

Adafruit_NeoPixel strip(LED_COUNT, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastScreenUpdateTime = 0;
const long screenUpdateInterval = 50;
unsigned long lastBlinkTime = 0;
bool blinkState = false;

int menuPage = 0;
int menuSelection = 0;
const char *mainMenuOptions[] = {"REDEMARRER", "INFOS SYS.", "APPARENCE"};

int currentTheme = 0;
int themeSelectionState = 0;
const char *themeNames[] = {"1. Carrousel", "2. Matrice", "3. Vague",
                            "4. Minimaliste"};

unsigned long lastAnimTime = 0;
int matrixDrops[20];
float wavePhase = 0.0;
float waveAnimPhase = 0.0;

const unsigned char PROGMEM surfer_bmp[] = {0b00011000, 0b00111100, 0b00011000,
                                            0b00011000, 0b00100100, 0b01000010,
                                            0b01111110, 0b00000000};

float creditsY = SCREEN_HEIGHT;
unsigned long lastCreditsUpdate = 0;
const char *creditsLines[] = {"--- INFOS SYSTEME ---",
                              "",
                              "Projet:",
                              "Gestion Citerne V14.2",
                              "",
                              "Securite:",
                              "Standard",
                              "",
                              "DEV:",
                              "Freddy Diatta",
                              "",
                              "--- FIN ---",
                              "",
                              "",
                              ""};
const int creditsCount = sizeof(creditsLines) / sizeof(creditsLines[0]);
const int creditLineHeight = 11;

void drawHeader(const char *title) {
  display2.setTextSize(1);
  display2.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display2.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display2.setCursor((SCREEN_WIDTH - w) / 2, 0);
  display2.print(title);
  display2.drawFastHLine(0, 9, 128, SSD1306_WHITE);
}

void initDisplays() {
  I2C_BUS_1.begin(21, 22, 400000);
  I2C_BUS_2.begin(33, 32, 400000);

  // Correcting the initialisation objects since we created them with &Wire
  // previously Wait, the original code had them instantiated with &I2C_BUS_1
  // and &I2C_BUS_2 globally We can just reconstruct them here safely, or use
  // them directly.
  display1 = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_BUS_1, -1);
  display2 = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_BUS_2, -1);

  if (!display1.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    Serial.println(F("Echec Ecran 1"));
  else
    display1.display();

  if (!display2.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    Serial.println(F("Echec Ecran 2"));
  else
    display2.display();

  for (int i = 0; i < 20; i++)
    matrixDrops[i] = random(-50, 0);
}

void initLedStrip() {
  strip.begin();
  strip.show();
  strip.setBrightness(255);
}

void gererLedPhysiques() {
  if (arretUrgenceActif) {
    digitalWrite(Led_AUTO, LOW);
    digitalWrite(Led_MANU, LOW);
    digitalWrite(Led_MAINT, LOW);
    return;
  }
  digitalWrite(Led_AUTO, LOW);
  digitalWrite(Led_MANU, LOW);
  digitalWrite(Led_MAINT, LOW);
  switch (Mode) {
  case 0:
    digitalWrite(Led_AUTO, HIGH);
    break;
  case 1:
    digitalWrite(Led_MANU, HIGH);
    break;
  case 2:
    digitalWrite(Led_MAINT, HIGH);
    break;
  }
}

void gererBandeLed() {
  uint32_t couleur = strip.Color(0, 0, 0);
  if (arretUrgenceActif) {
    if (blinkState)
      couleur = strip.Color(255, 0, 0);
    else
      couleur = strip.Color(0, 0, 0);
  } else if (incoherenceCapteurs) {
    couleur = strip.Color(255, 0, 0);
  } else if (etatCapteurHaut == LOW) {
    couleur = strip.Color(0, 255, 0);
  } else if (etatCapteurBas == HIGH) {
    couleur = strip.Color(255, 0, 0);
  } else if (Mode == 2) {
    couleur = strip.Color(255, 165, 0);
  } else if (digitalRead(relayPin) == RELAY_ON) {
    couleur = strip.Color(0, 255, 255);
  } else {
    couleur = strip.Color(0, 0, 50);
  }
  for (int i = 0; i < LED_COUNT; i++)
    strip.setPixelColor(i, couleur);
  strip.show();
}

void redemarrerESP32() {
  display2.clearDisplay();
  display2.setTextSize(2);
  display2.setCursor(15, 20);
  display2.print("Redemarrage...");
  display2.display();
  delay(1500);
  ESP.restart();
}

void afficherEtatPrimaire(DateTime now) {
  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(SSD1306_WHITE);

  if (defautSecurite) {
    if (blinkState) {
      display1.setTextSize(2);
      display1.setCursor(10, 5);
      display1.print("!DEFAUT!");
      display1.setTextSize(1);
      display1.setCursor(15, 25);
      display1.print("POMPE BLOQUEE");
    }
    display1.display();
    return;
  }
  if (arretUrgenceActif) {
    display1.setTextSize(2);
    display1.setCursor(10, 5);
    display1.print("!! ARRET !!");
    display1.setCursor(10, 25);
    display1.print(" URGENCE ");
    display1.display();
    return;
  }

  if (Mode == 2) {
    display1.setTextSize(2);
    display1.setCursor((SCREEN_WIDTH - (9 * 12)) / 2, 10);
    display1.print("ENTRETIEN");
    display1.display();
    return;
  }

  display1.setCursor(0, 0);
  char timeBuffer[6];
  sprintf(timeBuffer, "%02d:%02d", now.hour(), now.minute());
  display1.print(timeBuffer);
  display1.print("   WIFI:--");
  display1.setCursor(0, 16);
  switch (Mode) {
  case 0:
    display1.println("MODE:AUTO");
    break;
  case 1:
    display1.println("MODE:MANU");
    break;
  case 2:
    display1.println("MODE:MAINT");
    break;
  default:
    display1.println("MODE:--");
    break;
  }
  display1.setCursor(0, 24);
  if (digitalRead(relayPin) == RELAY_ON) {
    if (finitionRemplissageActive)
      display1.print("P:RUN (+1m)");
    else
      display1.print("P:RUN ");
  } else
    display1.print("P:STOP ");

  if (etatCapteurHaut == LOW)
    display1.print("CIT:PLEIN");
  else if (etatCapteurBas == LOW)
    display1.print("CIT:EN C.");
  else
    display1.print("CIT:BAS!");
  display1.display();
}

void drawAnimationSurfer(DateTime now) {
  display2.clearDisplay();
  float waveSpeed = (digitalRead(relayPin) == RELAY_ON) ? 0.4 : 0.15;
  waveAnimPhase += waveSpeed;
  int surfY = 0;
  for (int x = 0; x < 128; x++) {
    int y = 22 + 4 * sin((x * 0.12) + waveAnimPhase);
    display2.drawPixel(x, y, SSD1306_WHITE);
    if (x == 64)
      surfY = y;
  }
  display2.drawBitmap(60, surfY - 8, surfer_bmp, 8, 8, SSD1306_WHITE);
  display2.setTextSize(2);
  display2.setTextColor(SSD1306_WHITE);
  if (Mode == 0) {
    display2.setCursor(40, 0);
    display2.print("AUTO");
  } else if (Mode == 1) {
    display2.setCursor(40, 0);
    display2.print("MANU");
  }
  display2.setTextSize(1);
  if (digitalRead(relayPin) == RELAY_ON) {
    display2.setCursor(100, 0);
    display2.print("ON!");
  }
}

void drawPageRedemarrer() {
  display2.clearDisplay();
  drawHeader("REDEMARRAGE");
  display2.setTextSize(1);
  display2.setTextColor(SSD1306_WHITE);
  display2.setCursor(10, 15);
  display2.print("Confirmer reset ?");
}

void drawPageInfosSystem() {
  display2.clearDisplay();
  drawHeader("INFOS SYSTEME");
  if (millis() - lastCreditsUpdate > 50) {
    creditsY -= 0.5;
    lastCreditsUpdate = millis();
  }
  if (creditsY < -(creditsCount * creditLineHeight)) {
    creditsY = SCREEN_HEIGHT;
  }
  for (int i = 0; i < creditsCount; i++) {
    float currentLineY = creditsY + (i * creditLineHeight);
    if (currentLineY >= 10 && currentLineY <= 32) {
      int16_t x1, y1;
      uint16_t w, h;
      display2.getTextBounds(creditsLines[i], 0, 0, &x1, &y1, &w, &h);
      display2.setCursor((SCREEN_WIDTH - w) / 2, currentLineY);
      display2.print(creditsLines[i]);
    }
  }
}

void drawPageThemeSelection() {
  display2.clearDisplay();
  drawHeader("THEME");
  display2.setTextSize(2);
  display2.setTextColor(SSD1306_WHITE);
  String themeName = themeNames[themeSelectionState];
  int16_t x1, y1;
  uint16_t w, h;
  display2.getTextBounds(themeName, 0, 0, &x1, &y1, &w, &h);
  display2.setCursor((SCREEN_WIDTH - w) / 2, 16);
  display2.print(themeName);
}

void drawIconGeneric(int x, int y, int type, bool selected) {
  int r = selected ? 10 : 6;
  if (selected)
    display2.fillCircle(x, y, r, SSD1306_WHITE);
  else
    display2.drawCircle(x, y, r, SSD1306_WHITE);
  uint16_t color = selected ? SSD1306_BLACK : SSD1306_WHITE;
  switch (type) {
  case 0: // REDEMARRER (Reset icon)
    display2.drawCircle(x, y, r - 2, color);
    display2.drawLine(x, y - (r), x + 2, y - (r - 2), color);
    break;
  case 1: // INFOS (Info icon)
    display2.setCursor(x - 1, y - 3);
    display2.setTextSize(1);
    display2.setTextColor(color);
    display2.print("i");
    break;
  case 2: // APPARENCE (Palette icon)
    display2.fillRect(x - 2, y - 2, 4, 4, color);
    display2.drawLine(x, y, x + 3, y + 3, color);
    break;
  }
}

void drawHomeCarousel() {
  display2.clearDisplay();
  int centerIdx = menuSelection;
  int leftIdx = (centerIdx - 1 + MAIN_MENU_ITEMS) % MAIN_MENU_ITEMS;
  int rightIdx = (centerIdx + 1) % MAIN_MENU_ITEMS;
  drawIconGeneric(20, 24, leftIdx, false);
  drawIconGeneric(108, 24, rightIdx, false);
  drawIconGeneric(64, 24, centerIdx, true);
  display2.setTextSize(1);
  display2.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display2.getTextBounds(mainMenuOptions[centerIdx], 0, 0, &x1, &y1, &w, &h);
  display2.setCursor((SCREEN_WIDTH - w) / 2, 2);
  display2.print(mainMenuOptions[centerIdx]);
  display2.fillTriangle(2, 16, 8, 10, 8, 22, SSD1306_WHITE);
  display2.fillTriangle(126, 16, 120, 10, 120, 22, SSD1306_WHITE);
}

void drawHomeMatrix() {
  display2.fillRect(0, 0, 128, 32, SSD1306_BLACK);
  if (millis() - lastAnimTime > 30) {
    for (int i = 0; i < 20; i += 2) {
      matrixDrops[i] += random(1, 4);
      if (matrixDrops[i] > 32)
        matrixDrops[i] = random(-20, 0);
      display2.drawChar(i * 6 + 2, matrixDrops[i], (char)random(33, 126),
                        SSD1306_WHITE, SSD1306_BLACK, 1);
    }
    lastAnimTime = millis();
  }
  display2.setTextSize(2);
  String text = mainMenuOptions[menuSelection];
  int16_t x1, y1;
  uint16_t w, h;
  display2.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  int y = 8;
  if (random(0, 10) > 7)
    display2.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  else
    display2.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display2.setCursor(x + random(-1, 2), y + random(-1, 2));
  display2.print(text);
  display2.setTextColor(SSD1306_WHITE);
}

void drawHomeWave() {
  display2.clearDisplay();
  if (millis() - lastAnimTime > 50) {
    wavePhase += 0.2;
    lastAnimTime = millis();
  }
  for (int x = 0; x < 128; x++) {
    int y =
        24 + 4 * sin(x * 0.1 + wavePhase) + 2 * sin(x * 0.2 + wavePhase * 1.5);
    display2.drawFastVLine(x, y, 32 - y, SSD1306_WHITE);
  }
  int spacing = 128 / MAIN_MENU_ITEMS;
  for (int i = 0; i < MAIN_MENU_ITEMS; i++) {
    int x = i * spacing + spacing / 2;
    int y = 12 + 2 * sin(i + wavePhase);
    if (i == menuSelection) {
      display2.fillCircle(x, y, 8, SSD1306_BLACK);
      display2.fillCircle(x, y, 6, SSD1306_WHITE);
      display2.setTextSize(1);
      display2.setTextColor(SSD1306_WHITE);
      int16_t x1, y1;
      uint16_t w, h;
      display2.getTextBounds(mainMenuOptions[i], 0, 0, &x1, &y1, &w, &h);
      display2.setCursor((SCREEN_WIDTH - w) / 2, 0);
      display2.print(mainMenuOptions[i]);
    } else {
      display2.drawCircle(x, y, 3, SSD1306_WHITE);
    }
  }
}

void drawHomeMinimal() {
  display2.clearDisplay();
  display2.setTextSize(2);
  display2.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display2.getTextBounds(mainMenuOptions[menuSelection], 0, 0, &x1, &y1, &w,
                         &h);
  display2.setCursor((SCREEN_WIDTH - w) / 2, 6);
  display2.print(mainMenuOptions[menuSelection]);
  int barY = 28;
  int totalWidth = 100;
  int startX = (SCREEN_WIDTH - totalWidth) / 2;
  display2.drawFastHLine(startX, barY, totalWidth, SSD1306_WHITE);
  int step = totalWidth / (MAIN_MENU_ITEMS - 1);
  for (int i = 0; i < MAIN_MENU_ITEMS; i++) {
    int x = startX + i * step;
    if (i == menuSelection) {
      display2.fillCircle(x, barY, 3, SSD1306_WHITE);
    } else {
      display2.drawPixel(x, barY, SSD1306_WHITE);
      display2.drawPixel(x, barY - 1, SSD1306_WHITE);
    }
  }
}

void drawPageAccueil() {
  switch (currentTheme) {
  case 0:
    drawHomeCarousel();
    break;
  case 1:
    drawHomeMatrix();
    break;
  case 2:
    drawHomeWave();
    break;
  case 3:
    drawHomeMinimal();
    break;
  default:
    drawHomeCarousel();
    break;
  }
}

void drawMenu(DateTime now) {
  switch (menuPage) {
  case 0:
    drawPageAccueil();
    break;
  case 1:
    drawPageRedemarrer();
    break; // index 0 in menuSelection is REDEMARRER
  case 2:
    drawPageInfosSystem();
    break; // index 1 in menuSelection is INFOS SYS.
  case 3:
    drawPageThemeSelection();
    break; // index 2 in menuSelection is APPARENCE

  default:
    drawPageAccueil();
    break;
  }
  display2.display();
}

void updateDisplays(DateTime now) {
  if (millis() - lastScreenUpdateTime >= screenUpdateInterval) {
    lastScreenUpdateTime = millis();
    afficherEtatPrimaire(now);
    if (!arretUrgenceActif) {
      if (Mode == 2)
        drawMenu(now);
      else {
        menuPage = 0;
        menuSelection = 0;
        drawAnimationSurfer(now);
        display2.display();
      }
    } else {
      display2.clearDisplay();
      display2.display();
    }
  }
}

void gererMenuSecondaire(DateTime now) {
  if (bpMarche.fell()) {
    if (menuPage == 0)
      menuSelection = (menuSelection + 1) % MAIN_MENU_ITEMS;
    else if (menuPage == 3)
      themeSelectionState = (themeSelectionState + 1) % 4;
  }

  if (bpArret.fell()) {
    if (menuPage == 0) {
      if (menuSelection == 2) { // APPARENCE
        menuPage = 3;
        themeSelectionState = currentTheme;
      } else if (menuSelection == 1) { // INFOS SYS.
        menuPage = 2;
        creditsY = SCREEN_HEIGHT;
      } else if (menuSelection == 0) { // REDEMARRER
        menuPage = 1;
      }
    } else {
      if (menuPage == 1) { // Confirmer Redémarrer
        redemarrerESP32();
      }
      if (menuPage == 3) { // Valider Theme
        currentTheme = themeSelectionState;
        preferences.putInt("theme", currentTheme);
      }
      menuPage = 0;
      menuSelection = 0;
    }
  }
}
