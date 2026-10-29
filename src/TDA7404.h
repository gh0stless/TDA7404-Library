/**
 * TDA7404 – Arduino/PlatformIO-Library für den ST TDA7404 Audioprozessor (I2C).
 *
 * Alle Registerwerte stammen aus dem Datenblatt TDA7404 Rev.3 (Kap. 5).
 * Die Library hält eine Schattenkopie aller 13 Register, damit Funktionen,
 * die sich ein Register teilen (z. B. Lautsprecher LF + Bass-Mittenfrequenz),
 * sich nicht gegenseitig überschreiben.
 *
 * Der I2C-Bus wird NICHT von der Library gestartet – vorher Wire.begin() aufrufen.
 */
#ifndef TDA7404_H
#define TDA7404_H

#include <Arduino.h>
#include <Wire.h>

// 7-Bit-Adresse (Datenblatt: 1000100 + R/W)
#define TDA7404_I2C_ADDRESS 0x44

class TDA7404 {
public:
    // ---- Subadressen (Tabelle 7) ----
    enum Register : uint8_t {
        REG_INPUT       = 0x00,
        REG_LOUDNESS    = 0x01,
        REG_VOLUME      = 0x02,
        REG_TREBLE      = 0x03,
        REG_BASS        = 0x04,
        REG_SPK_LF      = 0x05,  // + Bass-Mittenfrequenz in D7..D6
        REG_SPK_RF      = 0x06,
        REG_SPK_LR      = 0x07,
        REG_SPK_RR      = 0x08,
        REG_SUB_L       = 0x09,
        REG_SUB_R       = 0x0A,
        REG_MUTE_MIX    = 0x0B,
        REG_OTHERS      = 0x0C,
        REG_TESTING     = 0x0D,
        REGISTER_COUNT  = 13     // 0x00..0x0C (Testing wird nicht gespiegelt)
    };

    // ---- Eingang (Tabelle 8, D2..D0) ----
    enum class Input : uint8_t {
        MONO_DIFF = 0,  // Mono differentiell
        SE1       = 1,
        MUTE      = 2,
        SE2       = 3,
        SE4       = 4,  // Pseudo-differentiell / Single Ended 4
        SE3       = 5,
        MUTE2     = 6,
        BEEP      = 7
    };

    // ---- Lautsprecherkanäle ----
    enum class Channel : uint8_t {
        LEFT_FRONT = 0, RIGHT_FRONT, LEFT_REAR, RIGHT_REAR, SUB_LEFT, SUB_RIGHT
    };

    // ---- Loudness (Tabelle 9) ----
    enum class LoudnessFreq : uint8_t { FLAT = 0, F400HZ = 1, F800HZ = 2, F2400HZ = 3 };
    enum class LoudnessShape : uint8_t { LOW_BOOST = 0, LOW_HIGH_BOOST = 1 };

    // ---- Treble (Tabelle 11) ----
    enum class TrebleFreq : uint8_t { F10KHZ = 0, F12K5HZ = 1, F15KHZ = 2, F17K5HZ = 3 };

    // ---- Bass (Tabelle 12 / 13) ----
    enum class BassQ : uint8_t { Q1_0 = 0, Q1_25 = 1, Q1_5 = 2, Q2_0 = 3 };
    enum class BassFreq : uint8_t { F60HZ = 0, F80HZ = 1, F100HZ = 2, F200HZ = 3 };

    // ---- Soft-Mute / Mixing (Tabelle 18) ----
    enum class MuteTime : uint8_t { T0_48MS = 0, T0_96MS = 1, T30_7MS = 2, T122_8MS = 3 };
    enum class MixSource : uint8_t { BEEP = 0, PHONE = 1 };
    enum class MixLevel : uint8_t {
        MAIN_M12_MIX_M2_5 = 0,  // Main -12 dB / Mix -2.5 dB
        MAIN_M6_MIX_M6    = 1,  // Main  -6 dB / Mix -6 dB
        MAIN_M3_5_MIX_M9_6 = 2, // Main -3.5 dB / Mix -9.6 dB
        OFF               = 3   // Main 0 dB / Mix aus
    };
    enum class BeepFreq : uint8_t { F781HZ = 0, F1560HZ = 1, F1800HZ = 3 };

    // ---- Others (Tabelle 19) ----
    enum class AcCoupling : uint8_t {
        INTERNAL_PASS  = 0,  // interne Verbindung, kein externes AC-Coupling
        MIX_TO_VOLUME  = 1,  // ext. Kopplung zwischen Mixing und Volume
        BASS_TO_SPEAKER = 3  // ext. Kopplung zwischen Bassfilter und Lautsprecher-Abschwächer
    };
    enum class SoftStepTime : uint8_t { T0_68MS = 0, T1_26MS = 1, T2_52MS = 2, T5_04MS = 3 };

