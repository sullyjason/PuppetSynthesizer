/*************************************************************
   Multi-Input MIDI Code

   Toggles (set to 1 or 0):
     - DEBUG
     - IMU_MODE
     - ANALOG_MODE
     - TOF_MODE
     - ULTRASONIC_MODE

   Each sensor is read at a specified interval
   using the millis() approach.

   Debug prints every 200ms if DEBUG = 1.
*************************************************************/

// ------------------------ TOGGLES -------------------------
#define DEBUG            1  // 1 = enable debug prints
#define IMU_MODE         0  // Gyroscope and accelerometer    MIDI channels 20 - 25
#define ANALOG_MODE      0  // Inputs such as faders          MIDI channels 10 - 15
#define TOF_MODE         0  
#define ULTRASONIC_MODE  1  // 

// ----------------------- LIBRARIES ------------------------
#include <Wire.h>
#include <MIDIUSB.h>                 // For sending MIDI
#include <Ultrasonic.h>              // For ultrasonic sensors
#include <SparkFun_VCNL4040_Arduino_Library.h> // For VCNL4040
#include <MPU9250.h>                 // For the MPU-9250

// ------------------- INTERVALS (ms) -----------------------
const unsigned long ULTRASONIC_INTERVAL   =     50;  
const unsigned long TOF_INTERVAL          =     20;  
const unsigned long IMU_INTERVAL          =     10;  
const unsigned long ANALOG_INTERVAL       =     10;  
const unsigned long DEBUG_INTERVAL        =     200; 

// ------------------- GLOBAL VARIABLES ---------------------
#if ULTRASONIC_MODE
int distance0, distance1, distance2;
#endif

#if TOF_MODE
unsigned int proximityVal1;
//unsigned int proximityVal2;
//unsigned int proximityVal3;
#endif

#if IMU_MODE
int accX, accY, accZ;
int gyroX, gyroY, gyroZ;
#endif

#if ANALOG_MODE
int analog0, analog1, analog2, analog3, analog4, analog5;
#endif

// Timestamps to track last read time
unsigned long lastUltrasonic = 0;
unsigned long lastTOF        = 0;
unsigned long lastIMU        = 0;
unsigned long lastAnalog     = 0;
unsigned long lastDebug      = 0;

// Pin definitions
#define TRIGGER_1    12
#define ECHO_1       13
#define TRIGGER_2    11
#define ECHO_2       10
#define TRIGGER_3    9
#define ECHO_3       8

#define ANALOG0      A0
#define ANALOG1      A1
#define ANALOG2      A2
#define ANALOG3      A3
#define ANALOG4      A4
#define ANALOG5      A5

// -------------------- SENSOR OBJECTS ----------------------
#if ULTRASONIC_MODE
  Ultrasonic ultrasonic1(TRIGGER_1, ECHO_1);
  Ultrasonic ultrasonic2(TRIGGER_2, ECHO_2);
  Ultrasonic ultrasonic3(TRIGGER_3, ECHO_3);
#endif

#if TOF_MODE
VCNL4040 proximitySensor1;
//VCNL4040 proximitySensor2;
//VCNL4040 proximitySensor3;
#endif

#if IMU_MODE
MPU9250 mpu;
#endif

// -------------------- SETUP FUNCTION ----------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  #if ULTRASONIC_MODE
  // no special init needed for Ultrasonic
  #endif

  #if TOF_MODE
    if (!proximitySensor.begin()) {
      Serial.println("VCNL4040 not found. Check wiring.");
      while (1);
    }
    Serial.println("VCNL4040 initialized.");
  #endif

  #if IMU_MODE
    if (!mpu.setup(0x68)) {
      Serial.println("MPU-9250 connection failed!");
      while (1);
    }
    Serial.println("MPU-9250 initialized.");
  #endif
}

// --------------------- MAIN LOOP -------------------------
void loop() {
  unsigned long now = millis(); // current time

  // ---------- ULTRASONIC ----------
  #if ULTRASONIC_MODE
  if ((now - lastUltrasonic) >= ULTRASONIC_INTERVAL) {
    lastUltrasonic = now;
    readUltrasonic(); 
  }
  #endif

  // ---------- TOF (VCNL4040) ----------
  #if TOF_MODE
  if ((now - lastTOF) >= TOF_INTERVAL) {
    lastTOF = now;
    readTOF();
  }
  #endif

  // ---------- IMU (MPU-9250) ----------
  #if IMU_MODE
  if ((now - lastIMU) >= IMU_INTERVAL) {
    lastIMU = now;
    readIMU();
  }
  #endif

  // ---------- ANALOG ----------
  #if ANALOG_MODE
  if ((now - lastAnalog) >= ANALOG_INTERVAL) {
    lastAnalog = now;
    readAnalogs();
  }
  #endif

  // ---------- DEBUG PRINT ----------
  #if DEBUG
  if ((now - lastDebug) >= DEBUG_INTERVAL) {
    lastDebug = now;
    debugPrint();
  }
  #endif
}

