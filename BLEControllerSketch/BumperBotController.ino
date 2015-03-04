/*

Copyright (c) 2012, 2013 RedBearLab

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

//"services.h/spi.h/boards.h" is needed in every new project
#include <SPI.h>
#include <boards.h>
#include <RBL_nRF8001.h>
#include <services.h>
#include <Servo.h> 
 
#define DIGITAL_OUT_PIN    2
#define DIGITAL_IN_PIN     A4
#define PWM_PIN            3
#define SERVO_PIN          5
#define ANALOG_IN_PIN      A5
int leftServoPin = 5;
int rightServoPin = 6;
int limSwitchPin = 12; //has to be this pin, other pins cause problems for some reason
int buzzerPin = 3;

Servo leftServo;
Servo rightServo;

void setup()
{
  // Default pins set to 9 and 8 for REQN and RDYN
  // Set your REQN and RDYN here before ble_begin() if you need
  //ble_set_pins(3, 2);
  
  // Set your BLE Shield name here, max. length 10
  //ble_set_name("My Name");
  
  // Init. and start BLE library.
  ble_begin();
  
  // Enable serial debug
  Serial.begin(57600);
  
  pinMode(DIGITAL_OUT_PIN, OUTPUT);
  pinMode(DIGITAL_IN_PIN, INPUT);
  pinMode(limSwitchPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  
  // Default to internally pull high, change it if you need
  digitalWrite(DIGITAL_IN_PIN, HIGH);
  //digitalWrite(DIGITAL_IN_PIN, LOW);
  
  leftServo.attach(leftServoPin);
  rightServo.attach(rightServoPin);
}
boolean reverse = false;
void loop()
{
  static boolean analog_enabled = false;
  static byte old_state = LOW;
  int rightSpeed = -1;
  int leftSpeed = -1;
    // If data is ready
    while(ble_available())
    {
      // read out command and data
      byte data0 = ble_read();
      byte data1 = ble_read();
      byte data2 = ble_read();
        if (data0 == 0x01)  // Command is to control digital out pin
        {
          if (data1 == 0x01)
            digitalWrite(DIGITAL_OUT_PIN, HIGH);
          else
            digitalWrite(DIGITAL_OUT_PIN, LOW);
        }
        else if (data0 == 0xA0) // Command is to enable analog in reading
        {
          if (data1 == 0x01)
            analog_enabled = true;
          else
            analog_enabled = false;
        }
        else if (data0 == 0x02) // Command is to control PWM pin
        {
          analogWrite(PWM_PIN, data1);
        }
        else if (data0 == 0x03)  // Command is to control Servo pin
        {
          leftSpeed = data1;
          rightSpeed = data2;
        }
        else if (data0 == 0x04)
        {
          analog_enabled = false;
//          leftServo.write(0);
          analogWrite(PWM_PIN, 0);
          digitalWrite(DIGITAL_OUT_PIN, LOW);
        }
        else if(data0 == 0x05) 
        {
          rightSpeed = data1;
        }

    }

    int limswitch = digitalRead(limSwitchPin);
    
    if(limswitch == LOW) {
      //a limit switch is triggered, go in reverse
      leftServo.write(20);
      rightServo.write(165);
      analogWrite(buzzerPin, 150);
      reverse = true; 
    } else if (reverse) {
      //limit switch no longer triggered but robot is in reverse, stop the robot
      leftServo.write(0);
      rightServo.write(0);
      analogWrite(buzzerPin, 0);
      reverse = false;
    }
    else if (ble_connected()){
      //if connected and not doing stuff with reverse, the let bluetooth control
      if (rightSpeed >= 0)
        rightServo.write(rightSpeed);
      if(leftSpeed >= 0) 
        leftServo.write(leftSpeed);
    }
    else if (!ble_connected()) {
      //if loose connection, stop the robot
      rightServo.write(0);
      leftServo.write(0);
    }
    
  
  if (analog_enabled)  // if analog reading enabled
  {
    // Read and send out
    uint16_t value = analogRead(ANALOG_IN_PIN); 
    ble_write(0x0B);
    ble_write(value >> 8);
    ble_write(value);
  }
  
  // If digital in changes, report the state
  if (digitalRead(DIGITAL_IN_PIN) != old_state)
  {
    old_state = digitalRead(DIGITAL_IN_PIN);
    
    if (digitalRead(DIGITAL_IN_PIN) == HIGH)
    {
      ble_write(0x0A);
      ble_write(0x01);
      ble_write(0x00);    
    }
    else
    {
      ble_write(0x0A);
      ble_write(0x00);
      ble_write(0x00);
    }
  }
  
  if (!ble_connected())
  {
    analog_enabled = false;
    digitalWrite(DIGITAL_OUT_PIN, LOW);
  }
  
  // Allow BLE Shield to send/receive data
  ble_do_events();  
}



