#include "TDA7404.h"
#include <string.h>

// Bits im Subadress-Byte (Tabelle 7)
#define SUB_SOFT_MUTE     0x80  // I3: 1 = Soft-Mute, 0 = Zero-Cross
#define SUB_AUTOZERO_KEEP 0x40  // I2: AutoZero Remain
#define SUB_TESTMODE      0x20  // I1
#define SUB_AUTO_INC      0x10  // I0

// Default-Registersatz, der in begin() geschrieben wird
static const uint8_t DEFAULTS[TDA7404::REGISTER_COUNT] = {
    0x01,  // Input: SE1, Gain 0 dB
    0x00,  // Loudness: flat, 0 dB, Soft-Step aus
    0x48,  // Volume: -40 dB, Single Ended
    0x7F,  // Treble: 0 dB, 17.5 kHz
    0x1F,  // Bass: 0 dB, Q=1, DC-Mode aus
    0x40,  // LF: 0 dB, Bass-Mittenfrequenz 80 Hz
    0x00,  // RF
    0x00,  // LR
    0x00,  // RR
    0x00,  // SubL
    0x00,  // SubR
    0x33,  // Mute/Mix: Soft-Mute aus, 0.96 ms, Mixing aus, Beep 781 Hz
    0x4C   // Others: AutoZero, Make-up-Bits, Soft-Step 0.68 ms, interne Kopplung, Beep aus
};

// Speaker-Stufen 24..31 in 1/2 dB (Tabelle 13): -24.5 -26 -28 -30 -32 -35 -40 -50
static const uint8_t SPEAKER_COARSE_HALF_DB[8] = { 49, 52, 56, 60, 64, 70, 80, 100 };

