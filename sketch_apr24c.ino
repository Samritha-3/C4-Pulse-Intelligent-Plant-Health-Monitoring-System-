// ================================================================
//  CropGuard v1 — Proteus Simulation Sketch
//
//  SIMULATION INPUTS:
//    BME280  (U1) I2C 0x76 → ambient temperature (Ta)
//    POT A1  (RV1)          → humidity RH (0-100%)
//    POT A2  (RV2)          → leaf temperature Tl (20-50°C range)
//    PCF8574 (U3) I2C 0x20 → LED driver
//    LCD LM016L  direct     → display
//    BUTTON  D8             → calibration trigger
//
//  NOTE: TC74 replaced by POT on A2 for simulation reliability.
//  Real device: Tl = MLX90614 over I2C (non-contact IR sensor)
//
//  LIBRARIES: Adafruit BME280, Adafruit Unified Sensor,
//             LiquidCrystal (built-in), Wire (built-in)
// ================================================================

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal.h>
#include <math.h>

// ---- I2C Addresses ----
#define BME_ADDR   0x76
#define PCF_ADDR   0x20

// ---- LCD: RS=7, EN=6, D4=5, D5=4, D6=3, D7=2 ----
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
Adafruit_BME280 bme;

// ---- Analog pins ----
#define POT_RH   A1   // RV1 — simulates Relative Humidity (0-100%)
#define POT_TL   A0   // RV2 — simulates Leaf Temperature (20-50°C)
#define TL_MIN   20.0 // POT fully left  = 20°C
#define TL_MAX   50.0 // POT fully right = 50°C

// ---- Button ----
#define BTN_PIN  8

// ---- PCF8574 LEDs (active LOW: LOW = ON) ----
#define LED_GREEN  0  // P0
#define LED_YELLOW 1  // P1
#define LED_RED    2  // P2
uint8_t pcfState = 0xFF; // all off

// ---- NWSB baseline ----
float nwsb_a =  1.5;
float nwsb_b = -2.0;
#define NTB_OFFSET 5.0

// ---- Calibration ----
#define TOTAL_SAMPLES   6
#define SAMPLE_INTERVAL 20000UL  // 20 sec per sample in sim
int   samplesCount = 0;
float sum_LAD=0, sum_VPD=0, sum_VPD2=0, sum_LAD_VPD=0;
unsigned long lastSampleTime = 0;

// ---- Streak counter ----
#define STREAK_NEEDED 3
int streak=0, lastDiag=-1;

// ---- States ----
#define ST_WAIT  0
#define ST_CALIB 1
#define ST_LIVE  2
int state = ST_WAIT;

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(BTN_PIN, INPUT_PULLUP);
  lcd.begin(16, 2);
  pcfWrite(0xFF); // all LEDs off

  // BME280 check
  if (!bme.begin(BME_ADDR)) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("BME280 ERROR!   ");
    lcd.setCursor(0,1); lcd.print("Check wiring    ");
    Serial.println("FATAL: BME280 not found at 0x76");
    while(1);
  }
  Serial.println("BME280 OK");

  // Boot screen
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("  CropGuard v1  ");
  lcd.setCursor(0,1); lcd.print("Btn: Calibrate  ");
  delay(2000);

  Serial.println("Ready. Press calibration button.");
}

// ================================================================
//  MAIN LOOP
// ================================================================
void loop() {
  // Button check — works in any state
  if (btnPressed()) {
    startCalib();
    return;
  }

  switch(state) {
    case ST_WAIT:
      break; // just waiting for button

    case ST_CALIB:
      runCalib();
      break;

    case ST_LIVE:
      runLive();
      delay(5000); // 5 sec cycle
      break;
  }
}

// ================================================================
//  READ SENSORS
// ================================================================

// Leaf temperature from POT on A2
// Fully left = 20°C, fully right = 50°C
float readLeafTemp() {
  int raw = analogRead(POT_TL);
  float tl = TL_MIN + (raw / 1023.0) * (TL_MAX - TL_MIN);
  return tl;
}

