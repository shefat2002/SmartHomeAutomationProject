#define BLYNK_TEMPLATE_ID           "TMPL6g3fIvYqV"
#define BLYNK_TEMPLATE_NAME         "Smart Home Automation System"
#define BLYNK_AUTH_TOKEN            "TgGtvuOFhZXZrjC3VgxPN7mxwSaYOWTL"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>
// Define RX and TX pins for SoftwareSerial (for Arduino Nano)
SoftwareSerial NanoSerial(D5, D6); // RX, TX  

char auth[] = "TgGtvuOFhZXZrjC3VgxPN7mxwSaYOWTL";
char ssid[] = "Free";
char pass[] = "password";

//Physical Pin
#define MQ2_PIN A0
#define RED_PIN D0
#define YELLOW_PIN D1
#define GREEN_PIN D2
#define FAN_PIN D3
#define DOOR_LOCK_PIN D4
  
//Virtual Pin
#define RED_V_PIN V1
#define YELLOW_V_PIN V2
#define GREEN_V_PIN V0
#define FAN_V_PIN V3
#define MQ2_V_PIN V4
#define DOOR_LOCK_V_PIN V5

#define gasThreshold 300
#define GAS_MESG 'G'
#define DOOR_UNLOCK_MESG 'U'

int gasTimer=0;
bool gasOn=0;
int gasTimeDelay =0;

bool isFanOn=false;

int doorOpen =0;
bool doorOpenByKeypad =false;

bool isDoorUnlocked=false;


void setup() {
  Serial.begin(9600); // UART communication with Arduino Nano
  NanoSerial.begin(9600);

  Blynk.begin(auth,ssid,pass);

  pinMode(MQ2_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(DOOR_LOCK_PIN, OUTPUT);

  // Connecting to WiFi
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

    // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
    // Print the local IP address
  Serial.println();
  Serial.print("NodeMCU IP Address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  
  Blynk.run();

  if(gasTimer>0){
    gasTimer--;
  }
  if(gasTimer==0 && gasOn){
    gasOn=false;
    turnFanOff();
  }
  if(gasTimeDelay)gasTimeDelay--;

  if(doorOpen>0){
    doorOpen--;
  }
  if(doorOpenByKeypad && doorOpen ==0){
    doorOpenByKeypad=false;
    lockDoor();
  }

  checkGasLevel();
  checkSerialCommands();
  delay(100);
}

void checkGasLevel() {
  int gasLevel = analogRead(MQ2_PIN);

  if(gasTimeDelay==0){
    Blynk.virtualWrite(MQ2_V_PIN, gasLevel);
    gasTimeDelay = 20;
  }

  if (gasLevel > gasThreshold) {
    gasOn=true;
    gasTimer = 400;
    turnFanOn();
    if (NanoSerial.availableForWrite()) {
      Serial.println(GAS_MESG);
      NanoSerial.write(GAS_MESG); // Notify Nano to activate buzzer
    }
  }
}

// Fan control logic
void turnFanOn() {
  if (!isFanOn) {
    digitalWrite(FAN_PIN, HIGH);
    isFanOn = true;
    Blynk.virtualWrite(FAN_V_PIN, 1);
  }
}

void turnFanOff() {
  if (isFanOn) {
    digitalWrite(FAN_PIN, LOW);
    isFanOn = false;
    Blynk.virtualWrite(FAN_V_PIN, 0);
  }
}

void checkSerialCommands() {
  if (NanoSerial.available() > 0) {
    char command = Serial.read();
    if (command == DOOR_UNLOCK_MESG) {         // 'U' indicates a successful password
      doorOpen =5000;
      doorOpenByKeypad = true;
      unlockDoor();
    }
  }
}

void unlockDoor() {
  digitalWrite(DOOR_LOCK_PIN, HIGH); // Unlock door
  isDoorUnlocked = true;
  Blynk.virtualWrite(DOOR_LOCK_V_PIN, 1);
}

// Lock the door
void lockDoor() {
  digitalWrite(DOOR_LOCK_PIN, LOW); // Lock door
  isDoorUnlocked = false;
  Blynk.virtualWrite(DOOR_LOCK_V_PIN, 0); // Reflect state in Blynk
}



//blynk
BLYNK_WRITE(FAN_V_PIN) {
  int value = param.asInt();
  if (value == 1) {
    turnFanOn(); // Turn fan on manually
  } else {
    turnFanOff(); // Turn fan off manually
  }
}

BLYNK_WRITE(DOOR_LOCK_V_PIN) {
  int value = param.asInt();
  if (value == 1) {
    unlockDoor();
  } else {
    lockDoor();
  }
}

BLYNK_WRITE(RED_V_PIN) { digitalWrite(RED_PIN, param.asInt()); }
BLYNK_WRITE(YELLOW_V_PIN) { digitalWrite(YELLOW_PIN, param.asInt()); }
BLYNK_WRITE(GREEN_V_PIN) { digitalWrite(GREEN_PIN, param.asInt()); }