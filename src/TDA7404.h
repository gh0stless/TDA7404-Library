/**
 * TDA7404 - Arduino/PlatformIO library for the ST TDA7404 audio processor (I2C).
 *
 * All register values are taken from the TDA7404 datasheet Rev.3 (chapter 5).
 * The library keeps a shadow copy of all 13 registers so that functions
 * sharing a register (e.g. speaker LF + bass center frequency) do not
 * overwrite each other.
 *
 * The library does NOT start the I2C bus - call Wire.begin() first.
 */
#ifndef TDA7404_H
#define TDA7404_H

#include <Arduino.h>
#include <Wire.h>

// 7-bit address (datasheet: 1000100 + R/W)
#define TDA7404_I2C_ADDRESS 0x44

class TDA7404 {
public:
    // ---- Subaddresses (table 7) ----
    enum Register : uint8_t {
        REG_INPUT       = 0x00,
        REG_LOUDNESS    = 0x01,
        REG_VOLUME      = 0x02,
        REG_TREBLE      = 0x03,
        REG_BASS        = 0x04,
        REG_SPK_LF      = 0x05,  // + bass center frequency in D7..D6
        REG_SPK_RF      = 0x06,
        REG_SPK_LR      = 0x07,
        REG_SPK_RR      = 0x08,
        REG_SUB_L       = 0x09,
        REG_SUB_R       = 0x0A,
        REG_MUTE_MIX    = 0x0B,
        REG_OTHERS      = 0x0C,
        REG_TESTING     = 0x0D,
        REGISTER_COUNT  = 13     // 0x00..0x0C (testing is not mirrored)
    };

    // ---- Input (table 8, D2..D0) ----
    enum class Input : uint8_t {
        MONO_DIFF = 0,  // mono differential
        SE1       = 1,
        MUTE      = 2,
        SE2       = 3,
        SE4       = 4,  // pseudo-differential / single ended 4
        SE3       = 5,
        MUTE2     = 6,
        BEEP      = 7
    };

    // ---- Speaker channels ----
    enum class Channel : uint8_t {
        LEFT_FRONT = 0, RIGHT_FRONT, LEFT_REAR, RIGHT_REAR, SUB_LEFT, SUB_RIGHT
    };

    // ---- Loudness (table 9) ----
    enum class LoudnessFreq : uint8_t { FLAT = 0, F400HZ = 1, F800HZ = 2, F2400HZ = 3 };
    enum class LoudnessShape : uint8_t { LOW_BOOST = 0, LOW_HIGH_BOOST = 1 };

    // ---- Treble (table 11) ----
    enum class TrebleFreq : uint8_t { F10KHZ = 0, F12K5HZ = 1, F15KHZ = 2, F17K5HZ = 3 };

    // ---- Bass (tables 12 / 13) ----
    enum class BassQ : uint8_t { Q1_0 = 0, Q1_25 = 1, Q1_5 = 2, Q2_0 = 3 };
    enum class BassFreq : uint8_t { F60HZ = 0, F80HZ = 1, F100HZ = 2, F200HZ = 3 };

    // ---- Soft mute / mixing (table 18) ----
    enum class MuteTime : uint8_t { T0_48MS = 0, T0_96MS = 1, T30_7MS = 2, T122_8MS = 3 };
    enum class MixSource : uint8_t { BEEP = 0, PHONE = 1 };
    enum class MixLevel : uint8_t {
        MAIN_M12_MIX_M2_5 = 0,  // main -12 dB / mix -2.5 dB
        MAIN_M6_MIX_M6    = 1,  // main  -6 dB / mix -6 dB
        MAIN_M3_5_MIX_M9_6 = 2, // main -3.5 dB / mix -9.6 dB
        OFF               = 3   // main 0 dB / mix off
    };
    enum class BeepFreq : uint8_t { F781HZ = 0, F1560HZ = 1, F1800HZ = 3 };

