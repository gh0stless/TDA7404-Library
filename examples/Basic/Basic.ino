// Basics: input, volume, tone, status
#include <Wire.h>
#include <TDA7404.h>

TDA7404 tda;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);  // TDA7404 supports up to 500 kHz

    if (!tda.begin()) {
        Serial.print("TDA7404 not found, I2C error ");
        Serial.println(tda.lastError());
        while (true) delay(1000);
    }

    tda.setInput(TDA7404::Input::SE1);
    tda.setInputGain(6);
    tda.setLoudness(5, TDA7404::LoudnessFreq::F400HZ);
    tda.setBass(4);
    tda.setBassFreq(TDA7404::BassFreq::F80HZ);
    tda.setTreble(2);
    tda.setVolume(-20);
}

void loop() {
    // Slow fade out/in via soft mute
    tda.setMute(true);
    while (tda.isMuting()) delay(1);
    delay(1000);
    tda.setMute(false);
    delay(3000);

    Serial.print("Volume: ");
    Serial.print(tda.getVolume());
    Serial.print(" dB, Bass: ");
    Serial.print(tda.getBass());
    Serial.print(" dB, Treble: ");
    Serial.print(tda.getTreble());
    Serial.println(" dB");
}
