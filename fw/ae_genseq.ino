/*  Generative Ambient CV Sequencer (AE Modular) - PWM scaffold
    Hardware: Arduino Pro Micro (ATmega32U4, 5V)
    Outputs: D3 (L1), D10 (L2), D5 (L3), D9 (SUM) → RC → buffer
    Lanes:
      L1: on rising gate -> add/sub ΔV
      L2: on falling gate -> add/sub ΔV
      L3: on CV threshold crossing (>,< selectable) -> add/sub ΔV
    Modes:
      clip vs wrap; bounce (invert direction at 0/5V); per-lane resets
    NOTE: D0/D1 are USB serial on 32U4; set USE_SAFE_PINS=true to avoid them.
*/

#include <Arduino.h>

// ====== Build-time option ======
static const bool USE_SAFE_PINS = false;   // true -> avoid D0/D1 for gates

// ====== PWM outs ======
const uint8_t PIN_PWM_L1 = 3;    // D3
const uint8_t PIN_PWM_L2 = 10;   // D10
const uint8_t PIN_PWM_L3 = 5;    // D5
const uint8_t PIN_PWM_SUM= 9;    // D9

// ====== Pots / ADC ======
const uint8_t PIN_A_OFFSET   = A2;  // global offset pot
const uint8_t PIN_A_L1_DV    = A6;  // lane1 ΔV pot
const uint8_t PIN_A_L2_DV    = A7;  // lane2 ΔV pot
const uint8_t PIN_A_L3_DV    = A8;  // lane3 ΔV pot
const uint8_t PIN_A_L3_THR   = A3;  // lane3 threshold pot

// ====== CV inputs ======
const uint8_t PIN_A_OFFSET_CV= A0;  // global offset CV in (0–5V)
const uint8_t PIN_A_L3_CVIN  = A1;  // lane3 comparator CV in (0–5V)

// ====== Gate / switch inputs ======
uint8_t PIN_G_L1_RISE;              // rising gate source for L1
uint8_t PIN_G_L2_FALL;              // falling gate source for L2
const uint8_t PIN_SW_L3_ABOVE = 2;  // compare above (1) / below (0)
const uint8_t PIN_SW_WRAP_CLIP= 14; // wrap=1 / clip=0
const uint8_t PIN_G_RESET_12  = 7;  // gate: reset lanes 1&2
const uint8_t PIN_G_BOUNCE_23 = 15; // gate: toggle bounce for lanes 2&3
const uint8_t PIN_G_RESET_ALL = 16; // gate: reset all

// ====== Model state ======
struct Lane { int16_t value; int16_t delta; bool bounce; int8_t dir; };
Lane L1{512,0,false,+1}, L2{512,0,false,+1}, L3{512,0,false,+1};
bool wrapMode=false;

// Edge tracking
struct Edge { bool prev; }; Edge eL1, eL2;

// ====== Helpers ======
inline uint16_t clamp10b(int32_t v){ if(v<0)return 0; if(v>1023)return 1023; return (uint16_t)v; }
inline uint16_t wrap10b(int32_t v){ int32_t m=v%1024; if(m<0)m+=1024; return (uint16_t)m; }
inline uint16_t byMode(int32_t v){ return wrapMode ? wrap10b(v) : clamp10b(v); }
inline uint16_t ADC10(uint8_t p){ return analogRead(p); }

int16_t potToDelta(uint16_t raw, uint16_t deadband=24, uint16_t maxStep=64){
  int16_t c=(int16_t)raw-512;
  if(abs(c)<(int16_t)deadband) return 0;
  long span=(c>0)? (long)(c-deadband):(long)(c+deadband);
  long maxSpan=512-deadband;
  return (int16_t)((span*maxStep)/maxSpan);
}

uint8_t bRead(uint8_t pin){ return (digitalRead(pin)==HIGH)?1:0; }

void writePWM(uint8_t pin, uint16_t v10){
  uint8_t v8 = (uint8_t)(v10 >> 2);
  analogWrite(pin, v8);
}

