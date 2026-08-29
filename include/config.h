#pragma once

// --- I2C & ECRANS ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

// --- BROCHES ---
#define LED_STRIP_PIN 25
#define LED_COUNT 22

#define relayPin 26
#define BP1_AUTO 4
#define Led_AUTO 13
#define BP2_MANU 16
#define Led_MANU 14
#define BP3_MAINT 17
#define Led_MAINT 27
#define BP4_MARCHE 12
#define BP5_ARRET 19
#define Pot1 39 // GPIO39 (VN)
#define Capteur_Niveau_Bas 18
#define Capteur_Niveau_Haut 5

// --- LOGIQUE ---
#define RELAY_ON LOW
#define RELAY_OFF HIGH
