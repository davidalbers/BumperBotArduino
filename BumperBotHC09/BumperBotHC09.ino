#include <Servo.h> 
char dataBytes[3];

int leftServoPin = 5;
int rightServoPin = 6;

Servo leftServo;
Servo rightServo;

void setup() {
  // Bluetooth communication 
  // happens on serial
  Serial.begin(9600);
  
  leftServo.attach(leftServoPin);
  rightServo.attach(rightServoPin);
}

void loop() {
  if(Serial.available()) {
    Serial.readBytes(dataBytes, 3);
    int leftSpeed = -1;
    int rightSpeed = -1;
    if (dataBytes[0] == 0x03)  // Command is to control Servo pin
    {
      leftSpeed = (int)dataBytes[1];
      rightSpeed = (int)dataBytes[2];
      if (rightSpeed >= 0)
        rightServo.write(rightSpeed);
      if(leftSpeed >= 0) 
        leftServo.write(leftSpeed);
    }
  }
  delay(10);
 }