// Humidity from POT on A1
float readHumidity() {
  int raw = analogRead(POT_RH);
  return (raw / 1023.0) * 100.0;
}

// VPD — Magnus formula
float calcVPD(float Ta, float RH) {
  float Vsat = 0.6108 * exp((17.27 * Ta) / (Ta + 237.3));
  float Vact = Vsat * (RH / 100.0);
  return max(Vsat - Vact, 0.01f);
}

// ================================================================
//  CALIBRATION
// ================================================================
void startCalib() {
  samplesCount = 0;
  sum_LAD = sum_VPD = sum_VPD2 = sum_LAD_VPD = 0;
  streak = 0; lastDiag = -1;
  // First sample taken immediately
  lastSampleTime = millis() - SAMPLE_INTERVAL;

  pcfWrite(0xFF);
  setLED(LED_YELLOW, true); // YELLOW on entire calibration

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Calibrating...  ");
  lcd.setCursor(0,1); lcd.print("Samples: 0/6    ");

  state = ST_CALIB;
  Serial.println("=== CALIBRATION STARTED ===");
  Serial.println("Keep both POTs at healthy position (RH~60%, Tl~Ta-2)");
}

void runCalib() {
  if (millis() - lastSampleTime < SAMPLE_INTERVAL) return;
  lastSampleTime = millis();

  float Ta  = bme.readTemperature();
  float RH  = readHumidity();
  float Tl  = readLeafTemp();
  float VPD = calcVPD(Ta, RH);
  float LAD = Tl - Ta;

  samplesCount++;
  sum_LAD     += LAD;
  sum_VPD     += VPD;
  sum_VPD2    += VPD * VPD;
  sum_LAD_VPD += LAD * VPD;

  // Show running NWSB estimate
  float curNWSB = nwsb_a + nwsb_b * VPD;

  Serial.print("Sample "); Serial.print(samplesCount);
  Serial.print("/6 | Ta="); Serial.print(Ta,1);
  Serial.print(" Tl="); Serial.print(Tl,1);
  Serial.print(" RH="); Serial.print(RH,0);
  Serial.print(" VPD="); Serial.print(VPD,2);
  Serial.print(" LAD="); Serial.print(LAD,2);
  Serial.print(" NWSB="); Serial.println(curNWSB,2);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Samples:"); lcd.print(samplesCount); lcd.print("/6  ");
  lcd.setCursor(0,1);
  lcd.print("NWSB="); lcd.print(curNWSB,2); lcd.print(" C    ");

  if (samplesCount >= TOTAL_SAMPLES) finishCalib();
}

void finishCalib() {
  float n     = (float)samplesCount;
  float denom = n * sum_VPD2 - sum_VPD * sum_VPD;
  float new_a = nwsb_a, new_b = nwsb_b;

  if (abs(denom) > 0.0001) {
    new_b = (n * sum_LAD_VPD - sum_VPD * sum_LAD) / denom;
    new_a = (sum_LAD - new_b * sum_VPD) / n;
  }

  Serial.print("Regression: a="); Serial.print(new_a,3);
  Serial.print(" b="); Serial.println(new_b,3);

  bool valid = isfinite(new_a) && isfinite(new_b)
            && new_a > -10.0 && new_a < 15.0
            && new_b >  -8.0 && new_b <  1.0;

  pcfWrite(0xFF);

  if (valid) {
    nwsb_a = new_a; nwsb_b = new_b;
    Serial.println("Calibration ACCEPTED");
    flashLED(LED_GREEN, 3);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Calib complete! ");
    lcd.setCursor(0,1);
    lcd.print("a="); lcd.print(nwsb_a,1);
    lcd.print(" b="); lcd.print(nwsb_b,1);
  } else {
    Serial.println("Calibration rejected — using defaults");
    flashLED(LED_RED, 3);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Using defaults  ");
    lcd.setCursor(0,1); lcd.print("a=1.5  b=-2.0   ");
  }

  delay(3000);
  lcd.clear();
  state = ST_LIVE;
  Serial.println("=== LIVE SENSING STARTED ===");
}

