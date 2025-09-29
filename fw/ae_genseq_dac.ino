/*  Generative Ambient CV Sequencer — SPI DAC variant (MCP4922 x2)
    Clean 12-bit CV on 4 channels: L1, L2, L3, SUM
    Wiring:
      MCP4922_A -> CH_A(L1), CH_B(L2)
      MCP4922_B -> CH_A(L3), CH_B(SUM)
    VDD=5V, VREF=5V (decouple), LDAC=GND (or GPIO)
*/

#include <Arduino.h>
#include <SPI.h>

// ====== Chip Selects ======
const uint8_t CS_A = 8;   // MCP4922 #1 (L1/L2)
const uint8_t CS_B = 9;   // MCP4922 #2 (L3/SUM)

// ====== Pots / ADC ======
const uint8_t PIN_A_OFFSET   = A2;
const uint8_t PIN_A_L1_DV    = A6;
const uint8_t PIN_A_L2_DV    = A7;
const uint8_t PIN_A_L3_DV    = A8;
const uint8_t PIN_A_L3_THR   = A3;
const uint8_t PIN_A_OFFSET_CV= A0;
const uint8_t PIN_A_L3_CVIN  = A1;

// ====== Gates / switches ======
const uint8_t PIN_G_L1_RISE  = 0;
const uint8_t PIN_G_L2_FALL  = 1;
const uint8_t PIN_SW_L3_ABOVE= 2;
const uint8_t PIN_SW_WRAP_CLIP=14;
const uint8_t PIN_G_RESET_12 = 7;
const uint8_t PIN_G_BOUNCE_23= 15;
const uint8_t PIN_G_RESET_ALL= 16;

// ====== Model ======
struct Lane { int16_t value; int16_t delta; bool bounce; int8_t dir; };
Lane L1{512,0,false,+1}, L2{512,0,false,+1}, L3{512,0,false,+1};
bool wrapMode=false;

// Bounce selection UI (lanes 2 & 3)
enum class BounceState : uint8_t { None=0, L2Only=1, L3Only=2, Both=3 };

// ====== Helpers ======
inline uint16_t clamp10b(int32_t v){ if(v<0)return 0; if(v>1023)return 1023; return (uint16_t)v; }
inline uint16_t wrap10b(int32_t v){ int32_t m=v%1024; if(m<0)m+=1024; return (uint16_t)m; }
inline uint16_t byMode(int32_t v){ return wrapMode? wrap10b(v) : clamp10b(v); }
inline uint16_t ADC10(uint8_t p){ return analogRead(p); }

int16_t potToDelta(uint16_t raw, uint16_t deadband=24, uint16_t maxStep=64){
  int16_t c=(int16_t)raw-512;
  if(abs(c)<(int16_t)deadband) return 0;
  long span=(c>0)? (long)(c-deadband):(long)(c+deadband);
  long maxSpan=512-deadband;
  return (int16_t)((span*maxStep)/maxSpan);
}

// ====== MCP4922 write ======
void mcp4922Write(uint8_t cs, uint8_t ch, uint16_t val12){
  // val12: 0..4095, ch: 0=A, 1=B
  // Config: BUF=1, GA=1 (x1), SHDN=1
  uint16_t cmd = 0;
  if (ch) cmd |= (1 << 15);           // channel
  cmd |= (1 << 14);                    // BUF=1
  cmd |= (1 << 13);                    // GA=1 (x1)
  cmd |= (1 << 12);                    // SHDN=1 (active)
  cmd |= (val12 & 0x0FFF);
  digitalWrite(cs, LOW);
  SPI.transfer16(cmd);
  digitalWrite(cs, HIGH);
}

inline uint16_t up10to12(uint16_t v10){ return (uint16_t)((uint32_t)v10 * 4095UL / 1023UL); }

// ====== Behaviors ======
void applyStep(Lane& L, int8_t sgn=+1){
  int32_t v=L.value + (int32_t)L.delta*sgn;
  if(L.bounce){
    if(v>1023){ L.dir=-1; v = 1023 - (v-1023); }
    if(v<0){    L.dir=+1; v = -v; }
    L.value = clamp10b(v);
  }else{
    L.value = byMode(v);
  }
}

void serviceLane1_Rising(){
  static bool prev=false;
  bool now = (digitalRead(PIN_G_L1_RISE)==LOW);
  if(now && !prev) applyStep(L1, L1.dir);
  prev = now;
}

