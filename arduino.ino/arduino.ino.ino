#include <Servo.h>
#include <Keypad.h>
#include "pitches.h"

// Pin Definitions
#define RAIN_SENSOR_PIN 2
#define ULTRASONIC_TRIG 3
#define ULTRASONIC_ECHO 4
#define IR_SENSOR_PIN 5
#define BUZZER_PIN 6
#define GARAGE_SERVO_PIN 8
#define RAIN_SHED_SERVO_PIN 13

// Servo Motors
Servo rainServo;
Servo garageServo;

// Keypad Configuration
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 10, 11, 12}; // Connect to the row pins of the keypad
byte colPins[COLS] = {A0, A1, A2, A3 }; // Connect to the column pins of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// Thresholds and Constants
#define DISTANCE_THRESHOLD 8
#define GAS_MESG 'G'
#define DOOR_UNLOCK_MESG 'U'

int rain =0;
int garageTimer=0;
bool ifDetect =0;

//gas
bool gasOn =0;
int gasTimer=0;

//door
String enteredCode = "";
String correctCode = "0000"; // Door Password
int incorrectAttempts = 0;

#include <SoftwareSerial.h>

SoftwareSerial nodeMCUSerial(0, 1);

void setup() {
  Serial.begin(9600); // UART communication with NodeMCU
  nodeMCUSerial.begin(9600);

  // Pin Modes
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(IR_SENSOR_PIN, INPUT);

  rainServo.attach(RAIN_SHED_SERVO_PIN);    // Servo for rain shed
  garageServo.attach(GARAGE_SERVO_PIN);  // Servo for garage door

  rainServo.write(0);      // Initial closed position
  garageServo.write(0);    // Initial closed position
}

void loop() {
  
  int rainStatus = digitalRead(RAIN_SENSOR_PIN);

  if(gasTimer>0){
    gasTimer--;
    GasAlertTone();
  }

  checknodeMCUSerial();

  checkRainSensor(rainStatus);
  checkUltrasonicSensor();
  checkIRSensor();
  checkKeypad();

  delay(10); // Adjust delay as needed
}

void checknodeMCUSerial(){
  if (nodeMCUSerial.available()) {
    char receivedMsg = Serial.read();
    // Serial.println(receivedMsg);
    if (receivedMsg == GAS_MESG) {
      gasOn =true;
      gasTimer =40;
      GasAlertTone();
    }
  }
}


void checkRainSensor(int rainStatus) {

  if (rainStatus==0 && rain==0)
  {
    Serial.println("Rain Detected");
    rainServo.write(0); // Open shed
  }
  if (rainStatus==1)
  {
    Serial.println("Rain Stopped");
    rainServo.write(90); // Close shed
    rain =30;
  }
  if(rain)rain--;

}

void checkUltrasonicSensor() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH);
  int distance = duration * 0.034 / 2;
  // Serial.print("Distance: ");
  // Serial.println(distance);
  if (distance <= DISTANCE_THRESHOLD) {
    UltrasonicTone();
    // Serial.println("Activity Detected!");
  }
}

void checkIRSensor() {
  if (digitalRead(IR_SENSOR_PIN) == LOW && ifDetect==0) {
    ifDetect =1;return;
  }
  if (digitalRead(IR_SENSOR_PIN) == LOW & ifDetect) {
    garageTimer =50;
    garageServo.write(90); // Open garage
    // Serial.println("Car Detected!");

    IRTone();
  }
  else{
    if(garageTimer>0){
      garageTimer--;
    }
    else{
      garageServo.write(0); // Close garage
    }
  }
}


void checkKeypad() {
  char key = keypad.getKey();

  if (key) {

    tone(BUZZER_PIN, 300, 100);  // Play tone for 100 ms
    delay(150);
    noTone(6);
    Serial.println(key);
    if (key == '#') {
      if (enteredCode == correctCode) {
        tone(BUZZER_PIN, 1000, 100);  // Play tone for 100 ms
        delay(150);
        tone(BUZZER_PIN, 1000, 100);  // Play tone for 100 ms
        delay(150);
        noTone(6);
        unlockDoor();
      } else {
        incorrectAttempts++;
        enteredCode = ""; // Reset code on incorrect attempt

        if (incorrectAttempts >= 3) {
          threeWrongAttemptTone();
          incorrectAttempts = 0;       // Reset attempts
        }
      }
    } else if (key == '*') {
      enteredCode = ""; // Clear the code input
    } else {
      enteredCode += key; // Add key to entered code
    }
  }
}

void unlockDoor() {
  nodeMCUSerial.write(DOOR_UNLOCK_MESG);
  enteredCode = "";                   // Reset the entered code
  incorrectAttempts = 0;              // Reset the incorrect attempts counter
}

// void activateBuzzer() {
//   for (int i = 0; i < 5; i++) {
//     digitalWrite(BUZZER_PIN, HIGH);
//     delay(1000);                      // Buzzer on for 1 second
//     digitalWrite(BUZZER_PIN, LOW);
//     delay(500);                       // Buzzer off for 0.5 seconds
//   }
// }

void IRTone() {
  noTone(6);
  tone(6, 200, 200);
  delay(200);
  // turn off tone function for pin 7:
  noTone(6);
  // play a note on pin 8 for 300 ms:
  tone(6, 1000, 300);
  delay(300);
}

 
void UltrasonicTone() {
  noTone(6);
  tone(6, 1600, 200);
  delay(20);

  noTone(6);
  // play a note on pin 7 for 500 ms:
  tone(6, 1000, 500);
  delay(50);

  // turn off tone function for pin 7:
  noTone(6);
  // play a note on pin 8 for 300 ms:
  tone(6, 1800, 300);
  delay(30);
}

void GasAlertTone() {
  for (int i = 500; i <= 2000; i += 5) {   // Slowly ascending frequency
    tone(BUZZER_PIN, i);
    delay(2);
  }
  noTone(6);
  for (int i = 2000; i >= 500; i -= 5) {   // Slowly descending frequency
    tone(BUZZER_PIN, i);
    delay(2);
  }
  noTone(6);
  for (int i = 500; i <= 2000; i += 5) {   // Slowly ascending frequency
    tone(BUZZER_PIN, i);
    delay(2);
  }
  noTone(6);
  
}

void threeWrongAttemptTone() {
  for (int i = 1000; i <= 2000; i += 10) {  // Ascending frequency
    tone(BUZZER_PIN, i);
    delay(5);
  }
  for (int i = 2000; i >= 1000; i -= 10) {  // Descending frequency
    tone(BUZZER_PIN, i);
    delay(5);
  }
  for (int i = 1000; i <= 2000; i += 10) {  // Ascending frequency
    tone(BUZZER_PIN, i);
    delay(5);
  }
  for (int i = 2000; i >= 1000; i -= 10) {  // Descending frequency
    tone(BUZZER_PIN, i);
    delay(5);
  }
  for (int i = 1000; i <= 2000; i += 10) {  // Ascending frequency
    tone(BUZZER_PIN, i);
    delay(5);
  }
  for (int i = 2000; i >= 1000; i -= 10) {  // Descending frequency
    tone(BUZZER_PIN, i);
    delay(5);
  }
  noTone(6);
}