    // ---- Wertebereiche ----
    static const int8_t VOLUME_MIN_DB  = -79;
    static const int8_t VOLUME_MAX_DB  = 30;  // Datenblatt empfiehlt max. +20 dB
    static const int8_t TONE_MAX_DB    = 15;  // Bass/Treble ±15 dB
    static const int8_t SPEAKER_MIN_DB = -50;

    TDA7404();

    /**
     * Prüft, ob der Chip antwortet, und schreibt den kompletten Default-Satz.
     * Danach: Eingang SE1, 0 dB Gain, Volume -40 dB, Klang neutral, alle
     * Lautsprecher 0 dB, Soft-Mute aus.
     */
    bool begin(TwoWire &wire = Wire, uint8_t address = TDA7404_I2C_ADDRESS);

    // Antwortet der Chip auf dem Bus?
    bool isConnected();

    // Alle 13 Register aus der Schattenkopie per Auto-Increment schreiben.
    // keepAutoZero = true: kein neuer AutoZero-Zyklus (kein kurzes Muten).
    bool writeAll(bool keepAutoZero = true);

    // ---- Eingang ----
    bool setInput(Input source);
    bool setInputGain(uint8_t gainDb);  // 0..14, 16, 18, 20 dB (wird gerundet)
    Input getInput() const;
    uint8_t getInputGain() const;       // in dB

    // ---- Lautstärke ----
    bool setVolume(int8_t db);          // -79..+30 dB, begrenzt durch setVolumeLimit()
    int8_t getVolume() const;
    void setVolumeLimit(int8_t maxDb);  // Default +20 dB
    bool setVolumeMute(bool mute);      // hartes Mute in der Volume-Stufe
    bool setDifferentialMode(bool diff);// Diffin-Mode: Differential Stereo

    // ---- Loudness ----
    bool setLoudness(uint8_t attenuationDb, LoudnessFreq freq = LoudnessFreq::F400HZ,
                     LoudnessShape shape = LoudnessShape::LOW_BOOST);  // 0..15 dB
    bool setSoftStep(bool enable);      // Soft-Step-Volume (nur für 1-dB-Schritte sinnvoll)

    // ---- Klang ----
    bool setTreble(int8_t db);          // -15..+15 dB
    bool setTrebleFreq(TrebleFreq freq);
    bool setBass(int8_t db);            // -15..+15 dB
    bool setBassFreq(BassFreq freq);
    bool setBassQ(BassQ q);
    bool setBassDcMode(bool enable);    // +4.4 dB DC-Gain, fc und Q -25 %
    int8_t getTreble() const;
    int8_t getBass() const;

    // ---- Lautsprecher / Subwoofer ----
    bool setSpeaker(Channel ch, int8_t db);  // 0..-50 dB (ab -24 dB gröbere Stufen)
    bool setSpeakerMute(Channel ch, bool mute);
    float getSpeaker(Channel ch) const;      // tatsächlicher Wert in dB
    // Balance (-100 = nur links, +100 = nur rechts) und Fader (-100 = nur hinten,
    // +100 = nur vorne) – wirken auf die vier Hauptlautsprecher.
    bool setBalanceFader(int8_t balance, int8_t fader);

    // ---- Soft-Mute ----
    bool setMute(bool mute);            // Soft-Mute über I2C
    bool setMuteTime(MuteTime t);
    bool setZeroCrossMute(bool zeroCross); // false = Soft-Mute (Default), true = Zero-Cross
    bool isMuting();                    // Statusbit: Mute-/Demute-Vorgang läuft

    // ---- Mixing / Beep ----
    bool setMixing(MixSource src, MixLevel level);
    bool setBeep(bool enable, BeepFreq freq = BeepFreq::F1560HZ);

    // ---- Others ----
    bool setAcCoupling(AcCoupling mode);
    bool setSoftStepTime(SoftStepTime t);

    // ---- Low-Level ----
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(uint8_t reg) const;
    int8_t readStatus();                // Statusbyte, -1 bei Fehler
    uint8_t lastError() const { return _lastError; }  // Rückgabe von endTransmission()

    // Hilfsfunktionen für Anzeige
    static float speakerStepToDb(uint8_t step);

private:
    bool update(uint8_t reg, uint8_t mask, uint8_t value);
    uint8_t subaddress(uint8_t reg, bool autoInc, bool keepAutoZero) const;
    static uint8_t speakerReg(Channel ch);
    static uint8_t speakerDbToStep(int8_t db);
    static uint8_t toneToBits(int8_t db);
    static int8_t bitsToTone(uint8_t bits);

    TwoWire *_wire = nullptr;
    uint8_t _address = TDA7404_I2C_ADDRESS;
    uint8_t _reg[REGISTER_COUNT];
    int8_t _volumeLimit = 20;
    int8_t _volumeDb = -40;
    bool _volumeMuted = false;
    bool _zeroCross = false;
    uint8_t _lastError = 0;
};

#endif
