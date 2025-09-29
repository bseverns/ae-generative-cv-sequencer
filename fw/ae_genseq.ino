/*  Generative Ambient CV Sequencer (AE Modular) - PWM scaffold
    ---------------------------------------------------------------------------
    This sketch is a deliberately over-commented roadmap for students poking at
    Arduino firmware while wrangling modular synth voltages. Lean in, read the
    commentary, and tweak boldly.

    Hardware: Arduino Pro Micro (ATmega32U4, 5V)
      - This board ships with hardware USB, so D0/D1 double as serial.

    Outputs: D3 (L1), D10 (L2), D5 (L3), D9 (SUM) → RC → buffer
      - Each PWM pin feeds an RC filter and op-amp to generate a CV lane.

    Lanes (aka “tracks”):
      L1: On rising gate → add/subtract ΔV in the direction of travel.
      L2: On falling gate → add/subtract ΔV.
      L3: On CV threshold crossing (>,< selectable) → add/subtract ΔV.

    Modes / tricks:
      - wrap vs clip at the 0–5 V rails
      - bounce (direction flip at the rails)
      - individual lane resets

    NOTE: D0/D1 are USB serial on 32U4; set USE_SAFE_PINS=true to dodge them
    and keep USB consoles happy.
*/

#include <Arduino.h>

// ====== Build-time option ======
// Want to keep the USB port for serial debugging? Flip this to true and we
// reroute the external gate inputs away from the serial pins. Default stays
// false because patch cables are short and we live dangerously.
static const bool USE_SAFE_PINS = false;   // true -> avoid D0/D1 for gates

// ====== PWM outs ======
// These are the four DAC-ish outputs. The RC filters on the hardware smooth
// the 8-bit PWM into a usable voltage. We annotate the board pins for sanity.
const uint8_t PIN_PWM_L1 = 3;    // D3
const uint8_t PIN_PWM_L2 = 10;   // D10
const uint8_t PIN_PWM_L3 = 5;    // D5
const uint8_t PIN_PWM_SUM= 9;    // D9

// ====== Pots / ADC ======
// Every pot is wired into the ADC. Remember: on the 32U4 the analog pin names
// are aliases for actual port/pin combos, hence the eclectic numbering.
const uint8_t PIN_A_OFFSET   = A2;  // global offset pot
const uint8_t PIN_A_L1_DV    = A6;  // lane1 ΔV pot
const uint8_t PIN_A_L2_DV    = A7;  // lane2 ΔV pot
const uint8_t PIN_A_L3_DV    = A8;  // lane3 ΔV pot
const uint8_t PIN_A_L3_THR   = A3;  // lane3 threshold pot

// ====== CV inputs ======
// External control voltages: one for the global offset, one for lane three’s
// comparator input. These read 0–5 V just like the pots.
const uint8_t PIN_A_OFFSET_CV= A0;  // global offset CV in (0–5V)
const uint8_t PIN_A_L3_CVIN  = A1;  // lane3 comparator CV in (0–5V)

// ====== Gate / switch inputs ======
// Gates are wired with pull-ups, so “LOW” means “the user poked it”. Switches
// follow the same convention to keep hardware wiring brain-dead simple.
uint8_t PIN_G_L1_RISE;              // rising gate source for L1
uint8_t PIN_G_L2_FALL;              // falling gate source for L2
const uint8_t PIN_SW_L3_ABOVE = 2;  // compare above (1) / below (0)
const uint8_t PIN_SW_WRAP_CLIP= 14; // wrap=1 / clip=0
const uint8_t PIN_G_RESET_12  = 7;  // gate: reset lanes 1&2
const uint8_t PIN_G_BOUNCE_23 = 15; // gate: toggle bounce for lanes 2&3
const uint8_t PIN_G_RESET_ALL = 16; // gate: reset all

// ====== Model state ======
// Each lane carries its current value (0–1023 ≈ 0–5 V), a step size, a bounce
// flag, and a direction (+1 or -1). We seed them at mid-rail pointing upwards.
struct Lane { int16_t value; int16_t delta; bool bounce; int8_t dir; };
Lane L1{512,0,false,+1}, L2{512,0,false,+1}, L3{512,0,false,+1};
bool wrapMode=false; // false → clamp at rails, true → wrap-around mayhem.