void serviceLane2_Falling(){
  static bool prev=true;
  bool now = (digitalRead(PIN_G_L2_FALL)==LOW);
  if(!now && prev) applyStep(L2, L2.dir);
  prev = now;
}

void serviceLane3_Threshold(){
  static bool prevTrig=false;
  uint16_t thr=ADC10(PIN_A_L3_THR);
  uint16_t x  =ADC10(PIN_A_L3_CVIN);
  bool above = x>thr;
  bool condAbove = (digitalRead(PIN_SW_L3_ABOVE)==LOW);
  bool trig = condAbove? above : !above;
  if(trig && !prevTrig) applyStep(L3, L3.dir);
  prevTrig = trig;
}

void readUI(){
  wrapMode = (digitalRead(PIN_SW_WRAP_CLIP)==LOW);
  L1.delta = potToDelta(ADC10(PIN_A_L1_DV));
  L2.delta = potToDelta(ADC10(PIN_A_L2_DV));
  L3.delta = potToDelta(ADC10(PIN_A_L3_DV));

  static BounceState bounceState = BounceState::None;
  static bool prevBounceHigh = true;
  static uint32_t pressStartMs = 0;
  const uint32_t HOLD_RESET_MS = 800;

  bool rawLevel = (digitalRead(PIN_G_BOUNCE_23)==HIGH); // HIGH when idle
  uint32_t nowMs = millis();

  if(!rawLevel && prevBounceHigh){
    pressStartMs = nowMs; // button just got pressed
  }

  if(rawLevel && !prevBounceHigh){
    uint32_t heldFor = nowMs - pressStartMs;
    if(heldFor >= HOLD_RESET_MS){
      bounceState = BounceState::None;
    }else{
      switch(bounceState){
        case BounceState::None:   bounceState = BounceState::L2Only; break;
        case BounceState::L2Only: bounceState = BounceState::L3Only; break;
        case BounceState::L3Only: bounceState = BounceState::Both;   break;
        case BounceState::Both:   bounceState = BounceState::None;   break;
      }
    }
  }
  prevBounceHigh = rawLevel;

  L1.bounce = false;
  L2.bounce = (bounceState == BounceState::L2Only || bounceState == BounceState::Both);
  L3.bounce = (bounceState == BounceState::L3Only || bounceState == BounceState::Both);

  if(digitalRead(PIN_G_RESET_12)==LOW){ L1.value=512; L2.value=512; }
  if(digitalRead(PIN_G_RESET_ALL)==LOW){ L1.value=L2.value=L3.value=512; }
}

void writeDACs(){
  uint16_t offPot=ADC10(PIN_A_OFFSET), offCV=ADC10(PIN_A_OFFSET_CV);
  uint16_t offset=(offPot+offCV)>>1;

  uint16_t o1 = byMode((int32_t)L1.value + offset - 512);
  uint16_t o2 = byMode((int32_t)L2.value + offset - 512);
  uint16_t o3 = byMode((int32_t)L3.value + offset - 512);
  uint16_t os = (uint16_t)(((uint32_t)o1 + o2 + o3)/3U);

  uint16_t d1=up10to12(o1), d2=up10to12(o2), d3=up10to12(o3), ds=up10to12(os);

  mcp4922Write(CS_A, 0, d1);  // L1
  mcp4922Write(CS_A, 1, d2);  // L2
  mcp4922Write(CS_B, 0, d3);  // L3
  mcp4922Write(CS_B, 1, ds);  // SUM
}

void setup(){
  pinMode(CS_A, OUTPUT); digitalWrite(CS_A, HIGH);
  pinMode(CS_B, OUTPUT); digitalWrite(CS_B, HIGH);

  pinMode(PIN_G_L1_RISE, INPUT_PULLUP);
  pinMode(PIN_G_L2_FALL, INPUT_PULLUP);
  pinMode(PIN_SW_L3_ABOVE, INPUT_PULLUP);
  pinMode(PIN_SW_WRAP_CLIP, INPUT_PULLUP);
  pinMode(PIN_G_RESET_12, INPUT_PULLUP);
  pinMode(PIN_G_BOUNCE_23, INPUT_PULLUP);
  pinMode(PIN_G_RESET_ALL, INPUT_PULLUP);

  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
}

void loop(){
  readUI();
  serviceLane1_Rising();
  serviceLane2_Falling();
  serviceLane3_Threshold();
  writeDACs();
  delay(1);
}
