
/*
   ROBOT SOCCER
   ESP32-C3 SuperMini
   RemoteXY Bluetooth BLE
   MX1508 Motor Driver
   Dual Joystick

   Joystick kiri Y  : Throttle
   Joystick kanan X : Steering

   Bluetooth Name  : Robot3
   Password        : 124124124

   Motor Pin:
   Left  IN1 = GPIO 10
   Left  IN2 = GPIO 0
   Right IN1 = GPIO 1
   Right IN2 = GPIO 3

   FEATURES:
   - Bluetooth BLE
   - Startup motor protection
   - Joystick neutral calibration
   - Communication disconnect failsafe
   - Deadzone
   - PWM ramp control
   - Motor diagnostic

   RemoteXY Library 4.1.10 or later
*/

// ======================================
// BLUETOOTH BLE CONFIGURATION
// ======================================

#define REMOTEXY_MODE__ESP32CORE_BLE

#include <BLEDevice.h>
#include <esp_system.h>

#define REMOTEXY_BLUETOOTH_NAME "Robot3"
#define REMOTEXY_ACCESS_PASSWORD "124124124"

#define REMOTEXY__DEBUGLOG
#define REMOTEXY__DEBUGLOG_SERIAL Serial
#define REMOTEXY__DEBUGLOG_SPEED 115200

#include <RemoteXY.h>


// ======================================
// REMOTEXY GUI CONFIGURATION
// GUI DARI KODE BLE HASIL GENERATE
// ======================================

#pragma pack(push, 1)

uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] = {
  255,4,0,0,0,47,0,19,0,0,0,
  82,111,98,111,116,51,0,8,2,106,200,
  200,74,1,1,2,0,5,200,14,162,162,
  16,5,55,55,32,2,26,31,5,0,14,
  162,162,128,6,54,54,32,2,26,79
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
// MOTOR PIN MX1508
// ======================================

#define L_IN1 10
#define L_IN2 0

#define R_IN1 1
#define R_IN2 3


// ======================================
// MOTOR CONTROL SETTINGS
// ======================================

#define DEADZONE 10

#define MAX_PWM 255

// PWM ramp
#define RAMP_STEP 8

// Interval PWM ramp
#define RAMP_INTERVAL_MS 5

// Invert direction if needed
const bool INVERT_THROTTLE = false;
const bool INVERT_STEERING = false;


// ======================================
// SYSTEM VARIABLES
// ======================================

int currentLeftPWM = 0;
int currentRightPWM = 0;

int targetLeftPWM = 0;
int targetRightPWM = 0;

bool controlEnabled = false;

unsigned long lastPrint = 0;
unsigned long lastRampUpdate = 0;


// ======================================
// MOTOR LEFT
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
// MOTOR RIGHT
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
// IMMEDIATE MOTOR STOP
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
// PWM RAMP
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


// ======================================
// UPDATE MOTOR RAMP
// ======================================

void updateMotorRamp() {

  unsigned long now = millis();

  if (now - lastRampUpdate < RAMP_INTERVAL_MS) {
    return;
  }

  lastRampUpdate = now;

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

  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32-C3 ROBOT SOCCER");
  Serial.println("BLUETOOTH BLE MODE");
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
  // BLUETOOTH INITIALIZATION
  // ====================================

  Serial.println("Starting Bluetooth BLE...");

  RemoteXY_Init();

  stopMotorImmediate();

  Serial.println("Bluetooth initialization finished");

  Serial.print("Bluetooth Name: ");
  Serial.println(REMOTEXY_BLUETOOTH_NAME);

  Serial.println("Waiting for smartphone...");
  Serial.println("==============================");

}


// ======================================
// MAIN LOOP
// ======================================

void loop() {

  // ====================================
  // REMOTEXY BLE PROCESS
  // ====================================

  RemoteXYEngine.handler();


  // ====================================
  // CONNECTION FAILSAFE
  // ====================================

  if (RemoteXY.connect_flag == 0) {

    controlEnabled = false;

    stopMotorImmediate();

  }

  else {

    // ==================================
    // READ JOYSTICK
    // ==================================

    int throttle = -RemoteXY.joystick_01_y;

    int steering = RemoteXY.joystick_02_x;


    // ==================================
    // DIRECTION INVERSION
    // ==================================

    if (INVERT_THROTTLE) {
      throttle = -throttle;
    }

    if (INVERT_STEERING) {
      steering = -steering;
    }


    // ==================================
    // APPLY DEADZONE
    // ==================================

    throttle = applyDeadzone(throttle);

    steering = applyDeadzone(steering);


    // ==================================
    // JOYSTICK NEUTRAL DETECTION
    // ==================================

    bool joystickNeutral =
        (throttle == 0 && steering == 0);


    // ==================================
    // STARTUP CONTROL PROTECTION
    // ==================================

    if (!controlEnabled) {

      // Wait until both joysticks are neutral

      if (joystickNeutral) {

        controlEnabled = true;

        stopMotorImmediate();

        Serial.println("CONTROL ENABLED");

      }

      else {

        stopMotorImmediate();

      }

    }


    // ==================================
    // MOTOR CONTROL
    // ==================================

    if (controlEnabled) {

      // Differential drive mixing

      int leftSpeed = throttle + steering;

      int rightSpeed = throttle - steering;


      // ==================================
      // SPEED NORMALIZATION
      // ==================================

      int maxSpeed =
          max(abs(leftSpeed), abs(rightSpeed));

      if (maxSpeed > 100) {

        leftSpeed =
            (long)leftSpeed * 100 / maxSpeed;

        rightSpeed =
            (long)rightSpeed * 100 / maxSpeed;

      }


      // ==================================
      // PWM CONVERSION
      // ==================================

      targetLeftPWM =
          map(abs(leftSpeed), 0, 100, 0, MAX_PWM);

      targetRightPWM =
          map(abs(rightSpeed), 0, 100, 0, MAX_PWM);


      // ==================================
      // MOTOR DIRECTION
      // ==================================

      if (leftSpeed < 0) {
        targetLeftPWM = -targetLeftPWM;
      }

      if (rightSpeed < 0) {
        targetRightPWM = -targetRightPWM;
      }


      // ==================================
      // UPDATE PWM RAMP
      // ==================================

      updateMotorRamp();

    }

  }


  // ====================================
  // SERIAL DIAGNOSTIC
  // ====================================

  if (millis() - lastPrint >= 1000) {

    lastPrint = millis();

    Serial.println();
    Serial.println("--------- STATUS ---------");

    Serial.print("Uptime: ");
    Serial.println(millis() / 1000);

    Serial.print("BLE Connect Flag: ");
    Serial.println((int)RemoteXY.connect_flag);

    Serial.print("Joystick 01 X: ");
    Serial.println(RemoteXY.joystick_01_x);

    Serial.print("Joystick 01 Y: ");
    Serial.println(RemoteXY.joystick_01_y);

    Serial.print("Joystick 02 X: ");
    Serial.println(RemoteXY.joystick_02_x);

    Serial.print("Joystick 02 Y: ");
    Serial.println(RemoteXY.joystick_02_y);

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
