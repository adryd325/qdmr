#include "anytone_codeplug.hh"
#include "utils.hh"
#include <QTimeZone>

using namespace Signaling;

Code _anytone_ctcss_num2code[52] = {
  SIGNALING_NONE, // 62.5 not supported
  CTCSS_67_0Hz,  SIGNALING_NONE, // 69.3 not supported
  CTCSS_71_9Hz,  CTCSS_74_4Hz,  CTCSS_77_0Hz,  CTCSS_79_7Hz,  CTCSS_82_5Hz,
  CTCSS_85_4Hz,  CTCSS_88_5Hz,  CTCSS_91_5Hz,  CTCSS_94_8Hz,  CTCSS_97_4Hz,  CTCSS_100_0Hz,
  CTCSS_103_5Hz, CTCSS_107_2Hz, CTCSS_110_9Hz, CTCSS_114_8Hz, CTCSS_118_8Hz, CTCSS_123_0Hz,
  CTCSS_127_3Hz, CTCSS_131_8Hz, CTCSS_136_5Hz, CTCSS_141_3Hz, CTCSS_146_2Hz, CTCSS_151_4Hz,
  CTCSS_156_7Hz,
  SIGNALING_NONE, // 159.8 not supported
  CTCSS_162_2Hz,
  SIGNALING_NONE, // 165.5 not supported
  CTCSS_167_9Hz,
  SIGNALING_NONE, // 171.3 not supported
  CTCSS_173_8Hz,
  SIGNALING_NONE, // 177.3 not supported
  CTCSS_179_9Hz,
  SIGNALING_NONE, // 183.5 not supported
  CTCSS_186_2Hz,
  SIGNALING_NONE, // 189.9 not supported
  CTCSS_192_8Hz,
  SIGNALING_NONE, SIGNALING_NONE, // 196.6 & 199.5 not supported
  CTCSS_203_5Hz,
  SIGNALING_NONE, // 206.5 not supported
  CTCSS_210_7Hz, CTCSS_218_1Hz, CTCSS_225_7Hz,
  SIGNALING_NONE, // 229.1 not supported
  CTCSS_233_6Hz, CTCSS_241_8Hz, CTCSS_250_3Hz,
  SIGNALING_NONE, SIGNALING_NONE // 254.1 and custom CTCSS not supported.
};

inline uint8_t
ctcss_code2num(Signaling::Code code) {
  for (uint8_t i=0; i<52; i++) {
    if (code == _anytone_ctcss_num2code[i])
      return i;
  }
  return 0;
}

inline Signaling::Code
ctcss_num2code(uint8_t num) {
  if (num >= 52)
    return Signaling::SIGNALING_NONE;
  return _anytone_ctcss_num2code[num];
}

QVector<char> _anytone_bin_dtmf_tab = {
  '0','1','2','3','4','5','6','7','8','9','A','B','C','D','*','#'
};


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::ChannelElement
 * ********************************************************************************************* */
AnytoneCodeplug::ChannelElement::ChannelElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::ChannelElement::ChannelElement(uint8_t *ptr)
  : Element(ptr, 0x0040)
{
  // pass...
}

AnytoneCodeplug::ChannelElement::~ChannelElement() {
  // pass...
}

void
AnytoneCodeplug::ChannelElement::clear() {
  setRXFrequency(0);
  setTXOffset(0);
  setMode(Mode::Analog);
  setPower(Channel::Power::Low);
  setBandwidth(AnalogChannel::Bandwidth::Narrow);
  setBit(0x0008, 5, false); // Unused set to 0
}

uint
AnytoneCodeplug::ChannelElement::rxFrequency() const {
  return ((uint)getBCD8_be(0x0000))*10;
}
void
AnytoneCodeplug::ChannelElement::setRXFrequency(uint hz) {
  setBCD8_be(0x0000, hz/10);
}

uint
AnytoneCodeplug::ChannelElement::txOffset() const {
  return ((uint)getBCD8_be(0x0004))*10;
}
void
AnytoneCodeplug::ChannelElement::setTXOffset(uint hz) {
  setBCD8_be(0x0004, hz/10);
}

uint
AnytoneCodeplug::ChannelElement::txFrequency() const {
  uint rx = rxFrequency(), off = txOffset();
  if (RepeaterMode::Simplex == repeaterMode())
    return rx;
  else if (RepeaterMode::Positive == repeaterMode())
    return rx+off;
  return rx-off;
}
void
AnytoneCodeplug::ChannelElement::setTXFrequency(uint hz) {
  uint rx = rxFrequency();
  if (rx == hz) {
    setTXOffset(0); setRepeaterMode(RepeaterMode::Simplex);
  } else if (rx < hz) {
    setTXOffset(hz-rx); setRepeaterMode(RepeaterMode::Positive);
  } else {
    setTXOffset(rx-hz); setRepeaterMode(RepeaterMode::Negative);
  }
}

AnytoneCodeplug::ChannelElement::Mode
AnytoneCodeplug::ChannelElement::mode() const {
  return (Mode) getUInt2(0x0008, 0);
}
void
AnytoneCodeplug::ChannelElement::setMode(Mode mode) {
  setUInt2(0x0008, 0, (uint)mode);
}

Channel::Power
AnytoneCodeplug::ChannelElement::power() const {
  switch ((Power)getUInt2(0x0008, 2)) {
  case POWER_LOW: return Channel::Power::Low;
  case POWER_MIDDLE: return Channel::Power::Mid;
  case POWER_HIGH: return Channel::Power::High;
  case POWER_TURBO: return Channel::Power::Max;
  }
  return Channel::Power::Low;
}
void
AnytoneCodeplug::ChannelElement::setPower(Channel::Power power) {
  switch (power) {
  case Channel::Power::Min:
  case Channel::Power::Low:
    setUInt2(0x0008, 2, (uint)POWER_LOW);
    break;
  case Channel::Power::Mid:
    setUInt2(0x0008, 2, (uint)POWER_MIDDLE);
    break;
  case Channel::Power::High:
    setUInt2(0x0008, 2, (uint)POWER_HIGH);
    break;
  case Channel::Power::Max:
    setUInt2(0x0008, 2, (uint)POWER_TURBO);
    break;
  }
}

AnalogChannel::Bandwidth
AnytoneCodeplug::ChannelElement::bandwidth() const {
  if (getBit(0x0008, 4))
    return AnalogChannel::Bandwidth::Wide;
  return AnalogChannel::Bandwidth::Narrow;
}
void
AnytoneCodeplug::ChannelElement::setBandwidth(AnalogChannel::Bandwidth bw) {
  switch (bw) {
  case AnalogChannel::Bandwidth::Narrow: setBit(0x0008, 4, false); break;
  case AnalogChannel::Bandwidth::Wide: setBit(0x0008, 4, true); break;
  }
}

AnytoneCodeplug::ChannelElement::RepeaterMode
AnytoneCodeplug::ChannelElement::repeaterMode() const {
  return (RepeaterMode)getUInt2(0x0008, 6);
}
void
AnytoneCodeplug::ChannelElement::setRepeaterMode(RepeaterMode mode) {
  setUInt2(0x0008, 6, (uint)mode);
}

AnytoneCodeplug::ChannelElement::SignalingMode
AnytoneCodeplug::ChannelElement::rxSignalingMode() const {
  return (SignalingMode)getUInt2(0x0009, 0);
}
void
AnytoneCodeplug::ChannelElement::setRXSignalingMode(SignalingMode mode) {
  setUInt2(0x0009, 0, (uint)mode);
}

AnytoneCodeplug::ChannelElement::SignalingMode
AnytoneCodeplug::ChannelElement::txSignalingMode() const {
  return (SignalingMode)getUInt2(0x0009, 2);
}
void
AnytoneCodeplug::ChannelElement::setTXSignalingMode(SignalingMode mode) {
  setUInt2(0x0009, 2, (uint)mode);
}

bool
AnytoneCodeplug::ChannelElement::ctcssPhaseReversal() const {
  return getBit(0x0009, 4);
}
void
AnytoneCodeplug::ChannelElement::enableCTCSSPhaseReversal(bool enable) {
  setBit(0x0009, 4, enable);
}
bool
AnytoneCodeplug::ChannelElement::rxOnly() const {
  return getBit(0x0009, 5);
}
void
AnytoneCodeplug::ChannelElement::enableRXOnly(bool enable) {
  setBit(0x0009, 5, enable);
}
bool
AnytoneCodeplug::ChannelElement::callConfirm() const {
  return getBit(0x0009, 6);
}
void
AnytoneCodeplug::ChannelElement::enableCallConfirm(bool enable) {
  setBit(0x0009, 6, enable);
}
bool
AnytoneCodeplug::ChannelElement::talkaround() const {
  return getBit(0x0009, 7);
}
void
AnytoneCodeplug::ChannelElement::enableTalkaround(bool enable) {
  setBit(0x0009, 7, enable);
}

Signaling::Code
AnytoneCodeplug::ChannelElement::txCTCSS() const {
  return ctcss_num2code(getUInt8(0x000a));
}
void
AnytoneCodeplug::ChannelElement::setTXCTCSS(Code tone) {
  setUInt8(0x000a, ctcss_code2num(tone));
}
Signaling::Code
AnytoneCodeplug::ChannelElement::rxCTCSS() const {
  return ctcss_num2code(getUInt8(0x000b));
}
void
AnytoneCodeplug::ChannelElement::setRXCTCSS(Code tone) {
  setUInt8(0x000b, ctcss_code2num(tone));
}
Signaling::Code
AnytoneCodeplug::ChannelElement::txDCS() const {
  uint16_t code = getUInt16_le(0x000c);
  if (512 < code)
    return Signaling::fromDCSNumber(dec_to_oct(code), false);
  return Signaling::fromDCSNumber(dec_to_oct(code-512), true);
}
void
AnytoneCodeplug::ChannelElement::setTXDCS(Code code) {
  if (Signaling::isDCSNormal(code))
    setUInt8(0x000c, oct_to_dec(Signaling::toDCSNumber(code)));
  else if (Signaling::isDCSInverted(code))
    setUInt8(0x000c, oct_to_dec(Signaling::toDCSNumber(code))+512);
  else
    setUInt8(0x000c, 0);
}

Signaling::Code
AnytoneCodeplug::ChannelElement::rxDCS() const {
  uint16_t code = getUInt16_le(0x000e);
  if (512 < code)
    return Signaling::fromDCSNumber(dec_to_oct(code), false);
  return Signaling::fromDCSNumber(dec_to_oct(code-512), true);
}
void
AnytoneCodeplug::ChannelElement::setRXDCS(Code code) {
  if (Signaling::isDCSNormal(code))
    setUInt16_le(0x000e, oct_to_dec(Signaling::toDCSNumber(code)));
  else if (Signaling::isDCSInverted(code))
    setUInt16_le(0x000e, oct_to_dec(Signaling::toDCSNumber(code))+512);
  else
    setUInt16_le(0x000e, 0);
}

double
AnytoneCodeplug::ChannelElement::customCTCSSFrequency() const {
  return ((double) getUInt16_le(0x0010))/10;
}
void
AnytoneCodeplug::ChannelElement::setCustomCTCSSFrequency(double hz) {
  setUInt16_le(0x0010, hz*10);
}

