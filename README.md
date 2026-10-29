# TDA7404

Arduino/PlatformIO-Library für den ST **TDA7404** Audioprozessor (I2C, 7-Bit-Adresse `0x44`).
Alle Registerwerte nach Datenblatt TDA7404 Rev.3, Kapitel 5.

## Schnellstart

```cpp
#include <Wire.h>
#include <TDA7404.h>

TDA7404 tda;

void setup() {
    Wire.begin();                 // Bus selbst starten (ESP32: Wire.begin(SDA, SCL))
    tda.begin();                  // schreibt den Default-Satz, false = Chip antwortet nicht
    tda.setInput(TDA7404::Input::SE1);
    tda.setVolume(-20);           // dB
    tda.setBass(4);               // -15..+15 dB
}
```

Nach `begin()` gilt: Eingang SE1, Gain 0 dB, Volume -40 dB, Klang neutral, alle Lautsprecher auf 0 dB, Soft-Mute aus.

## Funktionen

| Bereich | Funktionen | Werte |
|---|---|---|
| Eingang | `setInput`, `setInputGain` | `MONO_DIFF, SE1..SE4, MUTE, BEEP`; Gain 0..14, 16, 18, 20 dB |
| Lautstärke | `setVolume`, `setVolumeLimit`, `setVolumeMute`, `setDifferentialMode` | -79..+30 dB, Default-Limit +20 dB |
| Loudness | `setLoudness`, `setSoftStep`, `setSoftStepTime` | 0..15 dB, 400/800/2400 Hz |
| Höhen | `setTreble`, `setTrebleFreq` | ±15 dB, 10/12.5/15/17.5 kHz |
| Bass | `setBass`, `setBassFreq`, `setBassQ`, `setBassDcMode` | ±15 dB, 60/80/100/200 Hz, Q 1..2 |
| Lautsprecher | `setSpeaker`, `setSpeakerMute`, `getSpeaker`, `setBalanceFader` | 0..-23 dB in 1-dB-Schritten, danach -24.5 … -50 dB |
| Mute | `setMute`, `setMuteTime`, `setZeroCrossMute`, `isMuting` | Soft-Mute 0.48..122.8 ms |
| Mixing | `setMixing`, `setBeep` | Beep/Phone, 4 Pegel |
| Sonstiges | `setAcCoupling`, `writeRegister`, `getRegister`, `writeAll`, `readStatus` | |

Alle Setter geben `true` zurück, wenn der Chip den Schreibzugriff quittiert hat. `lastError()` liefert sonst den Rückgabewert von `Wire.endTransmission()`.

## Hinweise

- Die Library spiegelt alle Register. Funktionen, die sich ein Register teilen, überschreiben sich deshalb nicht gegenseitig. Ein Beispiel: Die Bass-Mittenfrequenz liegt im Register des Lautsprechers LF.
- Bei jedem Schreiben in Register 0 (Eingang/Gain) startet der Chip einen AutoZero-Zyklus und ist dabei ca. 0.3 ms stumm. `writeAll()` (Refresh) setzt standardmäßig *AutoZero Remain*, um das zu vermeiden.
- Soft-Step-Volume nur bei 1-dB-Schritten einschalten. Bei größeren Sprüngen kann laut Datenblatt ein Fehler von 1 dB entstehen.
- Der Chip hat keinen Software-Reset. Nach dem Power-On-Reset (VDD < 3.5 V) sind alle Ausgänge stumm, bis `begin()` aufgerufen wird.

## Beispiele

- `examples/Basic`: Eingang, Klang, Soft-Mute mit Statusabfrage
- `examples/BalanceFader`: Balance, Fader und Subwoofer über die serielle Konsole
