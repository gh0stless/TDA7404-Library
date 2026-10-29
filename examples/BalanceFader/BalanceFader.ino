// Balance/Fader und Subwoofer ueber die serielle Konsole
// Befehle: b<wert>  Balance -100..100   f<wert>  Fader -100..100
//          s<wert>  Subwoofer 0..-50 dB  v<wert>  Volume -79..20 dB
#include <Wire.h>
#include <TDA7404.h>

TDA7404 tda;
int8_t balance = 0, fader = 0;

void printSpeakers() {
    const char *names[] = { "LF", "RF", "LR", "RR", "SubL", "SubR" };
    for (uint8_t i = 0; i < 6; i++) {
        Serial.print(names[i]);
        Serial.print('=');
        Serial.print(tda.getSpeaker((TDA7404::Channel)i));
        Serial.print(" dB  ");
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    if (!tda.begin()) {
        Serial.println("TDA7404 nicht gefunden");
        while (true) delay(1000);
    }
    tda.setVolume(-30);
    printSpeakers();
}

void loop() {
    if (!Serial.available()) return;
    char cmd = Serial.read();
    int value = Serial.parseInt();

    switch (cmd) {
        case 'b': balance = constrain(value, -100, 100); break;
        case 'f': fader = constrain(value, -100, 100); break;
        case 's':
            tda.setSpeaker(TDA7404::Channel::SUB_LEFT, value);
            tda.setSpeaker(TDA7404::Channel::SUB_RIGHT, value);
            break;
        case 'v': tda.setVolume(value); break;
        default: return;
    }
    tda.setBalanceFader(balance, fader);
    printSpeakers();
}
