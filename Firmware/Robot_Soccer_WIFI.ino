
/*
   ROBOT SOCCER
   ESP32-C3 SuperMini
   RemoteXY + MX1508
   Dual Joystick

   Joystick kiri Y  : Throttle
   Joystick kanan X : Steering

   WiFi              : Robot02
   Password WiFi     : 124124124
   Password RemoteXY: 124124124
   Port              : 6377

   Diagnostic        : Serial Monitor 115200

   UPDATE:
   - Startup motor protection
   - Joystick neutral calibration
   - Communication failsafe
   - PWM ramp control
   - Motor diagnostic
*/

#define REMOTEXY_MODE__WIFI_POINT

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>

#define REMOTEXY_WIFI_SSID "Robot02"
#define REMOTEXY_WIFI_PASSWORD "124124124"
#define REMOTEXY_SERVER_PORT 6377
#define REMOTEXY_ACCESS_PASSWORD "124124124"

#define REMOTEXY__DEBUGLOG
#define REMOTEXY__DEBUGLOG_SERIAL Serial
#define REMOTEXY__DEBUGLOG_SPEED 115200

#include <RemoteXY.h>

// ======================================
// GUI REMOTEXY - TIDAK DIUBAH
// ======================================

#pragma pack(push, 1)

uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] = {
  255,4,0,0,0,48,0,19,0,0,0,82,111,98,111,116,48,50,0,8,
  2,106,200,200,84,1,1,2,0,5,207,29,143,143,26,17,40,40,32,2,
  26,31,5,6,24,143,143,128,17,39,39,32,2,26,31
};

struct {
  int8_t joystick_01_x;
  int8_t joystick_01_y;
  int8_t joystick_02_x;
  int8_t joystick_02_y;
  uint8_t connect_flag;
} RemoteXY;

#pragma pack(pop)

// ======================================
// PIN MOTOR MX1508
// ======================================

#define L_IN1 10
#define L_IN2 0
#define R_IN1 1
#define R_IN2 3

// ======================================
// PENGATURAN KENDALI
// ======================================

#define DEADZONE 10

#define MAX_PWM 255

// Timeout komunikasi
#define FAILSAFE_TIMEOUT 500

// Kecepatan perubahan PWM
#define RAMP_STEP 8

const bool INVERT_THROTTLE = false;
const bool INVERT_STEERING = false;

// ======================================
// VARIABEL SISTEM
// ======================================

int currentLeftPWM = 0;
int currentRightPWM = 0;

int targetLeftPWM = 0;
int targetRightPWM = 0;

unsigned long lastValidCommand = 0;
unsigned long lastPrint = 0;

bool controlEnabled = false;

// ======================================
// MOTOR KIRI
// ======================================

void motorLeft(int speed) {

  speed = constrain(speed, -MAX_PWM, MAX_PWM);

  if (speed > 0) {
    analogWrite(L_IN1, speed);
    analogWrite(L_IN2, 0);
  }
  else if (speed < 0) {
    analogWrite(L_IN1, 0);
    analogWrite(L_IN2, -speed);
  }
  else {
    analogWrite(L_IN1, 0);
    analogWrite(L_IN2, 0);
  }
}

// ======================================
// MOTOR KANAN
// ======================================

void motorRight(int speed) {

  speed = constrain(speed, -MAX_PWM, MAX_PWM);

  if (speed > 0) {
    analogWrite(R_IN1, speed);
    analogWrite(R_IN2, 0);
  }
  else if (speed < 0) {
    analogWrite(R_IN1, 0);
    analogWrite(R_IN2, -speed);
  }
  else {
    analogWrite(R_IN1, 0);
    analogWrite(R_IN2, 0);
  }
}

// ======================================
// STOP MOTOR LANGSUNG
// ======================================

void stopMotorImmediate() {

  currentLeftPWM = 0;
  currentRightPWM = 0;

  targetLeftPWM = 0;
  targetRightPWM = 0;

  analogWrite(L_IN1, 0);
  analogWrite(L_IN2, 0);

  analogWrite(R_IN1, 0);
  analogWrite(R_IN2, 0);
}

// ======================================
// DEADZONE
// ======================================

int applyDeadzone(int value) {

  if (abs(value) <= DEADZONE) {
    return 0;
  }

  return value;
}

// ======================================
// RAMP CONTROL
// ======================================

int rampValue(int current, int target) {

  if (current < target) {
    current += RAMP_STEP;

    if (current > target) {
      current = target;
    }
  }

  else if (current > target) {
    current -= RAMP_STEP;

    if (current < target) {
      current = target;
    }
  }

  return current;
}

void updateMotorRamp() {

  currentLeftPWM =
      rampValue(currentLeftPWM, targetLeftPWM);

  currentRightPWM =
      rampValue(currentRightPWM, targetRightPWM);

  motorLeft(currentLeftPWM);
  motorRight(currentRightPWM);
}

// ======================================
// SETUP
// ======================================

