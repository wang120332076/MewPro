// MewPro
//
//   Copyright (c) 2014-2016 orangkucing
//
// MewPro firmware version string for maintenance
#define MEWPRO_FIRMWARE_VERSION "2016032600"
#define BAUD_RATE 38400

#include <Arduino.h>
#include <EEPROM.h>
#include "MewPro.h"

// enable console output
// set false if this is MewPro #0 of dual dongle configuration
boolean debug = false;

//********************************************************
// c_I2C: I2C interface (THIS PART CAN'T BE OPTED OUT)
// 
// Note: in order to use MewPro reliably, THE FOLLOWING MODIFICATIONS TO STANDARD ARDUINO LIBRARY SOURCE IS
// STRONGLY RECOMMENDED:
//
// Arduino Pro Mini / Arduino Pro Micro
//     1. hardware/arduino/avr/libraries/Wire/Wire.h
//            old: #define BUFFER_LENGTH 32                        -->   new: #define BUFFER_LENGTH 64
//     2. hardware/arduino/avr/libraries/Wire/utility/twi.h
//            old: #define TWI_BUFFER_LENGTH 32                    -->   new: #define TWI_BUFFER_LENGTH 64
#include <Wire.h> // *** please comment out this and the following three lines if __MK20DX256__ or __MK20DX128__ or __MKL26Z64__ or __AVR_ATtiny1634__ is defined ***
#if BUFFER_LENGTH < 64
#error Please modify Arduino Wire library source code to increase the I2C buffer size
#endif
//
// ATtiny1634 core is included in https://github.com/SpenceKonde/ATTinyCore
//    WireS library is downloadable from https://github.com/orangkucing/WireS
//#include <WireS.h> // *** please comment out this line if __AVR_ATtiny1634__ is not defined ***
//Seven: Why not using #ifdef

//********************************************************
// k_Genlock: Generator Lock
//   Note: MewPro #0 in dual dongle configuration should always boolean debug = false;
#define  USE_GENLOCK       

// it is better to define this when RXI is connected to nothing (eg. MewPro #0 of Genlock system)
#undef  UART_RECEIVER_DISABLE

// end of Options
//////////////////////////////////////////////////////////

#include "DummySerial.h"
boolean lastHerobusState = LOW;  // Will be HIGH when camera attached.
int eepromId = 0;

void userSettings()
{
  // This function is called once after camera boot.
  // you can set put any camera commands here. For example:
  // if (!isMaster()) {
  //   queueIn(F("!")); // if slave change to master
  // }
  // queueIn(F("VO0")); // preview off
  // queueIn(F("CM0")); // change camera mode to video
  // queueIn(F("TI5"));
}

void setup()
{
  // Remark. Arduino Pro Mini 328 3.3V 8MHz is too slow to catch up with the highest 115200 baud.
  //     cf. http://forum.arduino.cc/index.php?topic=54623.0
  // Set 57600 baud or slower.
  Serial_begin(BAUD_RATE);              //Seven: Slow down baud rate here
#ifdef UART_RECEIVER_DISABLE
#ifndef __AVR_ATmega32U4__
  UCSR0B &= (~_BV(RXEN0));
#else
  UCSR1B &= (~_BV(RXEN1));
#endif
#endif

  setupGenlock();                 //Seven: Remain this line and remove other setup()s

  setupLED(); // onboard LED setup 
  pinMode(BPRDY, OUTPUT); digitalWrite(BPRDY, LOW);    // Show camera MewPro attach. 

  // don't forget to switch pin configurations to INPUT.
  pinMode(I2CINT, INPUT);  // Teensy: default disabled
  pinMode(HBUSRDY, INPUT); // default: analog input
  digitalWrite(PWRBTN, HIGH);
  pinMode(PWRBTN, OUTPUT);  // default: analog input
}

void loop() 
{
  // Attach or detach bacpac
  if (digitalRead(HBUSRDY) == HIGH) {
  //if ((PINC & _BV(0)) == 1) { // speed up!
    if (lastHerobusState != HIGH) {
#if !defined(USE_I2C_PROXY)
      pinMode(I2CINT, OUTPUT); digitalWrite(I2CINT, HIGH);
#endif
      lastHerobusState = HIGH;
      if (eepromId == 0) {
        isMaster(); // determine master/slave and resetI2C()
      } else {
        resetI2C();
      }
    }
  } else {
    if (lastHerobusState != LOW) {
      pinMode(I2CINT, INPUT);
      lastHerobusState = LOW;
    }
  }
  checkBacpacCommands();  //Seven: Check this
  checkCameraCommands();  //Seven: Check this
  checkGenlock();       //Seven: Check this
  checkStatus();        //Seven: It's no use, just ignore this 
}