// ------------------- SENSOR FUNCTIONS ---------------------
#if ULTRASONIC_MODE
void readUltrasonic() {
  distance0 = ultrasonic1.read();
  //distance1 = ultrasonic2.read();
  //distance2 = ultrasonic3.read();

  // For demonstration: send MIDI
  //byte midiDist0 = (distance0 > 127) ? 127 : distance0;
  //byte midiDist1 = (distance1 > 127) ? 127 : distance1;
  //byte midiDist2 = (distance2 > 127) ? 127 : distance2;

  sendMIDIControlChange(40, map(distance0, 2, 37, 0, 127));

  //sendMIDIControlChange(41, midiDist1);
  //sendMIDIControlChange(42, midiDist2);
  MidiUSB.flush();
}
#endif

#if TOF_MODE
void readTOF() {
  proximityVal = proximitySensor.getProximity(); // 0..65535
  byte mappedProx = map(proximityVal, 0, 65535, 0, 127);

  sendMIDIControlChange(30, mappedProx);
  MidiUSB.flush();
}
#endif

#if IMU_MODE
void readIMU() {
  if (mpu.update()) {
    accX = map(mpu.getAccX() * 100, -200, 200, 0, 127);
    accY = map(mpu.getAccY() * 100, -200, 200, 0, 127);
    accZ = map(mpu.getAccZ() * 100, -200, 200, 0, 127);

    gyroX = map(mpu.getGyroX(), -200, 200, 0, 127);
    gyroY = map(mpu.getGyroY(), -200, 200, 0, 127);
    gyroZ = map(mpu.getGyroZ(), -200, 200, 0, 127);

    sendMIDIControlChange(20, accX);
    sendMIDIControlChange(21, accY);
    sendMIDIControlChange(22, accZ);
    sendMIDIControlChange(23, gyroX);
    sendMIDIControlChange(24, gyroY);
    sendMIDIControlChange(25, gyroZ);
    MidiUSB.flush();
  }
}
#endif

#if ANALOG_MODE
void readAnalogs() {
  analog0 = analogRead(ANALOG0);
  analog1 = analogRead(ANALOG1);
  analog2 = analogRead(ANALOG2);
  analog3 = analogRead(ANALOG3);
  analog4 = analogRead(ANALOG4);
  analog5 = analogRead(ANALOG5);

  sendMIDIControlChange(10, map(analog0, 253, 765, 0, 127));
  sendMIDIControlChange(11, map(analog1, 253, 765, 0, 127));
  sendMIDIControlChange(12, map(analog2, 253, 765, 0, 127));
  sendMIDIControlChange(13, map(analog3, 253, 765, 0, 127));
  sendMIDIControlChange(14, map(analog4, 253, 765, 0, 127));
  sendMIDIControlChange(15, map(analog5, 253, 765, 0, 127));
  MidiUSB.flush();
}
#endif

// --------------------- DEBUG PRINT -----------------------
#if DEBUG
void debugPrint() {
  // Print ULTRASONIC
  #if ULTRASONIC_MODE
    Serial.print("Ultrasonic: ");
    Serial.print(distance0); Serial.print(", ");
    //Serial.print(distance1); Serial.print(", ");
    //Serial.print(distance2);
    //Serial.print(" | ");
  #endif

  // Print TOF
  #if TOF_MODE
    Serial.print("Proximity: ");
    Serial.print(proximityVal);
    Serial.print(" | ");
  #endif

  // Print ANALOG
  #if ANALOG_MODE
    Serial.print("Analog: ");
    Serial.print(analog0); Serial.print(",");
    Serial.print(analog1); Serial.print(",");
    Serial.print(analog2); Serial.print(",");
    Serial.print(analog3); Serial.print(",");
    Serial.print(analog4); Serial.print(",");
    Serial.print(analog5);
    Serial.print(" | ");
  #endif

  // Print IMU
  #if IMU_MODE
    Serial.print("Accel: ");
    Serial.print(accX); Serial.print(",");
    Serial.print(accY); Serial.print(",");
    Serial.print(accZ);
    Serial.print(" Gyro: ");
    Serial.print(gyroX); Serial.print(",");
    Serial.print(gyroY); Serial.print(",");
    Serial.print(gyroZ);
  #endif

  Serial.println();
}
#endif

// -------------------- MIDI HELPER ------------------------
void sendMIDIControlChange(byte control, byte value) {
  // CC on Channel 1 => 0xB0
  midiEventPacket_t event = {0x0B, 0xB0, control, value};
  MidiUSB.sendMIDI(event);
}