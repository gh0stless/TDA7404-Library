# TDA7404

Arduino/PlatformIO library for the ST **TDA7404** audio processor (I2C, 7-bit address `0x44`).
All register values follow the TDA7404 datasheet Rev.3, chapter 5.

## Quick start

```cpp
#include <Wire.h>
#include <TDA7404.h>

TDA7404 tda;

void setup() {
    Wire.begin();                 // start the bus yourself (ESP32: Wire.begin(SDA, SCL))
    tda.begin();                  // writes the default set, false = chip does not respond
    tda.setInput(TDA7404::Input::SE1);
    tda.setVolume(-20);           // dB
    tda.setBass(4);               // -15..+15 dB
}
```

After `begin()`: input SE1, gain 0 dB, volume -40 dB, tone flat, all speakers at 0 dB, soft mute off.

## Functions

| Area | Functions | Values |
|---|---|---|
| Input | `setInput`, `setInputGain` | `MONO_DIFF, SE1..SE4, MUTE, BEEP`; gain 0..14, 16, 18, 20 dB |
| Volume | `setVolume`, `setVolumeLimit`, `setVolumeMute`, `setDifferentialMode` | -79..+30 dB, default limit +20 dB |
| Loudness | `setLoudness`, `setSoftStep`, `setSoftStepTime` | 0..15 dB, 400/800/2400 Hz |
| Treble | `setTreble`, `setTrebleFreq` | ±15 dB, 10/12.5/15/17.5 kHz |
| Bass | `setBass`, `setBassFreq`, `setBassQ`, `setBassDcMode` | ±15 dB, 60/80/100/200 Hz, Q 1..2 |
| Speakers | `setSpeaker`, `setSpeakerMute`, `getSpeaker`, `setBalanceFader` | 0..-23 dB in 1 dB steps, then -24.5 … -50 dB |
| Mute | `setMute`, `setMuteTime`, `setZeroCrossMute`, `isMuting` | soft mute 0.48..122.8 ms |
| Mixing | `setMixing`, `setBeep` | beep/phone, 4 levels |
| Other | `setAcCoupling`, `writeRegister`, `getRegister`, `writeAll`, `readStatus` | |

All setters return `true` if the chip acknowledged the write. Otherwise `lastError()` returns the result of `Wire.endTransmission()`.

## Notes

- The library mirrors all registers, so functions sharing a register do not overwrite each other. Example: the bass center frequency lives in the register of speaker LF.
- Every write to register 0 (input/gain) starts an AutoZero cycle, during which the chip is muted for about 0.3 ms. `writeAll()` (refresh) sets *AutoZero Remain* by default to avoid this.
- Only enable soft step volume for 1 dB steps. According to the datasheet, larger jumps can cause an error of 1 dB.
- `isMuting()` and `readStatus()` read the status byte over the bus. If the chip does not respond, the read blocks until the `Wire` timeout (ESP32: about 1 s). So do not poll them continuously in a UI loop; only query them shortly after `setMute()`. If `readStatus()` returns -1, stop polling altogether.
- The chip has no software reset. After a power-on reset (VDD < 3.5 V) all outputs stay muted until `begin()` is called.

## Examples

- `examples/Basic`: input, tone, soft mute with status query
- `examples/BalanceFader`: balance, fader and subwoofer via the serial console

## License

Public domain ([The Unlicense](LICENSE)). Do whatever you want with it, no conditions.
