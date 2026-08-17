#pragma once
#include <Arduino.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

extern Adafruit_SSD1306 display1;
extern Adafruit_SSD1306 display2;
extern Adafruit_NeoPixel strip;

extern unsigned long lastScreenUpdateTime;
extern const long screenUpdateInterval;
extern unsigned long lastBlinkTime;
extern bool blinkState;

extern int menuPage;
extern int menuSelection;
#define MAIN_MENU_ITEMS 3

extern int currentTheme;
extern int themeSelectionState;

void initDisplays();
void initLedStrip();
void updateDisplays(DateTime now);
void gererBandeLed();
void gererLedPhysiques();
void redemarrerESP32();
void gererMenuSecondaire(DateTime now);