// Bounce selection UI (lanes 2 & 3)
// Think of this like a four-stop rotary switch we emulate with a single push
// button. The enum keeps the states obvious.
enum class BounceState : uint8_t { None=0, L2Only=1, L3Only=2, Both=3 };

// Edge tracking
// We track the previous sampled level for each gate so we can detect edges.
struct Edge { bool prev; }; Edge eL1, eL2;

// ====== Helpers ======
inline uint16_t clamp10b(int32_t v){ if(v<0)return 0; if(v>1023)return 1023; return (uint16_t)v; }
inline uint16_t wrap10b(int32_t v){ int32_t m=v%1024; if(m<0)m+=1024; return (uint16_t)m; }
inline uint16_t byMode(int32_t v){ return wrapMode ? wrap10b(v) : clamp10b(v); }
inline uint16_t ADC10(uint8_t p){ return analogRead(p); }

// Convert a raw 10-bit pot reading into a signed delta step.
// The deadband keeps tiny noise jitters from nudging the lane.
int16_t potToDelta(uint16_t raw, uint16_t deadband=24, uint16_t maxStep=64){
  int16_t c=(int16_t)raw-512;           // center the knob around zero
  if(abs(c)<(int16_t)deadband) return 0; // chill if we are inside the deadband
  long span=(c>0)? (long)(c-deadband):(long)(c+deadband); // trim the deadband
  long maxSpan=512-deadband;            // how far we can travel post-deadband
  return (int16_t)((span*maxStep)/maxSpan); // scale to the requested max step
}

// Helper that normalizes Arduino's HIGH/LOW into tidy 1/0 values.
uint8_t bRead(uint8_t pin){ return (digitalRead(pin)==HIGH)?1:0; }

// Feed a 10-bit value to an 8-bit PWM pin. Hardware RC does the smoothing.
void writePWM(uint8_t pin, uint16_t v10){
  uint8_t v8 = (uint8_t)(v10 >> 2); // cheap downscale by dropping the LSBs
  analogWrite(pin, v8);
}

// ====== Setup ======
void setupPins(){
  // Optionally reroute the two external gates off the serial pins.
  PIN_G_L1_RISE = USE_SAFE_PINS ? 4  : 0;  // alt: D4 instead of D0
  PIN_G_L2_FALL = USE_SAFE_PINS ? 6  : 1;  // alt: D6 instead of D1

  // Outputs first so nothing floats while we configure inputs.
  pinMode(PIN_PWM_L1, OUTPUT);
  pinMode(PIN_PWM_L2, OUTPUT);
  pinMode(PIN_PWM_L3, OUTPUT);
  pinMode(PIN_PWM_SUM,OUTPUT);

  // All inputs use the internal pull-up for simple active-low wiring.
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

  // Initialize edge trackers with the *active* state so the first service
  // pass does not accidentally fire a phantom step. Because the gates are
  // wired active-low we invert the readback here.
  eL1.prev = !bRead(PIN_G_L1_RISE);
  eL2.prev = !bRead(PIN_G_L2_FALL);
}

// ====== Core behaviors ======
// Step a lane forward or backward according to its delta and current direction.
// When bounce is enabled we reflect off the rails and flip direction.
void applyStep(Lane& L, int8_t sgn=+1){
  int32_t v = L.value + (int32_t)L.delta * sgn; // accumulate with full precision
  if (L.bounce){
    if (v>1023){ L.dir=-1; v = 1023 - (v-1023); } // reflect off the top rail
    if (v<0)   { L.dir=+1; v = -v; }              // reflect off the bottom rail
    L.value = clamp10b(v);
  } else {
    L.value = byMode(v); // wrap or clip depending on the global mode
  }
}

void serviceLane1_Rising(){
  bool now = !bRead(PIN_G_L1_RISE); // active low: LOW means gate present
  if (now && !eL1.prev) applyStep(L1, L1.dir); // fire on rising edge
  eL1.prev = now;
}

void serviceLane2_Falling(){
  bool now = !bRead(PIN_G_L2_FALL); // still active-low
  if (!now && eL2.prev) applyStep(L2, L2.dir); // fire on falling edge
  eL2.prev = now;
}

void serviceLane3_Threshold(){
  uint16_t thr  = ADC10(PIN_A_L3_THR);  // student tweak: move the threshold
  uint16_t cvin = ADC10(PIN_A_L3_CVIN); // external CV wailing into the compare
  bool above = cvin > thr;
  bool condAbove = !bRead(PIN_SW_L3_ABOVE); // active low toggle: above vs below
  static bool prevTrig=false;

  bool trig = condAbove ? above : !above; // pick the comparison direction
  if (trig && !prevTrig) applyStep(L3, L3.dir); // only on the rising edge
  prevTrig = trig;
}