    // ---- Others (table 19) ----
    enum class AcCoupling : uint8_t {
        INTERNAL_PASS  = 0,  // internal connection, no external AC coupling
        MIX_TO_VOLUME  = 1,  // external coupling between mixing and volume
        BASS_TO_SPEAKER = 3  // external coupling between bass filter and speaker attenuator
    };
    enum class SoftStepTime : uint8_t { T0_68MS = 0, T1_26MS = 1, T2_52MS = 2, T5_04MS = 3 };

    // ---- Value ranges ----
    static const int8_t VOLUME_MIN_DB  = -79;
    static const int8_t VOLUME_MAX_DB  = 30;  // datasheet recommends max. +20 dB
    static const int8_t TONE_MAX_DB    = 15;  // bass/treble ±15 dB
    static const int8_t SPEAKER_MIN_DB = -50;

    TDA7404();

    /**
     * Checks whether the chip responds and writes the complete default set.
     * Afterwards: input SE1, 0 dB gain, volume -40 dB, tone flat, all
     * speakers 0 dB, soft mute off.
     */
    bool begin(TwoWire &wire = Wire, uint8_t address = TDA7404_I2C_ADDRESS);

    // Does the chip respond on the bus?
    bool isConnected();

    // Write all 13 registers from the shadow copy using auto-increment.
    // keepAutoZero = true: no new AutoZero cycle (no short mute).
    bool writeAll(bool keepAutoZero = true);

    // ---- Input ----
    bool setInput(Input source);
    bool setInputGain(uint8_t gainDb);  // 0..14, 16, 18, 20 dB (rounded)
    Input getInput() const;
    uint8_t getInputGain() const;       // in dB

    // ---- Volume ----
    bool setVolume(int8_t db);          // -79..+30 dB, limited by setVolumeLimit()
    int8_t getVolume() const;
    void setVolumeLimit(int8_t maxDb);  // default +20 dB
    bool setVolumeMute(bool mute);      // hard mute in the volume stage
    bool setDifferentialMode(bool diff);// diffin mode: differential stereo

    // ---- Loudness ----
    bool setLoudness(uint8_t attenuationDb, LoudnessFreq freq = LoudnessFreq::F400HZ,
                     LoudnessShape shape = LoudnessShape::LOW_BOOST);  // 0..15 dB
    bool setSoftStep(bool enable);      // soft step volume (only useful for 1 dB steps)

    // ---- Tone ----
    bool setTreble(int8_t db);          // -15..+15 dB
    bool setTrebleFreq(TrebleFreq freq);
    bool setBass(int8_t db);            // -15..+15 dB
    bool setBassFreq(BassFreq freq);
    bool setBassQ(BassQ q);
    bool setBassDcMode(bool enable);    // +4.4 dB DC gain, fc and Q -25 %
    int8_t getTreble() const;
    int8_t getBass() const;

    // ---- Speakers / subwoofer ----
    bool setSpeaker(Channel ch, int8_t db);  // 0..-50 dB (coarser steps below -24 dB)
    bool setSpeakerMute(Channel ch, bool mute);
    float getSpeaker(Channel ch) const;      // actual value in dB
    // Balance (-100 = left only, +100 = right only) and fader (-100 = rear only,
    // +100 = front only) - applied to the four main speakers.
    bool setBalanceFader(int8_t balance, int8_t fader);

    // ---- Soft mute ----
    bool setMute(bool mute);            // soft mute via I2C
    bool setMuteTime(MuteTime t);
    bool setZeroCrossMute(bool zeroCross); // false = soft mute (default), true = zero cross
    bool isMuting();                    // status bit: mute/unmute in progress

    // ---- Mixing / beep ----
    bool setMixing(MixSource src, MixLevel level);
    bool setBeep(bool enable, BeepFreq freq = BeepFreq::F1560HZ);

    // ---- Others ----
    bool setAcCoupling(AcCoupling mode);
    bool setSoftStepTime(SoftStepTime t);

    // ---- Low level ----
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(uint8_t reg) const;
    int8_t readStatus();                // status byte, -1 on error
    uint8_t lastError() const { return _lastError; }  // return value of endTransmission()

    // Helper for displaying values
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