static int16_t clampi(int16_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

TDA7404::TDA7404() {
    memcpy(_reg, DEFAULTS, sizeof(_reg));
}

bool TDA7404::begin(TwoWire &wire, uint8_t address) {
    _wire = &wire;
    _address = address;
    if (!isConnected()) return false;
    return writeAll(false);
}

bool TDA7404::isConnected() {
    if (!_wire) return false;
    _wire->beginTransmission(_address);
    _lastError = _wire->endTransmission();
    return _lastError == 0;
}

uint8_t TDA7404::subaddress(uint8_t reg, bool autoInc, bool keepAutoZero) const {
    uint8_t sub = reg & 0x0F;
    if (!_zeroCross) sub |= SUB_SOFT_MUTE;
    if (keepAutoZero) sub |= SUB_AUTOZERO_KEEP;
    if (autoInc) sub |= SUB_AUTO_INC;
    return sub;
}

bool TDA7404::writeAll(bool keepAutoZero) {
    if (!_wire) return false;
    _wire->beginTransmission(_address);
    _wire->write(subaddress(REG_INPUT, true, keepAutoZero));
    _wire->write(_reg, REGISTER_COUNT);
    _lastError = _wire->endTransmission();
    return _lastError == 0;
}

bool TDA7404::writeRegister(uint8_t reg, uint8_t value) {
    if (!_wire || reg > REG_TESTING) return false;
    if (reg < REGISTER_COUNT) _reg[reg] = value;
    _wire->beginTransmission(_address);
    _wire->write(subaddress(reg, false, false));
    _wire->write(value);
    _lastError = _wire->endTransmission();
    return _lastError == 0;
}

uint8_t TDA7404::getRegister(uint8_t reg) const {
    return reg < REGISTER_COUNT ? _reg[reg] : 0;
}

bool TDA7404::update(uint8_t reg, uint8_t mask, uint8_t value) {
    return writeRegister(reg, (_reg[reg] & ~mask) | (value & mask));
}

int8_t TDA7404::readStatus() {
    if (!_wire) return -1;
    // Lesezugriff ohne Subadresse: der Chip liefert direkt das Statusbyte
    if (_wire->requestFrom(_address, (uint8_t)1) != 1) return -1;
    return _wire->read() & 0x01;
}

bool TDA7404::isMuting() {
    return readStatus() == 1;
}

// ---------------- Eingang ----------------

bool TDA7404::setInput(Input source) {
    return update(REG_INPUT, 0x07, (uint8_t)source);
}

bool TDA7404::setInputGain(uint8_t gainDb) {
    uint8_t code;
    if (gainDb <= 14) code = gainDb;
    else if (gainDb <= 16) code = 15;
    else if (gainDb <= 18) code = 16;
    else code = 17;
    return update(REG_INPUT, 0xF8, code << 3);
}

TDA7404::Input TDA7404::getInput() const {
    return (Input)(_reg[REG_INPUT] & 0x07);
}

uint8_t TDA7404::getInputGain() const {
    uint8_t code = _reg[REG_INPUT] >> 3;
    if (code & 0x10) return (code & 0x01) ? 20 : 18;
    return code == 15 ? 16 : code;
}

// ---------------- Lautstaerke ----------------

bool TDA7404::setVolume(int8_t db) {
    _volumeDb = clampi(db, VOLUME_MIN_DB, _volumeLimit);
    if (_volumeMuted) return true;
    return update(REG_VOLUME, 0x7F, 32 - _volumeDb);
}

int8_t TDA7404::getVolume() const {
    return _volumeDb;
}

void TDA7404::setVolumeLimit(int8_t maxDb) {
    _volumeLimit = clampi(maxDb, VOLUME_MIN_DB, VOLUME_MAX_DB);
    if (_volumeDb > _volumeLimit) setVolume(_volumeLimit);
}

bool TDA7404::setVolumeMute(bool mute) {
    _volumeMuted = mute;
    return update(REG_VOLUME, 0x7F, mute ? 0x7F : 32 - _volumeDb);
}

bool TDA7404::setDifferentialMode(bool diff) {
    return update(REG_VOLUME, 0x80, diff ? 0x80 : 0x00);
}

// ---------------- Loudness ----------------

bool TDA7404::setLoudness(uint8_t attenuationDb, LoudnessFreq freq, LoudnessShape shape) {
    // Bei "flat" muss D6 = 0 sein
    if (freq == LoudnessFreq::FLAT) shape = LoudnessShape::LOW_BOOST;
    uint8_t v = (clampi(attenuationDb, 0, 15))
              | ((uint8_t)freq << 4)
              | ((uint8_t)shape << 6);
    return update(REG_LOUDNESS, 0x7F, v);
}

bool TDA7404::setSoftStep(bool enable) {
    return update(REG_LOUDNESS, 0x80, enable ? 0x80 : 0x00);
}

// ---------------- Klang ----------------

// D4 = Boost/Cut, D3..D0 = 15 - |dB| (0000 = 15 dB, 1111 = 0 dB)
uint8_t TDA7404::toneToBits(int8_t db) {
    int16_t d = clampi(db, -TONE_MAX_DB, TONE_MAX_DB);
    uint8_t boost = d >= 0 ? 0x10 : 0x00;
    if (d < 0) d = -d;
    return boost | (uint8_t)(15 - d);
}

int8_t TDA7404::bitsToTone(uint8_t bits) {
    int8_t mag = 15 - (bits & 0x0F);
    return (bits & 0x10) ? mag : -mag;
}

bool TDA7404::setTreble(int8_t db) {
    return update(REG_TREBLE, 0x1F, toneToBits(db));
}

bool TDA7404::setTrebleFreq(TrebleFreq freq) {
    return update(REG_TREBLE, 0x60, (uint8_t)freq << 5);
}

bool TDA7404::setBass(int8_t db) {
    return update(REG_BASS, 0x1F, toneToBits(db));
}

bool TDA7404::setBassQ(BassQ q) {
    return update(REG_BASS, 0x60, (uint8_t)q << 5);
}

bool TDA7404::setBassDcMode(bool enable) {
    return update(REG_BASS, 0x80, enable ? 0x80 : 0x00);
}

bool TDA7404::setBassFreq(BassFreq freq) {
    // Die Bass-Mittenfrequenz liegt im Register des Lautsprechers LF
    return update(REG_SPK_LF, 0xC0, (uint8_t)freq << 6);
}

int8_t TDA7404::getTreble() const {
    return bitsToTone(_reg[REG_TREBLE]);
}

int8_t TDA7404::getBass() const {
    return bitsToTone(_reg[REG_BASS]);
}

// ---------------- Lautsprecher ----------------

uint8_t TDA7404::speakerReg(Channel ch) {
    return REG_SPK_LF + (uint8_t)ch;
}

uint8_t TDA7404::speakerDbToStep(int8_t db) {
    if (db >= 0) return 0;
    if (db >= -23) return -db;
    // Grobe Stufen: naechstgelegenen Wert suchen
    int16_t target = -2 * (int16_t)db;
    uint8_t best = 31;
    int16_t bestDiff = 0x7FFF;
    for (uint8_t i = 0; i < 8; i++) {
        int16_t diff = abs(target - (int16_t)SPEAKER_COARSE_HALF_DB[i]);
        if (diff < bestDiff) { bestDiff = diff; best = 24 + i; }
    }
    return best;
}

float TDA7404::speakerStepToDb(uint8_t step) {
    step &= 0x1F;
    if (step <= 23) return -(float)step;
    return -SPEAKER_COARSE_HALF_DB[step - 24] / 2.0f;
}

bool TDA7404::setSpeaker(Channel ch, int8_t db) {
    if ((uint8_t)ch > (uint8_t)Channel::SUB_RIGHT) return false;
    return update(speakerReg(ch), 0x1F, speakerDbToStep(db));
}

bool TDA7404::setSpeakerMute(Channel ch, bool mute) {
    if ((uint8_t)ch > (uint8_t)Channel::SUB_RIGHT) return false;
    return update(speakerReg(ch), 0x20, mute ? 0x20 : 0x00);
}

float TDA7404::getSpeaker(Channel ch) const {
    if ((uint8_t)ch > (uint8_t)Channel::SUB_RIGHT) return 0;
    return speakerStepToDb(_reg[speakerReg(ch)]);
}

bool TDA7404::setBalanceFader(int8_t balance, int8_t fader) {
    int16_t b = clampi(balance, -100, 100);
    int16_t f = clampi(fader, -100, 100);
    // Daempfung pro Seite: 0..100 % -> 0..-50 dB, bei 100 % stumm
    int16_t attLeft  = b > 0 ? b : 0;
    int16_t attRight = b < 0 ? -b : 0;
    int16_t attFront = f < 0 ? -f : 0;
    int16_t attRear  = f > 0 ? f : 0;

    struct { Channel ch; int16_t a, c; } spk[4] = {
        { Channel::LEFT_FRONT,  attLeft,  attFront },
        { Channel::RIGHT_FRONT, attRight, attFront },
        { Channel::LEFT_REAR,   attLeft,  attRear  },
        { Channel::RIGHT_REAR,  attRight, attRear  },
    };

    bool ok = true;
    for (uint8_t i = 0; i < 4; i++) {
        bool mute = spk[i].a >= 100 || spk[i].c >= 100;
        int16_t db = -((spk[i].a + spk[i].c) * 50) / 100;
        uint8_t reg = speakerReg(spk[i].ch);
        uint8_t v = (mute ? 0x20 : 0x00) | speakerDbToStep(clampi(db, SPEAKER_MIN_DB, 0));
        ok &= update(reg, 0x3F, v);
    }
    return ok;
}

// ---------------- Soft-Mute ----------------

bool TDA7404::setMute(bool mute) {
    // D0 = 0: Soft-Mute aktiv, D0 = 1: Soft-Mute aus
    return update(REG_MUTE_MIX, 0x01, mute ? 0x00 : 0x01);
}

bool TDA7404::setMuteTime(MuteTime t) {
    return update(REG_MUTE_MIX, 0x06, (uint8_t)t << 1);
}

bool TDA7404::setZeroCrossMute(bool zeroCross) {
    // Das Bit sitzt im Subadress-Byte, also mit dem naechsten Schreibzugriff uebertragen
    _zeroCross = zeroCross;
    return writeRegister(REG_MUTE_MIX, _reg[REG_MUTE_MIX]);
}

// ---------------- Mixing / Beep ----------------

bool TDA7404::setMixing(MixSource src, MixLevel level) {
    return update(REG_MUTE_MIX, 0x38, ((uint8_t)src << 3) | ((uint8_t)level << 4));
}

bool TDA7404::setBeep(bool enable, BeepFreq freq) {
    bool ok = update(REG_MUTE_MIX, 0xC0, (uint8_t)freq << 6);
    return update(REG_OTHERS, 0x80, enable ? 0x80 : 0x00) && ok;
}

// ---------------- Others ----------------

bool TDA7404::setAcCoupling(AcCoupling mode) {
    return update(REG_OTHERS, 0x03, (uint8_t)mode);
}

bool TDA7404::setSoftStepTime(SoftStepTime t) {
    return update(REG_OTHERS, 0x30, (uint8_t)t << 4);
}