// ====== UI / Modes / Resets ======
void readUI(){
  wrapMode = !bRead(PIN_SW_WRAP_CLIP); // switch is active-low: LOW = wrap mode

  // Convert pot positions into usable ΔV step sizes for each lane.
  L1.delta = potToDelta(ADC10(PIN_A_L1_DV));
  L2.delta = potToDelta(ADC10(PIN_A_L2_DV));
  L3.delta = potToDelta(ADC10(PIN_A_L3_DV));

  // ---- Bounce mode state machine ---------------------------------------
  static BounceState bounceState = BounceState::None;
  static bool prevBounceHigh = bRead(PIN_G_BOUNCE_23); // init to the real level
  static uint32_t pressStartMs = 0;
  static bool holdClearedState = false; // latched when a long hold already reset
  const uint32_t HOLD_RESET_MS = 800; // press & hold to reset to “None”

  bool rawLevel = bRead(PIN_G_BOUNCE_23); // HIGH (1) idle, LOW (0) pressed
  uint32_t nowMs = millis();

  if (!rawLevel && prevBounceHigh){
    pressStartMs = nowMs;     // falling edge: button just got pressed
    holdClearedState = false; // a new press arms both the cycle and hold paths
  }

  // While the button is held we watch for the “dramatic hold” to fire. Once
  // the timer trips we immediately slam the mode back to "None" and latch
  // that fact so the eventual release does not also cycle to the next state.
  if (!rawLevel && !prevBounceHigh && !holdClearedState){
    if (pressStartMs == 0){ pressStartMs = nowMs; }
    if ((nowMs - pressStartMs) >= HOLD_RESET_MS){
      bounceState = BounceState::None;
      holdClearedState = true;
    }
  }

  if (rawLevel && !prevBounceHigh){
    // rising edge: button released. Only cycle if we did not already nuke
    // the state via the long-hold logic above.
    if (!holdClearedState){
      switch (bounceState){
        case BounceState::None:   bounceState = BounceState::L2Only; break;
        case BounceState::L2Only: bounceState = BounceState::L3Only; break;
        case BounceState::L3Only: bounceState = BounceState::Both;   break;
        case BounceState::Both:   bounceState = BounceState::None;   break;
      }
    }
  }
  prevBounceHigh = rawLevel;

  // Map the enum choice back into per-lane booleans.
  L1.bounce = false;
  L2.bounce = (bounceState == BounceState::L2Only || bounceState == BounceState::Both);
  L3.bounce = (bounceState == BounceState::L3Only || bounceState == BounceState::Both);

  // Resets remain old-school: hold the button low and the lane snaps home.
  if (!bRead(PIN_G_RESET_12)){ L1.value=512; L2.value=512; }
  if (!bRead(PIN_G_RESET_ALL)){ L1.value=L2.value=L3.value=512; }
}

void outputCVs(){
  // Mix the offset pot and external offset CV. Averaging keeps things sane.
  uint16_t offPot = ADC10(PIN_A_OFFSET);
  uint16_t offCV  = ADC10(PIN_A_OFFSET_CV);
  uint16_t offset = (offPot + offCV) >> 1;

  // Apply the offset, then push through wrap/clip logic so rails behave.
  uint16_t o1 = byMode((int32_t)L1.value + offset - 512);
  uint16_t o2 = byMode((int32_t)L2.value + offset - 512);
  uint16_t o3 = byMode((int32_t)L3.value + offset - 512);
  uint16_t os = (uint16_t)(((uint32_t)o1 + o2 + o3)/3U); // lazy average for SUM

  writePWM(PIN_PWM_L1, o1);
  writePWM(PIN_PWM_L2, o2);
  writePWM(PIN_PWM_L3, o3);
  writePWM(PIN_PWM_SUM,os);
}

void loop(){
  // Order matters: read UI first (so deltas are current), then service each
  // lane, then spit the results out the PWM pins. The 1 ms delay chills the
  // CPU a smidge without tanking responsiveness.
  readUI();
  serviceLane1_Rising();
  serviceLane2_Falling();
  serviceLane3_Threshold();
  outputCVs();
  delay(1);
}