uint
AnytoneCodeplug::ChannelElement::twoToneDecodeIndex() const {
  return getUInt16_le(0x0012);
}
void
AnytoneCodeplug::ChannelElement::setTwoToneDecodeIndex(uint idx) {
  setUInt16_le(0x0012, idx);
}

uint
AnytoneCodeplug::ChannelElement::contactIndex() const {
  return getUInt32_le(0x0014);
}
void
AnytoneCodeplug::ChannelElement::setContactIndex(uint idx) {
  return setUInt32_le(0x0014, idx);
}

uint
AnytoneCodeplug::ChannelElement::radioIDIndex() const {
  return getUInt8(0x0018);
}
void
AnytoneCodeplug::ChannelElement::setRadioIDIndex(uint idx) {
  return setUInt8(0x0018, idx);
}

bool
AnytoneCodeplug::ChannelElement::silentSquelch() const {
  return getBit(0x0019, 4);
}
void
AnytoneCodeplug::ChannelElement::enableSilentSquelch(bool enable) {
  setBit(0x0019, 4, enable);
}

AnytoneCodeplug::ChannelElement::Admit
AnytoneCodeplug::ChannelElement::admit() const {
  return (Admit)getUInt2(0x001a, 0);
}
void
AnytoneCodeplug::ChannelElement::setAdmit(Admit admit) {
  setUInt2(0x001a, 0, (uint)admit);
}

AnytoneCodeplug::ChannelElement::OptSignaling
AnytoneCodeplug::ChannelElement::optionalSignaling() const {
  return (OptSignaling)getUInt2(0x001a, 4);
}
void
AnytoneCodeplug::ChannelElement::setOptionalSignaling(OptSignaling sig) {
  setUInt2(0x001a, 4, (uint)sig);
}

bool
AnytoneCodeplug::ChannelElement::hasScanListIndex() const {
  return 0xff != scanListIndex();
}
uint
AnytoneCodeplug::ChannelElement::scanListIndex() const {
  return getUInt8(0x001b);
}
void
AnytoneCodeplug::ChannelElement::setScanListIndex(uint idx) {
  setUInt8(0x001b, idx);
}
void
AnytoneCodeplug::ChannelElement::clearScanListIndex() {
  setScanListIndex(0xff);
}

bool
AnytoneCodeplug::ChannelElement::hasGroupListIndex() const {
  return 0xff != groupListIndex();
}
uint
AnytoneCodeplug::ChannelElement::groupListIndex() const {
  return getUInt8(0x001c);
}
void
AnytoneCodeplug::ChannelElement::setGroupListIndex(uint idx) {
  setUInt8(0x001c, idx);
}
void
AnytoneCodeplug::ChannelElement::clearGroupListIndex() {
  setGroupListIndex(0xff);
}

uint
AnytoneCodeplug::ChannelElement::twoToneIDIndex() const {
  return getUInt8(0x001d);
}
void
AnytoneCodeplug::ChannelElement::setTwoToneIDIndex(uint idx) {
  setUInt8(0x001d, idx);
}
uint
AnytoneCodeplug::ChannelElement::fiveToneIDIndex() const {
  return getUInt8(0x001e);
}
void
AnytoneCodeplug::ChannelElement::setFiveToneIDIndex(uint idx) {
  setUInt8(0x001e, idx);
}
uint
AnytoneCodeplug::ChannelElement::dtmfIDIndex() const {
  return getUInt8(0x001f);
}
void
AnytoneCodeplug::ChannelElement::setDTMFIDIndex(uint idx) {
  setUInt8(0x001f, idx);
}

uint
AnytoneCodeplug::ChannelElement::colorCode() const {
  return getUInt8(0x0020);
}
void
AnytoneCodeplug::ChannelElement::setColorCode(uint code) {
  setUInt8(0x0020, code);
}

DigitalChannel::TimeSlot
AnytoneCodeplug::ChannelElement::timeSlot() const {
  if (false == getBit(0x0021, 0))
    return DigitalChannel::TimeSlot::TS1;
  return DigitalChannel::TimeSlot::TS2;
}
void
AnytoneCodeplug::ChannelElement::setTimeSlot(DigitalChannel::TimeSlot ts) {
  if (DigitalChannel::TimeSlot::TS1 == ts)
    setBit(0x0021, 0, false);
  else
    setBit(0x0021, 0, true);
}

bool
AnytoneCodeplug::ChannelElement::smsConfirm() const {
  return getBit(0x0021, 1);
}
void
AnytoneCodeplug::ChannelElement::enableSMSConfirm(bool enable) {
  setBit(0x0021, 1, enable);
}
bool
AnytoneCodeplug::ChannelElement::simplexTDMA() const {
  return getBit(0x0021, 2);
}
void
AnytoneCodeplug::ChannelElement::enableSimplexTDMA(bool enable) {
  setBit(0x0021, 2, enable);
}
bool
AnytoneCodeplug::ChannelElement::adaptiveTDMA() const {
  return getBit(0x0021, 4);
}
void
AnytoneCodeplug::ChannelElement::enableAdaptiveTDMA(bool enable) {
  setBit(0x0021, 4, enable);
}
bool
AnytoneCodeplug::ChannelElement::rxAPRS() const {
  return getBit(0x0021, 5);
}
void
AnytoneCodeplug::ChannelElement::enableRXAPRS(bool enable) {
  setBit(0x0021, 5, enable);
}
bool
AnytoneCodeplug::ChannelElement::enhancedEncryption() const {
  return getBit(0x0021, 6);
}
void
AnytoneCodeplug::ChannelElement::enableEnhancedEncryption(bool enable) {
  setBit(0x0021, 6, enable);
}
bool
AnytoneCodeplug::ChannelElement::loneWorker() const {
  return getBit(0x0021, 7);
}
void
AnytoneCodeplug::ChannelElement::enableLoneWorker(bool enable) {
  setBit(0x0021, 7, enable);
}

bool
AnytoneCodeplug::ChannelElement::hasEncryptionKeyIndex() const {
  return 0xff != encryptionKeyIndex();
}
uint
AnytoneCodeplug::ChannelElement::encryptionKeyIndex() const {
  return getUInt8(0x0022);
}
void
AnytoneCodeplug::ChannelElement::setEncryptionKeyIndex(uint idx) {
  setUInt8(0x0022, idx);
}
void
AnytoneCodeplug::ChannelElement::clearEncryptionKeyIndex() {
  setEncryptionKeyIndex(0xff);
}

QString
AnytoneCodeplug::ChannelElement::name() const {
  return readASCII(0x0023, 16, 0x00);
}
void
AnytoneCodeplug::ChannelElement::setName(const QString &name) {
  writeASCII(0x0023, name, 16, 0x00);
}

bool
AnytoneCodeplug::ChannelElement::ranging() const {
  return getBit(0x0034, 0);
}
void
AnytoneCodeplug::ChannelElement::enableRanging(bool enable) {
  setBit(0x0034, 0, enable);
}
bool
AnytoneCodeplug::ChannelElement::throughMode() const {
  return getBit(0x0034, 1);
}
void
AnytoneCodeplug::ChannelElement::enableThroughMode(bool enable) {
  setBit(0x0034, 1, enable);
}
bool
AnytoneCodeplug::ChannelElement::dataACK() const {
  return !getBit(0x0034, 2);
}
void
AnytoneCodeplug::ChannelElement::enableDataACK(bool enable) {
  setBit(0x0034, 2, !enable);
}

bool
AnytoneCodeplug::ChannelElement::txAPRS() const {
  return getBit(0x0035, 0);
}
void
AnytoneCodeplug::ChannelElement::enableTXAPRS(bool enable) {
  setBit(0x0035, 0, enable);
}
uint
AnytoneCodeplug::ChannelElement::dmrAPRSSystemIndex() const {
  return getUInt8(0x0036);
}
void
AnytoneCodeplug::ChannelElement::setDMRAPRSSystemIndex(uint idx) {
  setUInt8(0x0036, idx);
}

uint
AnytoneCodeplug::ChannelElement::dmrEncryptionKeyIndex() const {
  return getUInt8(0x003a);
}
void
AnytoneCodeplug::ChannelElement::setDMREncryptionKeyIndex(uint idx) {
  setUInt8(0x003a, idx);
}

bool
AnytoneCodeplug::ChannelElement::multipleKeyEncryption() const {
  return getBit(0x003b, 0);
}
void
AnytoneCodeplug::ChannelElement::enableMultipleKeyEncryption(bool enable) {
  setBit(0x003b, 0, enable);
}

