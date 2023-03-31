/**********************************************************************
  Filename    : RF24_Remote_Car.ino
  Product     : Freenove 4WD Car for UNO
  Description : A RF24 Remote Car.
  Auther      : www.freenove.com
  Modification: 2020/11/27
**********************************************************************/
#include "Freenove_4WD_Car_for_Arduino.h"
#include "RF24_Remote.h"
#include <Wire.h>
#include "SSD1306AsciiWire.h"
#include <Servo.h>

#define I2C_ADDRESS 0x3C
#define NRF_UPDATE_TIMEOUT    1000

SSD1306AsciiWire oled;
Servo servo1;
u32 lastNrfUpdateTime = 0;

void setup() {
  setupScreen();
  clearScreen();

  screenPrint(1,3,"Group 1");
  getBatteryVoltage();
  char buffer[6];
  dtostrf(batteryVoltage,4,2,buffer);
  screenPrint(2,1,buffer);
  screenPrint(2,6,"Volts");
  screenPrint(3,1,"Mode:");

  pinsSetup();
  servo1.attach(PIN_SERVO);
  if (!nrf24L01Setup()) {
    alarm(4, 2);
  }
}

void loop() {
  if (getNrf24L01Data()) {
    clearNrfFlag();
    updateCarActionByNrfRemote();
    
    float servoSpeed = nrfDataRead[0] / 1023.0;

    if (nrfDataRead[5] == 0)
    {
      servo1.write(int(91+(servoSpeed*36)));
    } 
    else if (nrfDataRead[6] == 0)
    {
      servo1.write(int(91-(servoSpeed*32)));
    }
    else
    {
      servo1.write(91);
    }

    lastNrfUpdateTime = millis();
  }
  if (millis() - lastNrfUpdateTime > NRF_UPDATE_TIMEOUT) {
    lastNrfUpdateTime = millis();
    resetNrfDataBuf();
    updateCarActionByNrfRemote();
  }
}

void setupScreen() {
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.clear();
  oled.setFont(fixed_bold10x15);
}

void clearScreen() {
  oled.clear();
}

void screenPrint(int row, int col, char* value) {
  //   this funct clears the line from the specified column to the end
  oled.clearField((col - 1) *11, (row - 1) * 2, 12);   
  // and prints the string starting the the row and column location
  oled.print(value);  
}

