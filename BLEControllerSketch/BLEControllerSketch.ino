/*

Copyright (c) 2012, 2013 RedBearLab

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
/*
This code was once used, but is no longer used.
It is only kept for reference.
Please use BumperBotController.ino instead

#include <Servo.h> 
#include <SPI.h>
#include <boards.h>
#include <RBL_nRF8001.h>
#include "Boards.h"

#define PROTOCOL_MAJOR_VERSION   0 //
#define PROTOCOL_MINOR_VERSION   0 //
#define PROTOCOL_BUGFIX_VERSION  2 // bugfix

#define PIN_CAPABILITY_NONE      0x00
#define PIN_CAPABILITY_DIGITAL   0x01
#define PIN_CAPABILITY_ANALOG    0x02
#define PIN_CAPABILITY_PWM       0x04
#define PIN_CAPABILITY_SERVO     0x08
#define PIN_CAPABILITY_I2C       0x10

// pin modes
//#define INPUT                 0x00 // defined in wiring.h
//#define OUTPUT                0x01 // defined in wiring.h
#define ANALOG                  0x02 // analog pin in analogInput mode
#define PWM                     0x03 // digital pin in PWM output mode
#define SERVO                   0x04 // digital pin in Servo output mode

byte pin_mode[TOTAL_PINS];
byte pin_state[TOTAL_PINS];
byte pin_pwm[TOTAL_PINS];
byte pin_servo[TOTAL_PINS];

Servo servos[MAX_SERVOS];

int led = 13;
int buzzer = 6;
int ledBlinkInterval = 300;

void setup()
{
  Serial.begin(57600);
  Serial.println("BLE Arduino Slave");
  
  /* Default all to digital input */
  for (int pin = 0; pin < TOTAL_PINS; pin++)
  {
    // Set pin to input with internal pull up
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);

    // Save pin mode and state
    pin_mode[pin] = INPUT;
    pin_state[pin] = LOW;
  }
  
  // Default pins set to 9 and 8 for REQN and RDYN
  // Set your REQN and RDYN here before ble_begin() if you need
  //ble_set_pins(3, 2);
  
  // Set your BLE Shield name here, max. length 10
  //ble_set_name("My Name");
  
  // Init. and start BLE library.
  ble_begin();
}

static byte buf_len = 0;

void ble_write_string(byte *bytes, uint8_t len)
{
  if (buf_len + len > 20)
  {
    for (int j = 0; j < 15000; j++)
      ble_do_events();
    
    buf_len = 0;
  }
  
  for (int j = 0; j < len; j++)
  {
    ble_write(bytes[j]);
    buf_len++;
  }
    
  if (buf_len == 20)
  {
    for (int j = 0; j < 15000; j++)
      ble_do_events();
    
    buf_len = 0;
  }  
}

byte reportDigitalInput()
{
  if (!ble_connected())
    return 0;

  static byte pin = 0;
  byte report = 0;
  
  if (!IS_PIN_DIGITAL(pin))
  {
    pin++;
    if (pin >= TOTAL_PINS)
      pin = 0;
    return 0;
  }
  
  if (pin_mode[pin] == INPUT)
  {
      byte current_state = digitalRead(pin);
            
      if (pin_state[pin] != current_state)
      {
        pin_state[pin] = current_state;
        byte buf[] = {'G', pin, INPUT, current_state};
        ble_write_string(buf, 4);
        
        report = 1;
      }
  }
  
  pin++;
  if (pin >= TOTAL_PINS)
    pin = 0;
    
  return report;
}

void reportPinCapability(byte pin)
{
  byte buf[] = {'P', pin, 0x00};
  byte pin_cap = 0;
                    
  if (IS_PIN_DIGITAL(pin))
    pin_cap |= PIN_CAPABILITY_DIGITAL;
            
  if (IS_PIN_ANALOG(pin))
    pin_cap |= PIN_CAPABILITY_ANALOG;

  if (IS_PIN_PWM(pin))
    pin_cap |= PIN_CAPABILITY_PWM;

  if (IS_PIN_SERVO(pin))
    pin_cap |= PIN_CAPABILITY_SERVO;

  buf[2] = pin_cap;
  ble_write_string(buf, 3);
}

void reportPinServoData(byte pin)
{
//  if (IS_PIN_SERVO(pin))
//    servos[PIN_TO_SERVO(pin)].write(value);
//  pin_servo[pin] = value;
  
  byte value = pin_servo[pin];
  byte mode = pin_mode[pin];
  byte buf[] = {'G', pin, mode, value};         
  ble_write_string(buf, 4);
}

byte reportPinAnalogData()
{
  if (!ble_connected())
    return 0;
    
  static byte pin = 0;
  byte report = 0;
  
  if (!IS_PIN_DIGITAL(pin))
  {
    pin++;
    if (pin >= TOTAL_PINS)
      pin = 0;
    return 0;
  }
  
  if (pin_mode[pin] == ANALOG)
  {
    uint16_t value = analogRead(pin);
    byte value_lo = value;
    byte value_hi = value>>8;
    
    byte mode = pin_mode[pin];
    mode = (value_hi << 4) | mode;
    
    byte buf[] = {'G', pin, mode, value_lo};         
    ble_write_string(buf, 4);
  }
  
  pin++;
  if (pin >= TOTAL_PINS)
    pin = 0;
    
  return report;
}

void reportPinDigitalData(byte pin)
{
  byte state = digitalRead(pin);
  byte mode = pin_mode[pin];
  byte buf[] = {'G', pin, mode, state};         
  ble_write_string(buf, 4);
}

void reportPinPWMData(byte pin)
{
  byte value = pin_pwm[pin];
  byte mode = pin_mode[pin];
  byte buf[] = {'G', pin, mode, value};         
  ble_write_string(buf, 4);
}

void sendCustomData(uint8_t *buf, uint8_t len)
{
  uint8_t data[20] = "Z";
  memcpy(&data[1], buf, len);
  ble_write_string(data, len+1);
}

byte queryDone = false;

void loop()
{
  while(ble_available())
  {
    byte cmd;
    cmd = ble_read();
    Serial.write(cmd);
    
    // Parse data here
    switch (cmd)
    {
      case 0x01:
      { 
        analogWrite(buzzer, 1);
        delay(500);
        analogWrite(buzzer, 0);
        break;
      }
      case 0x02:
      {
        analogWrite(buzzer, 1);
        delay(500);
        analogWrite(buzzer, 0);
        delay(500);
        analogWrite(buzzer, 1);
        delay(500);
        analogWrite(buzzer, 0);
        break;
      }
      case 0x03:
      {
        //right
        digitalWrite(led, HIGH);
        analogWrite(buzzer, 1);
        delay(500);
        analogWrite(buzzer, 0);
        delay(500);
        analogWrite(buzzer, 1);
        delay(500);
        analogWrite(buzzer, 0);
        delay(500);
        analogWrite(buzzer, 1);
        delay(500);
        analogWrite(buzzer, 0);
        break;
      }
      default:
        break;
    }

    // send out any outstanding data
    ble_do_events();
    buf_len = 0;
    
    return; // only do this task in this loop
  }
    
  ble_do_events();
  buf_len = 0;
}

*/