// ====== Setup ======
void setupPins(){
  PIN_G_L1_RISE = USE_SAFE_PINS ? 4  : 0;  // alt: D4 instead of D0
  PIN_G_L2_FALL = USE_SAFE_PINS ? 6  : 1;  // alt: D6 instead of D1

  pinMode(PIN_PWM_L1, OUTPUT);
  pinMode(PIN_PWM_L2, OUTPUT);
  pinMode(PIN_PWM_L3, OUTPUT);
  pinMode(PIN_PWM_SUM,OUTPUT);

  pinMode(PIN_G_L1_RISE, INPUT_PULLUP);
  pinMode(PIN_G_L2_FALL, INPUT_PULLUP);
  pinMode(PIN_SW_L3_ABOVE, INPUT_PULLUP);
  pinMode(PIN_SW_WRAP_CLIP, INPUT_PULLUP);
  pinMode(PIN_G_RESET_12, INPUT_PULLUP);
  pinMode(PIN_G_BOUNCE_23, INPUT_PULLUP);
  pinMode(PIN_G_RESET_ALL, INPUT_PULLUP);
}

void setup(){
  setupPins();
  if (USE_SAFE_PINS) { Serial.begin(115200); } // optional debug

  eL1.prev = digitalRead(PIN_G_L1_RISE);
  eL2.prev = digitalRead(PIN_G_L2_FALL);
}

// ====== Core behaviors ======
void applyStep(Lane& L, int8_t sgn=+1){
  int32_t v = L.value + (int32_t)L.delta * sgn;
  if (L.bounce){
    if (v>1023){ L.dir=-1; v = 1023 - (v-1023); }
    if (v<0)   { L.dir=+1; v = -v; }
    L.value = clamp10b(v);
  } else {
    L.value = byMode(v);
  }
}

void serviceLane1_Rising(){
  bool now = !bRead(PIN_G_L1_RISE);
  if (now && !eL1.prev) applyStep(L1, L1.dir);
  eL1.prev = now;
}

void serviceLane2_Falling(){
  bool now = !bRead(PIN_G_L2_FALL);
  if (!now && eL2.prev) applyStep(L2, L2.dir);
  eL2.prev = now;
}

void serviceLane3_Threshold(){
  uint16_t thr  = ADC10(PIN_A_L3_THR);
  uint16_t cvin = ADC10(PIN_A_L3_CVIN);
  bool above = cvin > thr;
  bool condAbove = !bRead(PIN_SW_L3_ABOVE); // active low
  static bool prevTrig=false;

  bool trig = condAbove ? above : !above;
  if (trig && !prevTrig) applyStep(L3, L3.dir);
  prevTrig = trig;
}

// ====== UI / Modes / Resets ======
void readUI(){
  wrapMode = !bRead(PIN_SW_WRAP_CLIP);

  L1.delta = potToDelta(ADC10(PIN_A_L1_DV));
  L2.delta = potToDelta(ADC10(PIN_A_L2_DV));
  L3.delta = potToDelta(ADC10(PIN_A_L3_DV));

  static bool prevB=false;
  bool bNow = !bRead(PIN_G_BOUNCE_23);
  if (bNow && !prevB){ L2.bounce=!L2.bounce; L3.bounce=!L3.bounce; }
  prevB=bNow;

  if (!bRead(PIN_G_RESET_12)){ L1.value=512; L2.value=512; }
  if (!bRead(PIN_G_RESET_ALL)){ L1.value=L2.value=L3.value=512; }
}

void outputCVs(){
  uint16_t offPot = ADC10(PIN_A_OFFSET);
  uint16_t offCV  = ADC10(PIN_A_OFFSET_CV);
  uint16_t offset = (offPot + offCV) >> 1;

  uint16_t o1 = byMode((int32_t)L1.value + offset - 512);
  uint16_t o2 = byMode((int32_t)L2.value + offset - 512);
  uint16_t o3 = byMode((int32_t)L3.value + offset - 512);
  uint16_t os = (uint16_t)(((uint32_t)o1 + o2 + o3)/3U);

  writePWM(PIN_PWM_L1, o1);
  writePWM(PIN_PWM_L2, o2);
  writePWM(PIN_PWM_L3, o3);
  writePWM(PIN_PWM_SUM,os);
}

void loop(){
  readUI();
  serviceLane1_Rising();
  serviceLane2_Falling();
  serviceLane3_Threshold();
  outputCVs();
  delay(1);
}
