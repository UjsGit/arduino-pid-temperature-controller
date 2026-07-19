#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// -------- PINS --------
#define CLK 2
#define DT  3
#define SW  4
#define FAN_PIN 9
#define ONE_WIRE_BUS 5

LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// -------- MODES --------
enum Mode {MANUAL, LINEAR, PID, AUTO};
Mode mode = MANUAL;
unsigned int auto_Mode = 0;

// -------- STATE --------
bool editMode = false;
int editField = 0;

// -------- VALUES --------
int fanSet = 30;
int minT = 25;
int maxT = 40;
int setT = 30;

// -------- PID --------
float Kp = 8.0, Ki = 0.3, Kd = 2.0;
float integral = 0, lastError = 0;
unsigned long lastPID = 0;

int fanOut = 0;

// -------- TEMP --------
float temp = 0;

// -------- ENCODER --------
volatile int enc = 0;
volatile bool moved = false;
volatile bool lastCLK;
volatile unsigned long lastInt = 0;

// -------- BUTTON --------
unsigned long pressTime = 0;
bool lastBtn = HIGH;
bool longDone = false;

// -------- BLINK --------
unsigned long lastBlink = 0;
bool blink = true;

// -------- SETUP --------
void setup() {
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(SW, INPUT_PULLUP);
  pinMode(FAN_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  sensors.begin();

  lastCLK = digitalRead(CLK);
  attachInterrupt(digitalPinToInterrupt(CLK), isr, CHANGE);
  Serial.begin(9600);
}

// -------- LOOP --------
void loop() {
  readTemp();
  handleButton();
  handleEncoder();
  controlFan();
  updateBlink();
  drawUI();
  sendToPlotter();
}

// -------- TEMP --------
void readTemp() {
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
}

// -------- BUTTON --------
void handleButton() {
  bool cur = digitalRead(SW);

  if (lastBtn == HIGH && cur == LOW) {
    pressTime = millis();
    longDone = false;
  }

  if (cur == LOW && !longDone && millis() - pressTime > 600) {
    if (!editMode) {
      editMode = true;
      editField = 0;
    }
    longDone = true;
  }

  if (lastBtn == LOW && cur == HIGH) {
    if (!longDone) {
      if (!editMode) {
        mode = (Mode)((mode + 1) % 4);
        lcd.clear();
      } else {
        if (mode == LINEAR) {
          editField++;
          if (editField > 1) editMode = false;
        } else {
          editMode = false;
        }
      }
    }
  }

  lastBtn = cur;
}

// -------- ENCODER --------
void handleEncoder() {
  if (!moved) return;

  noInterrupts();
  int val = enc;
  enc = 0;
  moved = false;
  interrupts();

  if (!editMode) return;

  if (mode == MANUAL) {
    fanSet = constrain(fanSet + val, 0, 100);
  }
  else if (mode == LINEAR) {
    if (editField == 0)
      minT = constrain(minT + val, 0, maxT - 1);
    else
      maxT = constrain(maxT + val, minT + 1, 80);
  }
  else if (mode == PID || mode == AUTO) {
    setT = constrain(setT + val, 0, 80);
  }
}

// -------- FAN --------
void controlFan() {
  if (mode == MANUAL) {
    auto_Mode = 0;
    fanOut = fanSet;
  }
  else if (mode == LINEAR) {
    auto_Mode = 0;
    if (temp <= minT) fanOut = 0;
    else if (temp >= maxT) fanOut = 100;
    else fanOut = map(temp, minT, maxT, 20, 100);
  }
  else if (mode == PID) {
    auto_Mode = 0;
    float error = temp - setT;
    float dt = (millis() - lastPID) / 1000.0;
    if (dt <= 0) dt = 0.01;

    integral += error * dt;
    integral = constrain(integral, -50, 50);

    float derivative = (error - lastError) / dt;
    float output = Kp * error + Ki * integral + Kd * derivative;

    if (abs(error) < 0.5) fanOut = 0;
    else if (output > 0) fanOut = constrain(output, 16, 100);
    else fanOut = 0;

    lastError = error;
    lastPID = millis();
  }
  else if (mode == AUTO) {
    float error = temp - setT;

    if (error <= 0.5) {
      fanOut = 0;
      auto_Mode = 1;
    }
    else if (error > 5.0) {
      fanOut = 100;
      auto_Mode = 1;
    }
    else if (error > 2.0) {
      fanOut = map((int)(error * 10), 20, 50, 20, 100);
      fanOut = constrain(fanOut, 20, 100);
      auto_Mode = 2;
    }
    else {
      auto_Mode = 3;
      float dt = (millis() - lastPID) / 1000.0;
      if (dt <= 0) dt = 0.01;

      integral += error * dt;
      integral = constrain(integral, -50, 50);

      float derivative = (error - lastError) / dt;
      float output = Kp * error + Ki * integral + Kd * derivative;

      if (output > 0) fanOut = constrain(output, 16, 100);
      else fanOut = 0;

      lastError = error;
      lastPID = millis();
    }
  }

  analogWrite(FAN_PIN, map(fanOut, 0, 100, 0, 255));
}

// -------- BLINK --------
void updateBlink() {
  if (millis() - lastBlink > 400) {
    blink = !blink;
    lastBlink = millis();
  }
}

// -------- PRINT HELPERS --------
void print3(int val) {
  if (val < 100) lcd.print(" ");
  if (val < 10) lcd.print(" ");
  lcd.print(val);
}

void print2(int val) {
  if (val < 10) lcd.print(" ");
  lcd.print(val);
}

// -------- UI --------
void drawUI() {

  // LINE 1: Temperature on left, mode on right
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print((char)223);
  lcd.print("C    ");   // padding so mode fits cleanly on the right

  lcd.setCursor(10, 0);
  if (mode == MANUAL) lcd.print("MANUAL");
  else if (mode == LINEAR) lcd.print("LINEAR");
  else if (mode == PID) lcd.print("PID");
  else {
    if (auto_Mode == 1) lcd.print("AUTO-M");
    if (auto_Mode == 2) lcd.print("AUTO-L");
    if (auto_Mode == 3) lcd.print("AUTO-P");
  }
  // LINE 2: setting on left, fan speed on right
  lcd.setCursor(0, 1);

  if (mode == MANUAL) {
    lcd.print("Set:");
    if (editMode && blink) {
      lcd.print("   ");
    } else {
      print3(fanSet);
      lcd.print("%");
    }
    lcd.print("  ");
  }
  else if (mode == LINEAR) {
    if (editMode && editField == 0 && blink) lcd.print("  ");
    else print2(minT);

    lcd.print("C-");

    if (editMode && editField == 1 && blink) lcd.print("  ");
    else print2(maxT);

    lcd.print("C  ");
  }
  else if (mode == PID || mode == AUTO) {
    lcd.print("Set:");
    if (editMode && blink) {
      lcd.print("   ");
    } else {
      print3(setT);
      lcd.print("C");
    }
    lcd.print("  ");
  }

  // fan speed on right side of line 2
  lcd.setCursor(10, 1);
  lcd.print("F:");
  if (fanOut < 100) lcd.print(" ");
  if (fanOut < 10) lcd.print(" ");
  lcd.print(fanOut);
  lcd.print("%");
}
void sendToPlotter() {

  static unsigned long lastSend = 0;

  if (millis() - lastSend >= 500) {  // send every 0.5 sec

    Serial.print("Temp:");
    Serial.print(temp);

    Serial.print(", Fan:");
    Serial.print(fanOut);

    Serial.print(", Mode:");
    if(mode == MANUAL) Serial.println("Manual");
    if(mode == LINEAR) Serial.println("Linear");
    if(mode == PID) Serial.println("PID");
    if(mode == AUTO){
      if(auto_Mode == 1) Serial.println("Auto-Manual");
      if(auto_Mode == 2) Serial.println("Auto-Linear");
      if(auto_Mode == 3) Serial.println("Auto-PID");
    }

    lastSend = millis();
  }
}
// -------- ISR --------
void isr() {
  unsigned long t = micros();

  if (t - lastInt > 2000) {
    bool clk = digitalRead(CLK);

    if (clk == HIGH && lastCLK == LOW) {
      if (digitalRead(DT) != clk) enc++;
      else enc--;

      moved = true;
    }

    lastCLK = clk;
    lastInt = t;
  }
}