// ================================================================
//  LIVE SENSING
// ================================================================
void runLive() {
  float Ta  = bme.readTemperature();
  float RH  = readHumidity();
  float Tl  = readLeafTemp();
  float VPD = calcVPD(Ta, RH);
  float LAD = Tl - Ta;
  float NWSB = nwsb_a + nwsb_b * VPD;
  float NTB  = NWSB + NTB_OFFSET;
  float range = NTB - NWSB;
  float CWSI = (abs(range) < 0.01) ? 0.5 : (LAD - NWSB) / range;

  // NaN guard
  if (!isfinite(CWSI)) CWSI = 0.1;

  Serial.print("LIVE | Ta="); Serial.print(Ta,1);
  Serial.print(" Tl="); Serial.print(Tl,1);
  Serial.print(" RH="); Serial.print(RH,0);
  Serial.print(" VPD="); Serial.print(VPD,2);
  Serial.print(" LAD="); Serial.print(LAD,2);
  Serial.print(" CWSI="); Serial.println(CWSI,2);

  // Diagnosis
  int diag;
  if      (CWSI <= 0.2) diag = 0;
  else if (CWSI <= 0.5) diag = 1;
  else if (CWSI <= 0.8) diag = 2;
  else if (CWSI <= 1.1) diag = 3;
  else                  diag = 4;

  // Streak
  if (diag == lastDiag) { if (streak < STREAK_NEEDED) streak++; }
  else                  { streak = 1; lastDiag = diag; }
  bool confirmed = (streak >= STREAK_NEEDED);

  pcfWrite(0xFF);
  lcd.clear();

  switch(diag) {
    case 0:
      setLED(LED_GREEN, true);
      lcd.setCursor(0,0); lcd.print("CWSI:"); lcd.print(CWSI,2); lcd.print("  OK  ");
      lcd.setCursor(0,1); lcd.print("Plant thriving! ");
      break;
    case 1:
      setLED(LED_YELLOW, true);
      lcd.setCursor(0,0); lcd.print("CWSI:"); lcd.print(CWSI,2); lcd.print(" MILD ");
      lcd.setCursor(0,1); lcd.print(confirmed?"Water soon!     ":"Initial H2OStres");
      break;
    case 2:
      setLED(LED_RED, true);
      lcd.setCursor(0,0); lcd.print("CWSI:"); lcd.print(CWSI,2); lcd.print(" SICK ");
      lcd.setCursor(0,1); lcd.print(confirmed?"Bio Fever Det!  ":"Monitoring...   ");
      break;
    case 3:
      if ((millis()/400)%2==0) setLED(LED_RED,true);
      lcd.setCursor(0,0); lcd.print("CWSI:"); lcd.print(CWSI,2); lcd.print(" CRIT!");
      lcd.setCursor(0,1); lcd.print(confirmed?"Wilting Point!! ":"Critical...     ");
      break;
    case 4:
      pcfWrite(0xFF);
      lcd.setCursor(0,0); lcd.print("SENSOR ERROR    ");
      lcd.setCursor(0,1); lcd.print("Check Sensor!   ");
      break;
  }
}

// ================================================================
//  PCF8574 HELPERS
// ================================================================
void pcfWrite(uint8_t val) {
  Wire.beginTransmission(PCF_ADDR);
  Wire.write(val);
  Wire.endTransmission();
  pcfState = val;
}
void setLED(uint8_t pin, bool on) {
  if (on) pcfState &= ~(1<<pin);
  else    pcfState |=  (1<<pin);
  pcfWrite(pcfState);
}
void flashLED(uint8_t pin, int n) {
  for(int i=0;i<n;i++){
    setLED(pin,true);  delay(250);
    setLED(pin,false); delay(250);
  }
}

// ================================================================
//  BUTTON
// ================================================================
bool btnPressed() {
  if (digitalRead(BTN_PIN)==LOW) {
    delay(80);
    if (digitalRead(BTN_PIN)==LOW) {
      unsigned long t=millis();
      while(digitalRead(BTN_PIN)==LOW){if(millis()-t>2000)break;}
      return true;
    }
  }
  return false;
}
