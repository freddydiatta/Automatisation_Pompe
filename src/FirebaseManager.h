#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

class FirebaseManager {
public:
    static FirebaseManager& getInstance() {
        static FirebaseManager instance;
        return instance;
    }

    void begin();
    void update();
    void addLog(String category, String message, String severity, String timestamp);

private:
    FirebaseManager() {}
};

#endif // FIREBASE_MANAGER_H