bool
AnytoneCodeplug::ChannelElement::randomKey() const {
  return getBit(0x003b, 1);
}
void
AnytoneCodeplug::ChannelElement::enableRandomKey(bool enable) {
  setBit(0x003b, 1, enable);
}
bool
AnytoneCodeplug::ChannelElement::sms() const {
  return !getBit(0x003b, 2);
}
void
AnytoneCodeplug::ChannelElement::enableSMS(bool enable) {
  setBit(0x003b, 0, !enable);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::ContactElement
 * ********************************************************************************************* */
AnytoneCodeplug::ContactElement::ContactElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::ContactElement::ContactElement(uint8_t *ptr)
  : Element(ptr, 0x0064)
{
  // pass...
}

AnytoneCodeplug::ContactElement::~ContactElement() {
  // pass...
}

void
AnytoneCodeplug::ContactElement::clear() {
  memset(_data, 0, _size);
}

bool
AnytoneCodeplug::ContactElement::isValid() const {
  return !name().isEmpty();
}

DigitalContact::Type
AnytoneCodeplug::ContactElement::type() const {
  switch (getUInt8(0x0000)) {
  case 0: return DigitalContact::PrivateCall;
  case 1: return DigitalContact::GroupCall;
  case 2: return DigitalContact::AllCall;
  }
  return DigitalContact::PrivateCall;
}
void
AnytoneCodeplug::ContactElement::setType(DigitalContact::Type type) {
  switch (type) {
  case DigitalContact::PrivateCall: setUInt8(0x0000, 0); break;
  case DigitalContact::GroupCall: setUInt8(0x0000, 1); break;
  case DigitalContact::AllCall: setUInt8(0x0000, 2); break;
  }
}

QString
AnytoneCodeplug::ContactElement::name() const {
  return readASCII(0x0001, 16, 0x00);
}
void
AnytoneCodeplug::ContactElement::setName(const QString &name) {
  writeASCII(0x0001, name, 16, 0x00);
}

uint
AnytoneCodeplug::ContactElement::number() const {
  return getBCD8_le(0x0023);
}
void
AnytoneCodeplug::ContactElement::setNumber(uint number) {
  setBCD8_le(0x0023, number);
}

AnytoneCodeplug::ContactElement::AlertType
AnytoneCodeplug::ContactElement::alertType() const {
  return (AlertType) getUInt8(0x0027);
}
void
AnytoneCodeplug::ContactElement::setAlertType(AlertType type) {
  setUInt8(0x0027, (uint)type);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::DTMFContactElement
 * ********************************************************************************************* */
AnytoneCodeplug::DTMFContactElement::DTMFContactElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::DTMFContactElement::DTMFContactElement(uint8_t *ptr)
  : Element(ptr, 0x30)
{
  // pass...
}

AnytoneCodeplug::DTMFContactElement::~DTMFContactElement() {
  // pass...
}

void
AnytoneCodeplug::DTMFContactElement::clear() {
  memset(_data, 0, _size);
}

QString
AnytoneCodeplug::DTMFContactElement::number() const {
  QString number;
  int n = getUInt8(0x0013);
  for (int i=0; i<n; i++) {
    uint8_t byte = _data[i/2];
    if (0 == (i%2))
      number.append(_anytone_bin_dtmf_tab[(byte>>4)&0xf]);
    else
      number.append(_anytone_bin_dtmf_tab[(byte>>0)&0xf]);
  }
  return number;
}
void
AnytoneCodeplug::DTMFContactElement::setNumber(const QString &number) {
  if (! validDTMFNumber(number))
    return;
  memset(_data+0x0000, 0, 7);
  setUInt8(0x0013, number.length());
  for (int i=0; i<number.length(); i++) {
    if (0 == (i%2))
      _data[i/2] |= (_anytone_bin_dtmf_tab.indexOf(number[i].toLatin1())<<4);
    else
      _data[i/2] |= (_anytone_bin_dtmf_tab.indexOf(number[i].toLatin1())<<0);
  }
}

QString
AnytoneCodeplug::DTMFContactElement::name() const {
  return readASCII(0x0020, 15, 0x00);
}
void
AnytoneCodeplug::DTMFContactElement::setName(const QString &name) {
  writeASCII(0x0020, name, 15, 0x00);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::GroupListElement
 * ********************************************************************************************* */
AnytoneCodeplug::GroupListElement::GroupListElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::GroupListElement::GroupListElement(uint8_t *ptr)
  : Element(ptr, 0x120)
{
  // pass...
}

void
AnytoneCodeplug::GroupListElement::clear() {
  memset(_data, 0x00, _size);
  // set member indices to 0xffffffff
  memset(_data, 0xff, 4*64);
}

bool
AnytoneCodeplug::GroupListElement::isValid() const {
  return !name().isEmpty();
}

QString
AnytoneCodeplug::GroupListElement::name() const {
  return readASCII(0x0100, 16, 0x00);
}
void
AnytoneCodeplug::GroupListElement::setName(const QString &name) {
  writeASCII(0x0100, name, 16, 0x00);
}

bool
AnytoneCodeplug::GroupListElement::hasMemberIndex(uint n) const {
  return 0xffffffff != memberIndex(n);
}
uint
AnytoneCodeplug::GroupListElement::memberIndex(uint n) const {
  return getUInt32_le(0x0000 + 4*n);
}
void
AnytoneCodeplug::GroupListElement::setMemberIndex(uint n, uint idx) {
  setUInt32_le(0x0000 + 4*n, idx);
}
void
AnytoneCodeplug::GroupListElement::clearMemberIndex(uint n) {
  setMemberIndex(n, 0xffffffff);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::ScanListElement
 * ********************************************************************************************* */
AnytoneCodeplug::ScanListElement::ScanListElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::ScanListElement::ScanListElement(uint8_t *ptr)
  : Element(ptr, 0x0090)
{
  // pass...
}

void
AnytoneCodeplug::ScanListElement::clear() {
  memset(_data, 0, _size);
  setPriorityChannels(PriChannel::Off);
  clearPrimaryChannel();
  clearSecondaryChannel();
  setLookBackTimeA(150);
  setLookBackTimeB(250);
  setDropOutDelay(290);
  setDwellTime(290);
  setRevertChannel(RevertChannel::Selected);
  // clear members
  memset(_data+0x0020, 0xff, 2*50);
}

AnytoneCodeplug::ScanListElement::PriChannel
AnytoneCodeplug::ScanListElement::priorityChannels() const {
  return (PriChannel) getUInt8(0x0001);
}
void
AnytoneCodeplug::ScanListElement::setPriorityChannels(PriChannel sel) {
  setUInt8(0x0001, (uint)sel);
}

bool
AnytoneCodeplug::ScanListElement::hasPrimary() const {
  return 0xffff != getUInt16_le(0x0002);
}
bool
AnytoneCodeplug::ScanListElement::primaryIsSelected() const {
  return 0 == getUInt16_le(0x0002);
}
uint
AnytoneCodeplug::ScanListElement::primary() const {
  return ((uint)getUInt16_le(0x0002))-1;
}
void
AnytoneCodeplug::ScanListElement::setPrimary(uint idx) {
  setUInt16_le(0x0002, idx+1);
}
void
AnytoneCodeplug::ScanListElement::setPrimarySelected() {
  setUInt16_le(0x0002, 0);
}
void
AnytoneCodeplug::ScanListElement::clearPrimaryChannel() {
  setUInt16_le(0x0002, 0xffff);
}

bool
AnytoneCodeplug::ScanListElement::hasSecondary() const {
  return 0xffff != getUInt16_le(0x0004);
}
bool
AnytoneCodeplug::ScanListElement::secondaryIsSelected() const {
  return 0 == getUInt16_le(0x0004);
}
uint
AnytoneCodeplug::ScanListElement::secondary() const {
  return ((uint)getUInt16_le(0x0002))-1;
}
void
AnytoneCodeplug::ScanListElement::setSecondary(uint idx) {
  setUInt16_le(0x0004, idx+1);
}
void
AnytoneCodeplug::ScanListElement::setSecondarySelected() {
  setUInt16_le(0x0004, 0);
}
void
AnytoneCodeplug::ScanListElement::clearSecondaryChannel() {
  setUInt16_le(0x0004, 0xffff);
}

uint
AnytoneCodeplug::ScanListElement::lookBackTimeA() const {
  return ((uint)getUInt16_le(0x0006))*10;
}
void
AnytoneCodeplug::ScanListElement::setLookBackTimeA(uint sec) {
  setUInt16_le(0x0006, sec/10);
}
uint
AnytoneCodeplug::ScanListElement::lookBackTimeB() const {
  return ((uint)getUInt16_le(0x0008))*10;
}
void
AnytoneCodeplug::ScanListElement::setLookBackTimeB(uint sec) {
  setUInt16_le(0x0008, sec/10);
}
uint
AnytoneCodeplug::ScanListElement::dropOutDelay() const {
  return ((uint)getUInt16_le(0x000a))*10;
}
void
AnytoneCodeplug::ScanListElement::setDropOutDelay(uint sec) {
  setUInt16_le(0x000a, sec/10);
}
uint
AnytoneCodeplug::ScanListElement::dwellTime() const {
  return ((uint)getUInt16_le(0x000c))*10;
}
void
AnytoneCodeplug::ScanListElement::setDwellTime(uint sec) {
  setUInt16_le(0x000c, sec/10);
}

AnytoneCodeplug::ScanListElement::RevertChannel
AnytoneCodeplug::ScanListElement::revertChannel() const {
  return (RevertChannel)getUInt8(0x000e);
}
void
AnytoneCodeplug::ScanListElement::setRevertChannel(RevertChannel type) {
  setUInt8(0x000e, (uint)type);
}

QString
AnytoneCodeplug::ScanListElement::name() const {
  return readASCII(0x000f, 16, 0x00);
}
void
AnytoneCodeplug::ScanListElement::setName(const QString &name) {
  writeASCII(0x000f, name, 16, 0x00);
}

bool
AnytoneCodeplug::ScanListElement::hasMemberIndex(uint n) const {
  return 0xffff != memberIndex(n);
}
uint
AnytoneCodeplug::ScanListElement::memberIndex(uint n) const {
  return getUInt16_le(0x0020+2*n);
}
void
AnytoneCodeplug::ScanListElement::setMemberIndex(uint n, uint idx) {
  setUInt16_le(0x0020+2*n, idx);
}
void
AnytoneCodeplug::ScanListElement::clearMemberIndex(uint n) {
  setMemberIndex(n, 0xffff);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::RadioIDElement
 * ********************************************************************************************* */
AnytoneCodeplug::RadioIDElement::RadioIDElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::RadioIDElement::RadioIDElement(uint8_t *ptr)
  : Element(ptr, 0x0020)
{
  // pass...
}

void
AnytoneCodeplug::RadioIDElement::clear() {
  memset(_data, 0x00, _size);
}

uint
AnytoneCodeplug::RadioIDElement::number() const {
  return getBCD8_le(0x0000);
}
void
AnytoneCodeplug::RadioIDElement::setNumber(uint number) {
  setBCD8_le(0x0000, number);
}

QString
AnytoneCodeplug::RadioIDElement::name() const {
  return readASCII(0x0005, 16, 0x00);
}
void
AnytoneCodeplug::RadioIDElement::setName(const QString &name) {
  writeASCII(0x0005, name, 16, 0x00);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::GeneralSettingsElement::Melody
 * ********************************************************************************************* */
AnytoneCodeplug::GeneralSettingsElement::Melody::Melody()
{
  for (int i=0; i<5; i++)
    notes[i] = Note{0,0};
}

AnytoneCodeplug::GeneralSettingsElement::Melody::Melody(const Melody &other)
{
  for (int i=0; i<5; i++)
    notes[i] = other.notes[i];
}

AnytoneCodeplug::GeneralSettingsElement::Melody &
AnytoneCodeplug::GeneralSettingsElement::Melody::operator =(const Melody &other) {
  for (int i=0; i<5; i++)
    notes[i] = other.notes[i];
  return *this;
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::GeneralSettingsElement
 * ********************************************************************************************* */
AnytoneCodeplug::GeneralSettingsElement::GeneralSettingsElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::GeneralSettingsElement::GeneralSettingsElement(uint8_t *ptr)
  : Element(ptr, 0x00d0)
{
  // pass...
}

void
AnytoneCodeplug::GeneralSettingsElement::clear() {
  memset(_data, 0, _size);
}

bool
AnytoneCodeplug::GeneralSettingsElement::keyTone() const {
  return 0!=getUInt8(0x0000);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableKeyTone(bool enable) {
  setUInt8(0x0000, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::displayFrequency() const {
  return 0!=getUInt8(0x0001);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableDisplayFrequency(bool enable) {
  setUInt8(0x0001, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::autoKeyLock() const {
  return 0!=getUInt8(0x0002);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableAutoKeyLock(bool enable) {
  setUInt8(0x0002, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::autoShutdownDelay() const {
  switch ((AutoShutdown) getUInt8(0x0003)) {
  case AutoShutdown::Off: return 0;
  case AutoShutdown::After10min: return 10;
  case AutoShutdown::After30min: return 30;
  case AutoShutdown::After60min: return 60;
  case AutoShutdown::After120min: return 120;
  }
  return 0;
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoShutdownDelay(uint min) {
  if (0 == min) {
    setUInt8(0x0003, (uint)AutoShutdown::Off);
  } else if (min <= 10) {
    setUInt8(0x0003, (uint)AutoShutdown::After10min);
  } else if (min <= 30) {
    setUInt8(0x0003, (uint)AutoShutdown::After30min);
  } else if (min <= 60) {
    setUInt8(0x0003, (uint)AutoShutdown::After60min);
  } else {
    setUInt8(0x0003, (uint)AutoShutdown::After120min);
  }
}

AnytoneCodeplug::GeneralSettingsElement::BootDisplay
AnytoneCodeplug::GeneralSettingsElement::bootDisplay() const {
  return (BootDisplay) getUInt8(0x0006);
}
void
AnytoneCodeplug::GeneralSettingsElement::setBootDisplay(BootDisplay mode) {
  setUInt8(0x0006, (uint)mode);
}

bool
AnytoneCodeplug::GeneralSettingsElement::bootPassword() const {
  return getUInt8(0x0007);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableBootPassword(bool enable) {
  setUInt8(0x0006, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::squelchLevelA() const {
  return getUInt8(0x0009);
}
void
AnytoneCodeplug::GeneralSettingsElement::setSquelchLevelA(uint level) {
  setUInt8(0x0009, level);
}
uint
AnytoneCodeplug::GeneralSettingsElement::squelchLevelB() const {
  return getUInt8(0x000a);
}
void
AnytoneCodeplug::GeneralSettingsElement::setSquelchLevelB(uint level) {
  setUInt8(0x000a, level);
}

AnytoneCodeplug::GeneralSettingsElement::PowerSave
AnytoneCodeplug::GeneralSettingsElement::powerSave() const {
  return (PowerSave) getUInt8(0x000b);
}
void
AnytoneCodeplug::GeneralSettingsElement::setPowerSave(PowerSave mode) {
  setUInt8(0x000b, (uint)mode);
}

uint
AnytoneCodeplug::GeneralSettingsElement::voxLevel() const {
  return ((uint)getUInt8(0x000c))*3;
}
void
AnytoneCodeplug::GeneralSettingsElement::setVOXLevel(uint level) {
  setUInt8(0x000c, level/3);
}

uint
AnytoneCodeplug::GeneralSettingsElement::voxDelay() const {
  return 100 + 500*((uint)getUInt8(0x000d));
}
void
AnytoneCodeplug::GeneralSettingsElement::setVOXDelay(uint ms) {
  if (ms < 100)
    ms = 100;
  setUInt8(0x000d, (ms-100)/500);
}

AnytoneCodeplug::GeneralSettingsElement::VFOScanType
AnytoneCodeplug::GeneralSettingsElement::vfoScanType() const {
  return (VFOScanType) getUInt8(0x000e);
}
void
AnytoneCodeplug::GeneralSettingsElement::setVFOScanType(VFOScanType type) {
  setUInt8(0x000e, (uint)type);
}

uint
AnytoneCodeplug::GeneralSettingsElement::micGain() const {
  return (((uint)getUInt8(0x000f))*10)/4;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMICGain(uint gain) {
  setUInt8(0x000f, (gain*4)/10);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::progFuncKey1Short() const {
  return (KeyFunction)getUInt8(0x0010);
}
void
AnytoneCodeplug::GeneralSettingsElement::setProgFuncKey1Short(KeyFunction func) {
  setUInt8(0x0010, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::progFuncKey2Short() const {
  return (KeyFunction)getUInt8(0x0011);
}
void
AnytoneCodeplug::GeneralSettingsElement::setProgFuncKey2Short(KeyFunction func) {
  setUInt8(0x0011, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::progFuncKey3Short() const {
  return (KeyFunction)getUInt8(0x0012);
}
void
AnytoneCodeplug::GeneralSettingsElement::setProgFuncKey3Short(KeyFunction func) {
  setUInt8(0x0012, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::funcKey1Short() const {
  return (KeyFunction)getUInt8(0x0013);
}
void
AnytoneCodeplug::GeneralSettingsElement::setFuncKey1Short(KeyFunction func) {
  setUInt8(0x0013, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::funcKey2Short() const {
  return (KeyFunction)getUInt8(0x0014);
}
void
AnytoneCodeplug::GeneralSettingsElement::setFuncKey2Short(KeyFunction func) {
  setUInt8(0x0014, (uint)func);
}

bool
AnytoneCodeplug::GeneralSettingsElement::vfoModeB() const {
  return getUInt8(0x0015);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableVFOModeB(bool enable) {
  setUInt8(0x0015, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::vfoModeA() const {
  return getUInt8(0x0016);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableVFOModeA(bool enable) {
  setUInt8(0x0016, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::memoryZoneA() const {
  return getUInt8(0x001f);
}
void
AnytoneCodeplug::GeneralSettingsElement::setMemoryZoneA(uint zone) {
  setUInt8(0x001f, zone);
}

uint
AnytoneCodeplug::GeneralSettingsElement::memoryZoneB() const {
  return getUInt8(0x0020);
}
void
AnytoneCodeplug::GeneralSettingsElement::setMemoryZoneB(uint zone) {
  setUInt8(0x0020, zone);
}

bool
AnytoneCodeplug::GeneralSettingsElement::recording() const {
  return getUInt8(0x0022);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableRecording(bool enable) {
  setUInt8(0x0022, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::brightness() const {
  return (getUInt8(0x0026)*10)/4;
}
void
AnytoneCodeplug::GeneralSettingsElement::setBrightness(uint level) {
  setUInt8(0x0026, (level*4)/10);
}

bool
AnytoneCodeplug::GeneralSettingsElement::backlightPermanent() const {
  return 0 == backlightDuration();
}
uint
AnytoneCodeplug::GeneralSettingsElement::backlightDuration() const {
  return 5*((uint)getUInt8(0x0027));
}
void
AnytoneCodeplug::GeneralSettingsElement::setBacklightDuration(uint sec) {
  setUInt8(0x0027, sec/5);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableBacklightPermanent() {
  setBacklightDuration(0);
}

bool
AnytoneCodeplug::GeneralSettingsElement::gps() const {
  return getUInt8(0x0028);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableGPS(bool enable) {
  setUInt8(0x0028, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::smsAlert() const {
  return getUInt8(0x0029);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableSMSAlert(bool enable) {
  setUInt8(0x0029, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::activeChannelB() const {
  return getUInt8(0x002c);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableActiveChannelB(bool enable) {
  setUInt8(0x002c, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::subChannel() const {
  return getUInt8(0x002d);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableSubChannel(bool enable) {
  setUInt8(0x002d, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::callAlert() const {
  return getUInt8(0x002f);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableCallAlert(bool enable) {
  setUInt8(0x002f, (enable ? 0x01 : 0x00));
}

QTimeZone
AnytoneCodeplug::GeneralSettingsElement::gpsTimeZone() const {
  return QTimeZone((((int)getUInt8(0x0030))-12)*3600);
}
void
AnytoneCodeplug::GeneralSettingsElement::setGPSTimeZone(const QTimeZone &zone) {
  int offset = zone.offsetFromUtc(QDateTime::currentDateTime());
  setUInt8(0x0030, (12 + offset/3600));
}

bool
AnytoneCodeplug::GeneralSettingsElement::talkPermitDigital() const {
  return getBit(0x0031, 0);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableTalkPermitDigital(bool enable) {
  return setBit(0x0031, 0, enable);
}

bool
AnytoneCodeplug::GeneralSettingsElement::talkPermitAnalog() const {
  return getBit(0x0031, 1);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableTalkPermitAnalog(bool enable) {
  return setBit(0x0031, 1, enable);
}

bool
AnytoneCodeplug::GeneralSettingsElement::digitalResetTone() const {
  return getUInt8(0x0032);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableDigitalResetTone(bool enable) {
  return setUInt8(0x0032, (enable ? 0x01 : 0x00));
}

AnytoneCodeplug::GeneralSettingsElement::VoxSource
AnytoneCodeplug::GeneralSettingsElement::voxSource() const {
  return (VoxSource)getUInt8(0x0033);
}
void
AnytoneCodeplug::GeneralSettingsElement::setVOXSource(VoxSource source) {
  setUInt8(0x0033, (uint)source);
}

bool
AnytoneCodeplug::GeneralSettingsElement::idleChannelTone() const {
  return getUInt8(0x0036);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableIdleChannelTone(bool enable) {
  return setUInt8(0x0036, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::menuExitTime() const {
  return 5 + 5*((uint) getUInt8(0x0037));
}
void
AnytoneCodeplug::GeneralSettingsElement::setMenuExitTime(uint sec) {
  if (sec < 5)
    sec = 5;
  setUInt8(0x0037, (sec-5)/5);
}

bool
AnytoneCodeplug::GeneralSettingsElement::startupTone() const {
  return getUInt8(0x0039);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableStartupTone(bool enable) {
  return setUInt8(0x0039, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::callEndPrompt() const {
  return getUInt8(0x003a);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableCallEndPrompt(bool enable) {
  return setUInt8(0x003a, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::maxVolume() const {
  return (((uint)getUInt8(0x003b))*10)/8;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMaxVolume(uint level) {
  setUInt8(0x003b, (level*8)/10);
}

bool
AnytoneCodeplug::GeneralSettingsElement::getGPSPosition() const {
  return getUInt8(0x003f);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableGetGPSPosition(bool enable) {
  return setUInt8(0x003f, (enable ? 0x01 : 0x00));
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::progFuncKey1Long() const {
  return (KeyFunction)getUInt8(0x0041);
}
void
AnytoneCodeplug::GeneralSettingsElement::setProgFuncKey1Long(KeyFunction func) {
  setUInt8(0x0041, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::progFuncKey2Long() const {
  return (KeyFunction)getUInt8(0x0042);
}
void
AnytoneCodeplug::GeneralSettingsElement::setProgFuncKey2Long(KeyFunction func) {
  setUInt8(0x0042, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::progFuncKey3Long() const {
  return (KeyFunction)getUInt8(0x0043);
}
void
AnytoneCodeplug::GeneralSettingsElement::setProgFuncKey3Long(KeyFunction func) {
  setUInt8(0x0043, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::funcKey1Long() const {
  return (KeyFunction)getUInt8(0x0044);
}
void
AnytoneCodeplug::GeneralSettingsElement::setFuncKey1Long(KeyFunction func) {
  setUInt8(0x0044, (uint)func);
}

AnytoneCodeplug::GeneralSettingsElement::KeyFunction
AnytoneCodeplug::GeneralSettingsElement::funcKey2Long() const {
  return (KeyFunction)getUInt8(0x0045);
}
void
AnytoneCodeplug::GeneralSettingsElement::setFuncKey2Long(KeyFunction func) {
  setUInt8(0x0045, (uint)func);
}

uint
AnytoneCodeplug::GeneralSettingsElement::longPressDuration() const {
  return (((uint)getUInt8(0x0046))+1)*1000;
}
void
AnytoneCodeplug::GeneralSettingsElement::setLongPressDuration(uint ms) {
  if (ms < 1000)
    ms = 1000;
  setUInt8(0x0046, ms/1000-1);
}

bool
AnytoneCodeplug::GeneralSettingsElement::volumeChangePrompt() const {
  return getUInt8(0x0047);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableVolumeChangePrompt(bool enable) {
  setUInt8(0x0047, (enable ? 0x01 : 0x01));
}

AnytoneCodeplug::GeneralSettingsElement::AutoRepDir
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterDirectionA() const {
  return (AutoRepDir) getUInt8(0x0048);
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterDirectionA(AutoRepDir dir) {
  setUInt8(0x0048, (uint)dir);
}

AnytoneCodeplug::GeneralSettingsElement::LastCallerDisplayMode
AnytoneCodeplug::GeneralSettingsElement::lastCallerDisplayMode() const {
  return (LastCallerDisplayMode)getUInt8(0x004d);
}
void
AnytoneCodeplug::GeneralSettingsElement::setLastCallerDisplayMode(LastCallerDisplayMode mode) {
  setUInt8(0x004d, (uint)mode);
}

bool
AnytoneCodeplug::GeneralSettingsElement::displayClock() const {
  return getUInt8(0x0051);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableDisplayClock(bool enable) {
  setUInt8(0x0051, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::maxHeadphoneVolume() const {
  return (((uint)getUInt8(0x0052))*10)/8;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMaxHeadPhoneVolume(uint max) {
  setUInt8(0x0052, (max*8)/10);
}

bool
AnytoneCodeplug::GeneralSettingsElement::enhanceAudio() const {
  return getUInt8(0x0057);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableEnhancedAudio(bool enable) {
  setUInt8(0x0057, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::minVFOScanFrequencyUHF() const {
  return ((uint)getBCD8_le(0x0058))*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMinVFOScanFrequencyUHF(uint hz) {
  setBCD8_le(0x0058, hz/10);
}

uint
AnytoneCodeplug::GeneralSettingsElement::maxVFOScanFrequencyUHF() const {
  return ((uint)getBCD8_le(0x005c))*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMaxVFOScanFrequencyUHF(uint hz) {
  setBCD8_le(0x005c, hz/10);
}

uint
AnytoneCodeplug::GeneralSettingsElement::minVFOScanFrequencyVHF() const {
  return ((uint)getBCD8_le(0x0060))*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMinVFOScanFrequencyVHF(uint hz) {
  setBCD8_le(0x0060, hz/10);
}

uint
AnytoneCodeplug::GeneralSettingsElement::maxVFOScanFrequencyVHF() const {
  return ((uint)getBCD8_le(0x0064))*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setMaxVFOScanFrequencyVHF(uint hz) {
  setBCD8_le(0x0064, hz/10);
}

bool
AnytoneCodeplug::GeneralSettingsElement::hasAutoRepeaterOffsetFrequencyIndexUHF() const {
  return 0xff != autoRepeaterOffsetFrequencyIndexUHF();
}
uint
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterOffsetFrequencyIndexUHF() const {
  return getUInt8(0x0068);
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterOffsetFrequenyIndexUHF(uint idx) {
  setUInt8(0x0068, idx);
}
void
AnytoneCodeplug::GeneralSettingsElement::clearAutoRepeaterOffsetFrequencyIndexUHF() {
  setAutoRepeaterOffsetFrequenyIndexUHF(0xff);
}

bool
AnytoneCodeplug::GeneralSettingsElement::hasAutoRepeaterOffsetFrequencyIndexVHF() const {
  return 0xff != autoRepeaterOffsetFrequencyIndexVHF();
}
uint
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterOffsetFrequencyIndexVHF() const {
  return getUInt8(0x0069);
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterOffsetFrequenyIndexVHF(uint idx) {
  setUInt8(0x0069, idx);
}
void
AnytoneCodeplug::GeneralSettingsElement::clearAutoRepeaterOffsetFrequencyIndexVHF() {
  setAutoRepeaterOffsetFrequenyIndexVHF(0xff);
}

AnytoneCodeplug::GeneralSettingsElement::Melody
AnytoneCodeplug::GeneralSettingsElement::callToneMelody() const {
  Melody melody;
  for (int i=0; i<5; i++) {
    melody.notes[i].frequency = getUInt16_le(0x0072+2*i);
    melody.notes[i].duration  = getUInt16_le(0x007c+2*i);
  }
  return melody;
}
void
AnytoneCodeplug::GeneralSettingsElement::setCallToneMelody(const Melody &melody) {
  for (int i=0; i<5; i++) {
    setUInt16_le(0x0072+2*i, melody.notes[i].frequency);
    setUInt16_le(0x007c+2*i, melody.notes[i].duration);
  }
}

AnytoneCodeplug::GeneralSettingsElement::Melody
AnytoneCodeplug::GeneralSettingsElement::idleToneMelody() const {
  Melody melody;
  for (int i=0; i<5; i++) {
    melody.notes[i].frequency = getUInt16_le(0x0086+2*i);
    melody.notes[i].duration  = getUInt16_le(0x0090+2*i);
  }
  return melody;
}
void
AnytoneCodeplug::GeneralSettingsElement::setIdleToneMelody(const Melody &melody) {
  for (int i=0; i<5; i++) {
    setUInt16_le(0x0086+2*i, melody.notes[i].frequency);
    setUInt16_le(0x0090+2*i, melody.notes[i].duration);
  }
}

AnytoneCodeplug::GeneralSettingsElement::Melody
AnytoneCodeplug::GeneralSettingsElement::resetToneMelody() const {
  Melody melody;
  for (int i=0; i<5; i++) {
    melody.notes[i].frequency = getUInt16_le(0x009a+2*i);
    melody.notes[i].duration  = getUInt16_le(0x00a4+2*i);
  }
  return melody;
}
void
AnytoneCodeplug::GeneralSettingsElement::setResetToneMelody(const Melody &melody) {
  for (int i=0; i<5; i++) {
    setUInt16_le(0x009a+2*i, melody.notes[i].frequency);
    setUInt16_le(0x00a4+2*i, melody.notes[i].duration);
  }
}

uint
AnytoneCodeplug::GeneralSettingsElement::recordingDelay() const {
  return ((uint)getUInt8(0x00ae))*200;
}
void
AnytoneCodeplug::GeneralSettingsElement::setRecodringDelay(uint ms) {
  setUInt8(0x00ae, ms/200);
}

bool
AnytoneCodeplug::GeneralSettingsElement::displayCall() const {
  return getUInt8(0x00af);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableDisplayCall(bool enable) {
  setUInt8(0x00af, (enable ? 0x01 : 0x00));
}

AnytoneCodeplug::GeneralSettingsElement::Color
AnytoneCodeplug::GeneralSettingsElement::callDisplayColor() const {
  return (Color)getUInt8(0x00b0);
}
void
AnytoneCodeplug::GeneralSettingsElement::setCallDisplayColor(Color color) {
  setUInt8(0x00b0, (uint)color);
}

uint
AnytoneCodeplug::GeneralSettingsElement::gpsUpdatePeriod() const {
  return getUInt8(0x00b1);
}
void
AnytoneCodeplug::GeneralSettingsElement::setGPSUpdatePeriod(uint sec) {
  setUInt8(0x00b1, sec);
}

bool
AnytoneCodeplug::GeneralSettingsElement::showZoneAndContact() const {
  return getUInt8(0x00b2);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableShowZoneAndContact(bool enable) {
  setUInt8(0x00b2, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::keyToneLevelAdjustable() const {
  return 0 == keyToneLevel();
}
uint
AnytoneCodeplug::GeneralSettingsElement::keyToneLevel() const {
  return ((uint)getUInt8(0x00b3))*10/15;
}
void
AnytoneCodeplug::GeneralSettingsElement::setKeyToneLevel(uint level) {
  setUInt8(0x00b3, level*10/15);
}
void
AnytoneCodeplug::GeneralSettingsElement::setKeyToneLevelAdjustable() {
  setUInt8(0x00b3, 0);
}

bool
AnytoneCodeplug::GeneralSettingsElement::gpsUnitsImperial() const {
  return getUInt8(0x00b4);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableGPSUnitsImperial(bool enable) {
  setUInt8(0x00b4, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::GeneralSettingsElement::knobLock() const {
  return getBit(0x00b5, 0);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableKnobLock(bool enable) {
  setBit(0x00b5, 0, enable);
}
bool
AnytoneCodeplug::GeneralSettingsElement::keypadLock() const {
  return getBit(0x00b5, 1);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableKeypadLock(bool enable) {
  setBit(0x00b5, 1, enable);
}
bool
AnytoneCodeplug::GeneralSettingsElement::sidekeysLock() const {
  return getBit(0x00b5, 3);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableSidekeysLock(bool enable) {
  setBit(0x00b5, 3, enable);
}
bool
AnytoneCodeplug::GeneralSettingsElement::professionalKeyLock() const {
  return getBit(0x00b5, 4);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableProfessionalKeyLock(bool enable) {
  setBit(0x00b5, 4, enable);
}

bool
AnytoneCodeplug::GeneralSettingsElement::showLastHeard() const {
  return getUInt8(0x00b6);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableShowLastHeard(bool enable) {
  setUInt8(0x00b6, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterMinFrequencyVHF() const {
  return getBCD8_le(0x00b8)*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterMinFrequencyVHF(uint Hz) {
  setBCD8_le(0x00b8, Hz/10);
}
uint
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterMaxFrequencyVHF() const {
  return getBCD8_le(0x00bc)*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterMaxFrequencyVHF(uint Hz) {
  setBCD8_le(0x00bc, Hz/10);
}

uint
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterMinFrequencyUHF() const {
  return getBCD8_le(0x00c0)*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterMinFrequencyUHF(uint Hz) {
  setBCD8_le(0x00c0, Hz/10);
}
uint
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterMaxFrequencyUHF() const {
  return getBCD8_le(0x00c4)*10;
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterMaxFrequencyUHF(uint Hz) {
  setBCD8_le(0x00c4, Hz/10);
}

AnytoneCodeplug::GeneralSettingsElement::AutoRepDir
AnytoneCodeplug::GeneralSettingsElement::autoRepeaterDirectionB() const {
  return (AutoRepDir)getUInt8(0x00c8);
}
void
AnytoneCodeplug::GeneralSettingsElement::setAutoRepeaterDirectionB(AutoRepDir dir) {
  setUInt8(0x00c8, (uint)dir);
}

bool
AnytoneCodeplug::GeneralSettingsElement::defaultChannel() const {
  return getUInt8(0x00ca);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableDefaultChannel(bool enable) {
  setUInt8(0x00ca, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::GeneralSettingsElement::defaultZoneIndexA() const {
  return getUInt8(0x00cb);
}
void
AnytoneCodeplug::GeneralSettingsElement::setDefaultZoneIndexA(uint idx) {
  setUInt8(0x00cb, idx);
}

uint
AnytoneCodeplug::GeneralSettingsElement::defaultZoneIndexB() const {
  return getUInt8(0x00cc);
}
void
AnytoneCodeplug::GeneralSettingsElement::setDefaultZoneIndexB(uint idx) {
  setUInt8(0x00cc, idx);
}

bool
AnytoneCodeplug::GeneralSettingsElement::defaultChannelAIsVFO() const {
  return 0xff == defaultChannelAIndex();
}
uint
AnytoneCodeplug::GeneralSettingsElement::defaultChannelAIndex() const {
  return getUInt8(0x00cd);
}
void
AnytoneCodeplug::GeneralSettingsElement::setDefaultChannelAIndex(uint idx) {
  setUInt8(0x00cd, idx);
}
void
AnytoneCodeplug::GeneralSettingsElement::setDefaultChannelAToVFO() {
  setDefaultChannelAIndex(0xff);
}

bool
AnytoneCodeplug::GeneralSettingsElement::defaultChannelBIsVFO() const {
  return 0xff == defaultChannelBIndex();
}
uint
AnytoneCodeplug::GeneralSettingsElement::defaultChannelBIndex() const {
  return getUInt8(0x00ce);
}
void
AnytoneCodeplug::GeneralSettingsElement::setDefaultChannelBIndex(uint idx) {
  setUInt8(0x00ce, idx);
}
void
AnytoneCodeplug::GeneralSettingsElement::setDefaultChannelBToVFO() {
  setDefaultChannelBIndex(0xff);
}

bool
AnytoneCodeplug::GeneralSettingsElement::keepLastCaller() const {
  return getUInt8(0x00cf);
}
void
AnytoneCodeplug::GeneralSettingsElement::enableKeepLastCaller(bool enable) {
  setUInt8(0x00cf, (enable ? 0x01 : 0x00));
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::ZoneChannelListElement
 * ********************************************************************************************* */
AnytoneCodeplug::ZoneChannelListElement::ZoneChannelListElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pas...
}

AnytoneCodeplug::ZoneChannelListElement::ZoneChannelListElement(uint8_t *ptr)
  : Element(ptr, 0x0400)
{
  // pass...
}

void
AnytoneCodeplug::ZoneChannelListElement::clear() {
  memset(_data, 0x00, _size);
  memset(_data, 0xff, 2*250);
  memset(_data+0x200, 0xff, 2*250);
}

bool
AnytoneCodeplug::ZoneChannelListElement::hasChannelA(uint n) const {
  return 0xffff == channelIndexA(n);
}
uint
AnytoneCodeplug::ZoneChannelListElement::channelIndexA(uint n) const {
  return getUInt16_le(0x0000 + 2*n);
}
void
AnytoneCodeplug::ZoneChannelListElement::setChannelIndexA(uint n, uint idx) {
  setUInt16_le(0x0000 + 2*n, idx);
}
void
AnytoneCodeplug::ZoneChannelListElement::clearChannelIndexA(uint n) {
  setChannelIndexA(n, 0xffff);
}

bool
AnytoneCodeplug::ZoneChannelListElement::hasChannelB(uint n) const {
  return 0xffff == channelIndexB(n);
}
uint
AnytoneCodeplug::ZoneChannelListElement::channelIndexB(uint n) const {
  return getUInt16_le(0x0200 + 2*n);
}
void
AnytoneCodeplug::ZoneChannelListElement::setChannelIndexB(uint n, uint idx) {
  setUInt16_le(0x0200 + 2*n, idx);
}
void
AnytoneCodeplug::ZoneChannelListElement::clearChannelIndexB(uint n) {
  setChannelIndexB(n, 0xffff);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::BootSettingsElement
 * ********************************************************************************************* */
AnytoneCodeplug::BootSettingsElement::BootSettingsElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::BootSettingsElement::BootSettingsElement(uint8_t *ptr)
  : Element(ptr, 0x0030)
{
  // pass...
}

void
AnytoneCodeplug::BootSettingsElement::clear() {
  memset(_data, 0x00, _size);
}

QString
AnytoneCodeplug::BootSettingsElement::introLine1() const {
  return readASCII(0x0000, 16, 0x00);
}
void
AnytoneCodeplug::BootSettingsElement::setIntroLine1(const QString &txt) {
  writeASCII(0x0000, txt, 16, 0x00);
}
QString
AnytoneCodeplug::BootSettingsElement::introLine2() const {
  return readASCII(0x0010, 16, 0x00);
}
void
AnytoneCodeplug::BootSettingsElement::setIntroLine2(const QString &txt) {
  writeASCII(0x0010, txt, 16, 0x00);
}

QString
AnytoneCodeplug::BootSettingsElement::password() const {
  return readASCII(0x0020, 16, 0x00);
}
void
AnytoneCodeplug::BootSettingsElement::setPassword(const QString &txt) {
  writeASCII(0x0020, txt, 16, 0x00);
}

/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::DMRAPRSSettingsElement
 * ********************************************************************************************* */
AnytoneCodeplug::DMRAPRSSettingsElement::DMRAPRSSettingsElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::DMRAPRSSettingsElement::DMRAPRSSettingsElement(uint8_t *ptr)
  : Element(ptr, 0x0030)
{
  // pass...
}

void
AnytoneCodeplug::DMRAPRSSettingsElement::clear() {
  memset(_data, 0x00, _size);
}

uint
AnytoneCodeplug::DMRAPRSSettingsElement::manualInterval() const {
  return getUInt8(0x0000);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setManualInterval(uint sec) {
  setUInt8(0x0000, sec);
}

bool
AnytoneCodeplug::DMRAPRSSettingsElement::automatic() const {
  return 0!=getUInt8(0x0001);
}
uint
AnytoneCodeplug::DMRAPRSSettingsElement::automaticInterval() const {
  return 45 + 15*((uint)getUInt8(0x0001));
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setAutomaticInterval(uint sec) {
  if (sec<60)
    sec = 60;
  setUInt8(0x0001, (sec-45)/15);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::disableAutomatic() {
  setUInt8(0x0001, 0x00);
}

bool
AnytoneCodeplug::DMRAPRSSettingsElement::fixedLocation() const {
  return getUInt8(0x0002);
}
QGeoCoordinate
AnytoneCodeplug::DMRAPRSSettingsElement::location() const {
  double latitude  = getUInt8(0x0003) + double(getUInt8(0x0004))/60 + double(getUInt8(0x0005))/3600;
  if (getUInt8(0x0006)) latitude *= -1;
  double longitude = getUInt8(0x0007) + double(getUInt8(0x0008))/60 + double(getUInt8(0x0009))/3600;
  if (getUInt8(0x000a)) longitude *= -1;
  return QGeoCoordinate(latitude, longitude);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setLocation(const QGeoCoordinate &pos) {
  double latitude = pos.latitude();
  bool south = (0 > latitude); latitude = std::abs(latitude);
  uint lat_deg = int(latitude); latitude -= lat_deg; latitude *= 60;
  uint lat_min = int(latitude); latitude -= lat_min; latitude *= 60;
  uint lat_sec = int(latitude);
  double longitude = pos.longitude();
  bool west = (0 > longitude); longitude = std::abs(longitude);
  uint lon_deg = int(longitude); longitude -= lon_deg; longitude *= 60;
  uint lon_min = int(longitude); longitude -= lon_min; longitude *= 60;
  uint lon_sec = int(longitude);
  setUInt8(0x0003, lat_deg); setUInt8(0x0004, lat_min); setUInt8(0x0005, lat_sec); setUInt8(0x0006, (south ? 0x01 : 0x00));
  setUInt8(0x0007, lon_deg); setUInt8(0x0008, lon_min); setUInt8(0x0009, lon_sec); setUInt8(0x000a, (west ? 0x01 : 0x00));
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::enableFixedLocation(bool enable) {
  setUInt8(0x0002, (enable ? 0x01 : 0x00));
}

Channel::Power
AnytoneCodeplug::DMRAPRSSettingsElement::power() const {
  switch (getUInt8(0x000b)) {
  case 0x00: return Channel::Power::Low;
  case 0x01: return Channel::Power::Mid;
  case 0x02: return Channel::Power::High;
  case 0x03: return Channel::Power::Max;
  }
  return Channel::Power::Low;
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setPower(Channel::Power power) {
  switch (power) {
  case Channel::Power::Min:
  case Channel::Power::Low:
    setUInt8(0x000b, 0x00);
    break;
  case Channel::Power::Mid:
    setUInt8(0x000b, 0x01);
    break;
  case Channel::Power::High:
    setUInt8(0x000b, 0x02);
    break;
  case Channel::Power::Max:
    setUInt8(0x000b, 0x03);
    break;
  }
}

bool
AnytoneCodeplug::DMRAPRSSettingsElement::hasChannel(uint n) const {
  return 0xffff != channelIndex(n);
}
bool
AnytoneCodeplug::DMRAPRSSettingsElement::channelIsVFOA(uint n) const {
  return 0x0fa0 == channelIndex(n);
}
bool
AnytoneCodeplug::DMRAPRSSettingsElement::channelIsVFOB(uint n) const {
  return 0x0fa1 == channelIndex(n);
}
bool
AnytoneCodeplug::DMRAPRSSettingsElement::channelIsSelected(uint n) const {
  return 0x0fa2 == channelIndex(n);
}
uint
AnytoneCodeplug::DMRAPRSSettingsElement::channelIndex(uint n) const {
  return getUInt16_le(0x000c + 2*n);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setChannelIndex(uint n, uint idx) {
  setUInt16_le(0x000c + 2*n, idx);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setChannelVFOA(uint n) {
  setChannelIndex(n, 0x0fa0);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setChannelVFOB(uint n) {
  setChannelIndex(n, 0x0fa1);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setChannelSelected(uint n) {
  setChannelIndex(n, 0x0fa2);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::clearChannel(uint n) {
  setChannelIndex(n, 0xffff);
}

uint
AnytoneCodeplug::DMRAPRSSettingsElement::destination() const {
  return getBCD8_le(0x001c);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setDestination(uint id) {
  setBCD8_le(0x001c, id);
}

DigitalContact::Type
AnytoneCodeplug::DMRAPRSSettingsElement::callType() const {
  switch (getUInt8(0x0020)) {
  case 0x00: return DigitalContact::PrivateCall;
  case 0x01: return DigitalContact::GroupCall;
  case 0x02: return DigitalContact::AllCall;
  }
  return DigitalContact::PrivateCall;
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::setCallType(DigitalContact::Type type) {
  switch (type) {
  case DigitalContact::PrivateCall: setUInt8(0x0020, 0x00); break;
  case DigitalContact::GroupCall: setUInt8(0x0020, 0x01); break;
  case DigitalContact::AllCall: setUInt8(0x0020, 0x02); break;
  }
}

bool
AnytoneCodeplug::DMRAPRSSettingsElement::timeSlotOverride() const {
  return 0 != getUInt8(0x0021);
}
DigitalChannel::TimeSlot
AnytoneCodeplug::DMRAPRSSettingsElement::timeslot() const {
  if (1 == getUInt8(0x0021))
    return DigitalChannel::TimeSlot::TS1;
  else if (2 == getUInt8(0x0021))
    return DigitalChannel::TimeSlot::TS2;
  return DigitalChannel::TimeSlot::TS1;
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::overrideTimeSlot(DigitalChannel::TimeSlot ts) {
  if (DigitalChannel::TimeSlot::TS1 == ts)
    setUInt8(0x0021, 0x01);
  else
    setUInt8(0x0021, 0x02);
}
void
AnytoneCodeplug::DMRAPRSSettingsElement::disableTimeSlotOverride() {
  setUInt8(0x0021, 0x00);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::MessageListElement
 * ********************************************************************************************* */
AnytoneCodeplug::MessageListElement::MessageListElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::MessageListElement::MessageListElement(uint8_t *ptr)
  : Element(ptr, 0x0010)
{
  // pass...
}

void
AnytoneCodeplug::MessageListElement::clear() {
  memset(_data, 0x00, _size);
  clearNext();
  clearIndex();
}

bool
AnytoneCodeplug::MessageListElement::hasNext() const {
  return 0xff != next();
}
uint
AnytoneCodeplug::MessageListElement::next() const {
  return getUInt8(0x0002);
}
void
AnytoneCodeplug::MessageListElement::setNext(uint idx) {
  setUInt8(0x0002, idx);
}
void
AnytoneCodeplug::MessageListElement::clearNext() {
  setNext(0xff);
}

bool
AnytoneCodeplug::MessageListElement::hasIndex() const {
  return 0xff != index();
}
uint
AnytoneCodeplug::MessageListElement::index() const {
  return getUInt8(0x0003);
}
void
AnytoneCodeplug::MessageListElement::setIndex(uint idx) {
  setUInt8(0x0003, idx);
}
void
AnytoneCodeplug::MessageListElement::clearIndex() {
  setIndex(0xff);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::MessageElement
 * ********************************************************************************************* */
AnytoneCodeplug::MessageElement::MessageElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::MessageElement::MessageElement(uint8_t *ptr)
  : Element(ptr, 0x0100)
{
  // pass...
}

void
AnytoneCodeplug::MessageElement::clear() {
  memset(_data, 0x00, _size);
}

QString
AnytoneCodeplug::MessageElement::message() const {
  return readASCII(0x0000, 99, 0x00);
}
void
AnytoneCodeplug::MessageElement::setMessage(const QString &msg) {
  writeASCII(0x0000, msg, 99, 0x00);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::AnalogQuickCallElement
 * ********************************************************************************************* */
AnytoneCodeplug::AnalogQuickCallElement::AnalogQuickCallElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::AnalogQuickCallElement::AnalogQuickCallElement(uint8_t *ptr)
  : Element(ptr, 0x0002)
{
  // pass...
}

void
AnytoneCodeplug::AnalogQuickCallElement::clear() {
  memset(_data, 0x00, _size);
  clearContactIndex();
}

AnytoneCodeplug::AnalogQuickCallElement::Type
AnytoneCodeplug::AnalogQuickCallElement::type() const {
  return (Type)getUInt8(0x0000);
}
void
AnytoneCodeplug::AnalogQuickCallElement::setType(Type type) {
  setUInt8(0x0000, (uint)type);
}

bool
AnytoneCodeplug::AnalogQuickCallElement::hasContactIndex() const {
  return 0xff != contactIndex();
}
uint
AnytoneCodeplug::AnalogQuickCallElement::contactIndex() const {
  return getUInt8(0x0001);
}
void
AnytoneCodeplug::AnalogQuickCallElement::setContactIndex(uint idx) {
  setUInt8(0x0001, idx);
}
void
AnytoneCodeplug::AnalogQuickCallElement::clearContactIndex() {
  setContactIndex(0xff);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::HotKeyElement
 * ********************************************************************************************* */
AnytoneCodeplug::HotKeyElement::HotKeyElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::HotKeyElement::HotKeyElement(uint8_t *ptr)
  : Element(ptr, 0x0030)
{
  // pass...
}

void
AnytoneCodeplug::HotKeyElement::clear() {
  memset(_data, 0x00, _size);
  clearContactIndex();
  clearMessageIndex();
}

AnytoneCodeplug::HotKeyElement::Type
AnytoneCodeplug::HotKeyElement::type() const {
  return (Type)getUInt8(0x0000);
}
void
AnytoneCodeplug::HotKeyElement::setType(Type type) {
  setUInt8(0x0000, (uint)type);
}

AnytoneCodeplug::HotKeyElement::MenuItem
AnytoneCodeplug::HotKeyElement::menuItem() const {
  return (MenuItem) getUInt8(0x0001);
}
void
AnytoneCodeplug::HotKeyElement::setMenuItem(MenuItem item) {
  setUInt8(0x0001, (uint)item);
}

AnytoneCodeplug::HotKeyElement::CallType
AnytoneCodeplug::HotKeyElement::callType() const {
  return (CallType)getUInt8(0x0002);
}
void
AnytoneCodeplug::HotKeyElement::setCallType(CallType type) {
  setUInt8(0x0002, (uint)type);
}

AnytoneCodeplug::HotKeyElement::DigiCallType
AnytoneCodeplug::HotKeyElement::digiCallType() const {
  return (DigiCallType)getUInt8(0x0003);
}
void
AnytoneCodeplug::HotKeyElement::setDigiCallType(DigiCallType type) {
  setUInt8(0x0003, (uint)type);
}

bool
AnytoneCodeplug::HotKeyElement::hasContactIndex() const {
  return 0xffffffff != contactIndex();
}
uint
AnytoneCodeplug::HotKeyElement::contactIndex() const {
  return getUInt32_le(0x0004);
}
void
AnytoneCodeplug::HotKeyElement::setContactIndex(uint idx) {
  setUInt32_le(0x0004, idx);
}
void
AnytoneCodeplug::HotKeyElement::clearContactIndex() {
  setContactIndex(0xffffffff);
}

bool
AnytoneCodeplug::HotKeyElement::hasMessageIndex() const {
  return 0xff != messageIndex();
}
uint
AnytoneCodeplug::HotKeyElement::messageIndex() const {
  return getUInt8(0x0008);
}
void
AnytoneCodeplug::HotKeyElement::setMessageIndex(uint idx) {
  setUInt8(0x0008, idx);
}
void
AnytoneCodeplug::HotKeyElement::clearMessageIndex() {
  setMessageIndex(0xff);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::AlarmSettingElement::AnalogAlarm
 * ********************************************************************************************* */
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::AnalogAlarm(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::AnalogAlarm(uint8_t *ptr)
  : Element(ptr, 0x000a)
{
  // pass...
}

void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::clear() {
  memset(_data, 0x00, _size);
  setChannelSelected();
}

AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::Action
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::action() const {
  return (Action) getUInt8(0x0000);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setAction(Action action) {
  setUInt8(0x0000, (uint)action);
}

AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::ENIType
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::encodingType() const {
  return (ENIType) getUInt8(0x0001);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setEncodingType(ENIType type) {
  setUInt8(0x0001, (uint)type);
}

uint
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::emergencyIndex() const {
  return getUInt8(0x0002);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setEmergencyIndex(uint idx) {
  setUInt8(0x0002, idx);
}

uint
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::duration() const {
  return getUInt8(0x0003);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setDuration(uint sec) {
  setUInt8(0x0003, sec);
}

uint
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::txDuration() const {
  return getUInt8(0x0004);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setTXDuration(uint sec) {
  setUInt8(0x0004, sec);
}

uint
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::rxDuration() const {
  return getUInt8(0x0005);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setRXDuration(uint sec) {
  setUInt8(0x0005, sec);
}

bool
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::channelIsSelected() const {
  return getUInt8(0x0008);
}
uint
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::channelIndex() const {
  return getUInt16_le(0x0006);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setChannelIndex(uint idx) {
  setUInt16_le(0x0006, idx);
  setUInt8(0x0008, 0x00);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setChannelSelected() {
  setUInt8(0x0008, 0x01);
}

bool
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::repeatContinuously() const {
  return 0 == repetitions();
}
uint
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::repetitions() const {
  return getUInt8(0x0009);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setRepetitions(uint num) {
  setUInt8(0x0009, num);
}
void
AnytoneCodeplug::AlarmSettingElement::AnalogAlarm::setRepatContinuously() {
  setRepetitions(0x00);
}

/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::AlarmSettingElement::DigitalAlarm
 * ********************************************************************************************* */
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::DigitalAlarm(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::DigitalAlarm(uint8_t *ptr)
  : Element(ptr, 0x000c)
{
  // pass...
}

void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::clear() {
  memset(_data, 0x00, _size);
  setChannelSelected();
}

AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::Action
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::action() const {
  return (Action) getUInt8(0x0000);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setAction(Action action) {
  setUInt8(0x0000, (uint)action);
}

uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::duration() const {
  return getUInt8(0x0001);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setDuration(uint sec) {
  setUInt8(0x0001, sec);
}

uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::txDuration() const {
  return getUInt8(0x0002);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setTXDuration(uint sec) {
  setUInt8(0x0002, sec);
}

uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::rxDuration() const {
  return getUInt8(0x0003);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setRXDuration(uint sec) {
  setUInt8(0x0003, sec);
}

bool
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::channelIsSelected() const {
  return getUInt8(0x0006);
}
uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::channelIndex() const {
  return getUInt16_le(0x0004);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setChannelIndex(uint idx) {
  setUInt16_le(0x0004, idx);
  setUInt8(0x0006, 0x00);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setChannelSelected() {
  setUInt8(0x0006, 0x01);
}

bool
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::repeatContinuously() const {
  return 0 == repetitions();
}
uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::repetitions() const {
  return getUInt8(0x0007);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setRepetitions(uint num) {
  setUInt8(0x0007, num);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setRepatContinuously() {
  setRepetitions(0x00);
}

uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::voiceBroadcastDuration() const {
  return getUInt8(0x0008)+1;
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setVoiceBroadcastDuration(uint min) {
  if (1 > min) min = 1;
  setUInt8(0x0008, min-1);
}

uint
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::areaBroadcastDuration() const {
  return getUInt8(0x0009)+1;
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::setAreaBroadcastDuration(uint min) {
  if (1 > min) min = 1;
  setUInt8(0x0009, min-1);
}

bool
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::vox() const {
  return getUInt8(0x000a);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::enableVOX(bool enable) {
  setUInt8(0x000a, (enable ? 0x01 : 0x00));
}

bool
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::rxAlarm() const {
  return getUInt8(0x000b);
}
void
AnytoneCodeplug::AlarmSettingElement::DigitalAlarm::enableRXAlarm(bool enable) {
  setUInt8(0x000b, (enable ? 0x01 : 0x00));
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::AlarmSettingElement
 * ********************************************************************************************* */
AnytoneCodeplug::AlarmSettingElement::AlarmSettingElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::AlarmSettingElement::AlarmSettingElement(uint8_t *ptr)
  : Element(ptr, 0x0020)
{
  // pass...
}

void
AnytoneCodeplug::AlarmSettingElement::clear() {
  AnalogAlarm(analog()).clear();
  DigitalAlarm(digital()).clear();
}

uint8_t *
AnytoneCodeplug::AlarmSettingElement::analog() const {
  return _data + 0x0000;
}
uint8_t *
AnytoneCodeplug::AlarmSettingElement::digital() const {
  return _data + 0x000a;
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::DigitalAlarmExtensionElement
 * ********************************************************************************************* */
AnytoneCodeplug::DigitalAlarmExtensionElement::DigitalAlarmExtensionElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::DigitalAlarmExtensionElement::DigitalAlarmExtensionElement(uint8_t *ptr)
  : Element(ptr, 0x0030)
{
  // pass...
}

void
AnytoneCodeplug::DigitalAlarmExtensionElement::clear() {
  memset(_data, 0x00, _size);
}

DigitalContact::Type
AnytoneCodeplug::DigitalAlarmExtensionElement::callType() const {
  switch (getUInt8(0x0000)) {
  case 0x00: return DigitalContact::PrivateCall;
  case 0x01: return DigitalContact::GroupCall;
  case 0x02: return DigitalContact::AllCall;
  }
  return DigitalContact::PrivateCall;
}
void
AnytoneCodeplug::DigitalAlarmExtensionElement::setCallType(DigitalContact::Type type) {
  switch (type) {
  case DigitalContact::PrivateCall: setUInt8(0x0000, 0x00); break;
  case DigitalContact::GroupCall: setUInt8(0x0000, 0x01); break;
  case DigitalContact::AllCall: setUInt8(0x0000, 0x02); break;
  }
}

uint
AnytoneCodeplug::DigitalAlarmExtensionElement::destination() const {
  return getBCD8_le(0x0023);
}
void
AnytoneCodeplug::DigitalAlarmExtensionElement::setDestination(uint number) {
  setBCD8_le(0x0023, number);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::FiveToneIDElement
 * ********************************************************************************************* */
AnytoneCodeplug::FiveToneIDElement::FiveToneIDElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::FiveToneIDElement::FiveToneIDElement(uint8_t *ptr)
  : Element(ptr, 0x0020)
{
  // pass...
}

void
AnytoneCodeplug::FiveToneIDElement::clear() {
  memset(_data, 0x00, _size);
}

AnytoneCodeplug::FiveToneIDElement::Standard
AnytoneCodeplug::FiveToneIDElement::standard() const {
  return (Standard) getUInt8(0x0001);
}
void
AnytoneCodeplug::FiveToneIDElement::setStandard(Standard std) {
  setUInt8(0x0001, (uint)std);
}

uint
AnytoneCodeplug::FiveToneIDElement::toneDuration() const {
  return getUInt8(0x0003);
}
void
AnytoneCodeplug::FiveToneIDElement::setToneDuration(uint ms) {
  setUInt8(0x0003, ms);
}

QString
AnytoneCodeplug::FiveToneIDElement::id() const {
  uint len = getUInt8(0x0002);
  QString id;
  for (uint i=0; i<len; i++) {
    uint8_t b = getUInt8(0x0004+(i/2));
    if (0 == (i%2))
      id.append(QString::number((b>>4)&0xf,16));
    else
      id.append(QString::number((b>>0)&0xf,16));
  }
  return id;
}
void
AnytoneCodeplug::FiveToneIDElement::setID(const QString &id) {
  uint len = 0;
  for (int i=0; i<id.length(); i++) {
    bool ok;
    if (0 == (len%2)) {
      setUInt4(0x0004+len/2, 4, id.mid(i, 1).toUInt(&ok, 16));
    } else {
      setUInt4(0x0004+len/2, 0, id.mid(i, 1).toUInt(&ok, 16));
    }
    len++;
  }
  setUInt8(0x0002, len);
}

QString
AnytoneCodeplug::FiveToneIDElement::name() const {
  return readASCII(0x0018, 7, 0x00);
}
void
AnytoneCodeplug::FiveToneIDElement::setName(const QString &name) {
  writeASCII(0x0018, name, 7, 0x00);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::FiveToneFunctionElement
 * ********************************************************************************************* */
AnytoneCodeplug::FiveToneFunctionElement::FiveToneFunctionElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::FiveToneFunctionElement::FiveToneFunctionElement(uint8_t *ptr)
  : Element(ptr, 0x0020)
{
  // pass...
}

void
AnytoneCodeplug::FiveToneFunctionElement::clear() {
  memset(_data, 0x00, _size);
}

AnytoneCodeplug::FiveToneFunctionElement::Function
AnytoneCodeplug::FiveToneFunctionElement::function() const {
  return (Function) getUInt8(0x0000);
}
void
AnytoneCodeplug::FiveToneFunctionElement::setFunction(Function function) {
  setUInt8(0x0000, (uint)function);
}

AnytoneCodeplug::FiveToneFunctionElement::Response
AnytoneCodeplug::FiveToneFunctionElement::response() const {
  return (Response) getUInt8(0x0001);
}
void
AnytoneCodeplug::FiveToneFunctionElement::setResponse(Response response) {
  setUInt8(0x0001, (uint)response);
}

QString
AnytoneCodeplug::FiveToneFunctionElement::id() const {
  uint len = getUInt8(0x0002);
  QString id;
  for (uint i=0; i<len; i++) {
    uint8_t b = getUInt8(0x0003+(i/2));
    if (0 == (i%2))
      id.append(QString::number((b>>4)&0xf, 16));
    else
      id.append(QString::number((b>>0)&0xf, 16));
  }
  return id;
}
void
AnytoneCodeplug::FiveToneFunctionElement::setID(const QString &id) {
  uint len = 0;
  for (int i=0; i<id.length(); i++) {
    bool ok;
    if (0 == (len%2)) {
      setUInt4(0x0003+len/2, 4, id.mid(i, 1).toUInt(&ok, 16));
    } else {
      setUInt4(0x0003+len/2, 0, id.mid(i, 1).toUInt(&ok, 16));
    }
    len++;
  }
  setUInt8(0x0002, len);
}

QString
AnytoneCodeplug::FiveToneFunctionElement::name() const {
  return readASCII(0x000f, 7, 0x00);
}
void
AnytoneCodeplug::FiveToneFunctionElement::setName(const QString &name) {
  writeASCII(0x000f, name, 7, 0x00);
}


/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug::FiveToneSettingsElement
 * ********************************************************************************************* */
AnytoneCodeplug::FiveToneSettingsElement::FiveToneSettingsElement(uint8_t *ptr, uint size)
  : Element(ptr, size)
{
  // pass...
}

AnytoneCodeplug::FiveToneSettingsElement::FiveToneSettingsElement(uint8_t *ptr)
  : Element(ptr, 0x0080)
{
  // pass...
}

void
AnytoneCodeplug::FiveToneSettingsElement::clear() {
  memset(_data, 0x00, _size);
}

AnytoneCodeplug::FiveToneSettingsElement::Response
AnytoneCodeplug::FiveToneSettingsElement::decodingResponse() const {
  return (Response) getUInt8(0x0021);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setDecodingResponse(Response response) {
  setUInt8(0x0021, (uint)response);
}

AnytoneCodeplug::FiveToneSettingsElement::Standard
AnytoneCodeplug::FiveToneSettingsElement::decodingStandard() const {
  return (Standard) getUInt8(0x0022);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setDecodingStandard(Standard standard) {
  setUInt8(0x0022, (uint)standard);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::decodingToneDuration() const {
  return getUInt8(0x0024);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setDecodingToneDuration(uint ms) {
  setUInt8(0x0024, ms);
}

QString
AnytoneCodeplug::FiveToneSettingsElement::id() const {
  uint len = getUInt8(0x0023);
  QString id;
  for (uint i=0; i<len; i++) {
    uint8_t b = getUInt8(0x0025+(i/2));
    if (0 == (i%2))
      id.append(QString::number((b>>4)&0xf, 16));
    else
      id.append(QString::number((b>>0)&0xf, 16));
  }
  return id;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setID(const QString &id) {
  uint len = 0;
  for (int i=0; i<id.length(); i++) {
    bool ok;
    if (0 == (len%2)) {
      setUInt4(0x0025+len/2, 4, id.mid(i, 1).toUInt(&ok, 16));
    } else {
      setUInt4(0x0025+len/2, 0, id.mid(i, 1).toUInt(&ok, 16));
    }
    len++;
  }
  setUInt8(0x0023, len);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::postEncodeDelay() const {
  return ((uint)getUInt8(0x002c))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setPostEncodeDelay(uint ms) {
  setUInt8(0x002c, ms/10);
}

bool
AnytoneCodeplug::FiveToneSettingsElement::hasPTTID() const {
  return 0 != pttID();
}
uint
AnytoneCodeplug::FiveToneSettingsElement::pttID() const {
  return getUInt8(0x002d);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setPTTID(uint id) {
  setUInt8(0x002d, id);
}
void
AnytoneCodeplug::FiveToneSettingsElement::clearPTTID() {
  setPTTID(0);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::autoResetTime() const {
  return ((uint)getUInt8(0x002e))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setAutoResetTime(uint s) {
  setUInt8(0x002e, s/10);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::firstDelay() const {
  return ((uint)getUInt8(0x002f))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setFirstDelay(uint ms) {
  setUInt8(0x002f, ms/10);
}

bool
AnytoneCodeplug::FiveToneSettingsElement::sidetoneEnabled() const {
  return getUInt8(0x0030);
}
void
AnytoneCodeplug::FiveToneSettingsElement::enableSidetone(bool enable) {
  setUInt8(0x0030, (enable ? 0x01 : 0x00));
}

uint
AnytoneCodeplug::FiveToneSettingsElement::stopCode() const {
  return getUInt8(0x0032);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setStopCode(uint code) {
  setUInt8(0x0032, code);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::stopTime() const {
  return ((uint)getUInt8(0x0033))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setStopTime(uint ms) {
  setUInt8(0x0033, ms/10);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::decodeTime() const {
  return ((uint)getUInt8(0x0034))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setDecodeTime(uint ms) {
  setUInt8(0x0034, ms/10);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::delayAfterStop() const {
  return ((uint)getUInt8(0x0035))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setDelayAfterStop(uint ms) {
  setUInt8(0x0035, ms/10);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::preTime() const {
  return ((uint)getUInt8(0x0036))*10;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setPreTime(uint ms) {
  setUInt8(0x0036, ms/10);
}

AnytoneCodeplug::FiveToneSettingsElement::Standard
AnytoneCodeplug::FiveToneSettingsElement::botStandard() const {
  return (Standard) getUInt8(0x0041);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setBOTStandard(Standard standard) {
  setUInt8(0x0041, (uint)standard);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::botToneDuration() const {
  return getUInt8(0x0043);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setBOTToneDuration(uint ms) {
  setUInt8(0x0043, ms);
}

QString
AnytoneCodeplug::FiveToneSettingsElement::botID() const {
  uint len = getUInt8(0x0042);
  QString id;
  for (uint i=0; i<len; i++) {
    uint8_t b = getUInt8(0x0044+(i/2));
    if (0 == (i%2))
      id.append(QString::number((b>>4)&0xf, 16));
    else
      id.append(QString::number((b>>0)&0xf, 16));
  }
  return id;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setBOTID(const QString &id) {
  uint len = 0;
  for (int i=0; i<id.length(); i++) {
    bool ok;
    if (0 == (len%2)) {
      setUInt4(0x0044+len/2, 4, id.mid(i, 1).toUInt(&ok, 16));
    } else {
      setUInt4(0x0044+len/2, 0, id.mid(i, 1).toUInt(&ok, 16));
    }
    len++;
  }
  setUInt8(0x0042, len);
}

AnytoneCodeplug::FiveToneSettingsElement::Standard
AnytoneCodeplug::FiveToneSettingsElement::eotStandard() const {
  return (Standard) getUInt8(0x0061);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setEOTStandard(Standard standard) {
  setUInt8(0x0061, (uint)standard);
}

uint
AnytoneCodeplug::FiveToneSettingsElement::eotToneDuration() const {
  return getUInt8(0x0063);
}
void
AnytoneCodeplug::FiveToneSettingsElement::setEOTToneDuration(uint ms) {
  setUInt8(0x0063, ms);
}

QString
AnytoneCodeplug::FiveToneSettingsElement::eotID() const {
  uint len = getUInt8(0x0062);
  QString id;
  for (uint i=0; i<len; i++) {
    uint8_t b = getUInt8(0x0064+(i/2));
    if (0 == (i%2))
      id.append(QString::number((b>>4)&0xf, 16));
    else
      id.append(QString::number((b>>0)&0xf, 16));
  }
  return id;
}
void
AnytoneCodeplug::FiveToneSettingsElement::setEOTID(const QString &id) {
  uint len = 0;
  for (int i=0; i<id.length(); i++) {
    bool ok;
    if (0 == (len%2)) {
      setUInt4(0x0064+len/2, 4, id.mid(i, 1).toUInt(&ok, 16));
    } else {
      setUInt4(0x0064+len/2, 0, id.mid(i, 1).toUInt(&ok, 16));
    }
    len++;
  }
  setUInt8(0x0062, len);
}










/* ********************************************************************************************* *
 * Implementation of AnytoneCodeplug
 * ********************************************************************************************* */
AnytoneCodeplug::AnytoneCodeplug(QObject *parent)
  : Codeplug(parent)
{
  // pass...
}

AnytoneCodeplug::~AnytoneCodeplug() {
  // pass...
}

bool
AnytoneCodeplug::index(Config *config, Context &ctx) const {
  return true;
}


