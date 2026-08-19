#define BLYNK_TEMPLATE_ID   "TMPL37_Ja6CQd"      // from Blynk console
#define BLYNK_TEMPLATE_NAME "Automatic Gas Alert System"
#define BLYNK_AUTH_TOKEN    "INDqyYvkw4BQPuB31u9xHYmHDyx42QXF"        // from Blynk console

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

// ---------- WiFi credentials ----------
char ssid[] = "Dooms";
char pass[] = "123456789i";
char auth[] = BLYNK_AUTH_TOKEN;

// ---------- Pin definitions ----------
const int gasPin    = 34;   // MQ-2 AOUT -> ADC1 pin (input-only, safe with WiFi)
const int servoPin  = 18;   // SG90 signal
const int buzzerPin = 19;
const int redLED    = 21;
const int greenLED  = 22;

// ---------- Thresholds & timing ----------
const int GAS_THRESHOLD          = 1800;   // 12-bit ADC (0-4095) — CALIBRATE for your unit
const unsigned long ALERT_COOLDOWN = 60000; // 1 min between repeat push notifications
const unsigned long READ_INTERVAL  = 500;   // gas check interval (ms)
const unsigned long WARMUP_TIME    = 20000; // MQ-2 heater warm-up before trusting readings

// ---------- State ----------
Servo doorServo;
BlynkTimer timer;

bool doorOpen        = false;
bool manualOverride  = false;
bool wasAlerted       = false;
bool sensorReady      = false;
unsigned long lastAlertTime = 0;
unsigned long startTime      = 0;

// ---------- Function: open the emergency door ----------
void openDoor() {
  doorServo.write(90);
  doorOpen = true;
}

// ---------- Function: close the emergency door ----------
void closeDoor() {
  doorServo.write(0);
  doorOpen = false;
}

// ---------- Main gas-check routine (called on timer) ----------
void checkGas() {

  // Wait for MQ-2 warm-up before acting on readings
  if (!sensorReady) {
    if (millis() - startTime >= WARMUP_TIME) {
      sensorReady = true;
      Serial.println("MQ-2 warm-up complete. Monitoring started.");
    } else {
      Serial.println("Warming up MQ-2 sensor...");
      return;
    }
  }

  int gasValue = analogRead(gasPin);
  Serial.print("Gas Level: ");
  Serial.println(gasValue);

  // Push live reading to Blynk gauge
  Blynk.virtualWrite(V0, gasValue);

  if (gasValue > GAS_THRESHOLD && !manualOverride) {
    // --- ALARM STATE ---
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
    digitalWrite(buzzerPin, HIGH);
    Blynk.virtualWrite(V1, 255);  // alarm LED widget ON

    if (!doorOpen) {
      openDoor();
    }

    // Send push notification (rate-limited)
    if (!wasAlerted || (millis() - lastAlertTime > ALERT_COOLDOWN)) {
      String msg = "Gas leak detected! Level: " + String(gasValue) +
                   " — Emergency door opened automatically.";
      Blynk.logEvent("gas_alert", msg);
      lastAlertTime = millis();
      wasAlerted = true;
    }

  } else if (!manualOverride) {
    // --- SAFE STATE ---
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    digitalWrite(buzzerPin, LOW);
    Blynk.virtualWrite(V1, 0);

    // Notify once when gas clears (door still stays open for manual reset — safety first)
    if (wasAlerted) {
      Blynk.logEvent("gas_cleared", "Gas levels back to normal. Door remains open pending manual reset.");
      wasAlerted = false;
    }
  }
}

// ---------- Blynk: manual reset button (V2) ----------
BLYNK_WRITE(V2) {
  int pressed = param.asInt();
  if (pressed) {
    closeDoor();
    manualOverride = false;
    digitalWrite(buzzerPin, LOW);
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    Blynk.virtualWrite(V1, 0);
    Serial.println("Manual reset triggered — door closed.");
  }
}

// ---------- Blynk: manual override toggle (V3) - optional, silences auto-trigger ----------
BLYNK_WRITE(V3) {
  manualOverride = param.asInt();
  Serial.print("Manual override: ");
  Serial.println(manualOverride ? "ON" : "OFF");
}

// ---------- Sync app state on connect ----------
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2, V3);
}

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);  // safe by default

  doorServo.attach(servoPin);
  doorServo.write(0);            // door closed at startup

  startTime = millis();

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(READ_INTERVAL, checkGas);

  Serial.println("System initialized. Warming up gas sensor...");
}

void loop() {
  Blynk.run();
  timer.run();
}