void setup() {

  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32-C3 ROBOT SOCCER");
  Serial.println("SYSTEM INITIALIZATION");
  Serial.println("==============================");

  Serial.print("Reset reason: ");
  Serial.println((int)esp_reset_reason());

  // ====================================
  // MOTOR INITIALIZATION
  // ====================================

  pinMode(L_IN1, OUTPUT);
  pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT);
  pinMode(R_IN2, OUTPUT);

  analogWriteResolution(8);

  stopMotorImmediate();

  Serial.println("Motor protection: ACTIVE");

  // ====================================
  // WIFI INITIALIZATION
  // ====================================

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  esp_err_t wifiPowerResult =
      esp_wifi_set_max_tx_power(34);

  Serial.print("TX Power: ");

  if (wifiPowerResult == ESP_OK) {
    Serial.println("8.5 dBm OK");
  } else {
    Serial.println("ERROR");
  }

  // ====================================
  // REMOTEXY INITIALIZATION
  // ====================================

  Serial.println("Starting RemoteXY...");

  RemoteXY_Init();

  stopMotorImmediate();

  Serial.println("RemoteXY initialization finished");

  Serial.print("SSID: ");
  Serial.println(WiFi.softAPSSID());

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("Channel: ");
  Serial.println(WiFi.channel());

  Serial.println("==============================");
  Serial.println("WAITING FOR REMOTEXY");
}

// ======================================
// LOOP
// ======================================

void loop() {

  // ====================================
  // REMOTEXY PROCESS
  // ====================================

  RemoteXYEngine.handler();

  // ====================================
  // CONNECTION FAILSAFE
  // ====================================

  if (RemoteXY.connect_flag == 0) {

    controlEnabled = false;

    lastValidCommand = 0;

    stopMotorImmediate();

  }

  else {

    // ==================================
    // BACA JOYSTICK
    // ==================================

    int throttle = -RemoteXY.joystick_01_y;
    int steering = RemoteXY.joystick_02_x;

    if (INVERT_THROTTLE) {
      throttle = -throttle;
    }

    if (INVERT_STEERING) {
      steering = -steering;
    }

    throttle = applyDeadzone(throttle);
    steering = applyDeadzone(steering);

    // ==================================
    // NETRAL JOYSTICK
    // ==================================

    bool joystickNeutral =
        (throttle == 0 && steering == 0);

    // ==================================
    // AKTIVASI KENDALI
    // ==================================

    if (!controlEnabled) {

      // Tunggu joystick berada di posisi netral
      if (joystickNeutral) {

        controlEnabled = true;
        lastValidCommand = millis();

        stopMotorImmediate();

        Serial.println("CONTROL ENABLED");
      }

      else {

        stopMotorImmediate();
      }
    }

    // ==================================
    // KENDALI MOTOR
    // ==================================

    if (controlEnabled) {

      lastValidCommand = millis();

      int leftSpeed = throttle + steering;
      int rightSpeed = throttle - steering;

      // NORMALISASI
      int maxSpeed =
          max(abs(leftSpeed), abs(rightSpeed));

      if (maxSpeed > 100) {

        leftSpeed =
            (long)leftSpeed * 100 / maxSpeed;

        rightSpeed =
            (long)rightSpeed * 100 / maxSpeed;
      }

      // KONVERSI PWM
      targetLeftPWM =
          map(abs(leftSpeed), 0, 100, 0, MAX_PWM);

      targetRightPWM =
          map(abs(rightSpeed), 0, 100, 0, MAX_PWM);

      if (leftSpeed < 0) {
        targetLeftPWM = -targetLeftPWM;
      }

      if (rightSpeed < 0) {
        targetRightPWM = -targetRightPWM;
      }

      // RAMP MOTOR
      updateMotorRamp();
    }
  }

  // ====================================
  // TIMEOUT FAILSAFE
  // ====================================

  if (controlEnabled &&
      lastValidCommand != 0 &&
      millis() - lastValidCommand > FAILSAFE_TIMEOUT) {

    controlEnabled = false;

    stopMotorImmediate();

    Serial.println("FAILSAFE TIMEOUT!");
  }

  // ====================================
  // DIAGNOSTIC
  // ====================================

  if (millis() - lastPrint >= 1000) {

    lastPrint = millis();

    Serial.println();
    Serial.println("--------- STATUS ---------");

    Serial.print("Uptime: ");
    Serial.println(millis() / 1000);

    Serial.print("WiFi Mode: ");
    Serial.println((int)WiFi.getMode());

    Serial.print("WiFi IP: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("Connected HP: ");
    Serial.println(WiFi.softAPgetStationNum());

    Serial.print("RemoteXY Connect Flag: ");
    Serial.println((int)RemoteXY.connect_flag);

    Serial.print("Joystick 01 Y: ");
    Serial.println(RemoteXY.joystick_01_y);

    Serial.print("Joystick 02 X: ");
    Serial.println(RemoteXY.joystick_02_x);

    Serial.print("Control Enabled: ");
    Serial.println(controlEnabled);

    Serial.print("Target Left PWM: ");
    Serial.println(targetLeftPWM);

    Serial.print("Target Right PWM: ");
    Serial.println(targetRightPWM);

    Serial.print("Current Left PWM: ");
    Serial.println(currentLeftPWM);

    Serial.print("Current Right PWM: ");
    Serial.println(currentRightPWM);

    Serial.println("--------------------------");
  }

}
