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

#define MODE_STOP 0
#define MODE_TELEOP 1
#define MODE_AUTO 2

#define AUTO_MODE_STOP 0
#define AUTO_MODE_FORWARD 1
#define AUTO_MODE_BACKWARD 2
#define AUTO_MODE_RIGHTTURN 3
#define AUTO_MODE_LEFTTURN 4
#define AUTO_MODE_SERVO_POS 5
#define AUTO_MODE_SERVO_NEG 6

int mode, automode, start_time, current_time, delta_time;

SSD1306AsciiWire oled;
Servo servo1;
u32 lastNrfUpdateTime = 0;

void setup() {
  setupScreen();
  clearScreen();
  //test
  screenPrint(1,3,"Group 1");
  getBatteryVoltage();
  char buffer[6];
  dtostrf(batteryVoltage,4,2,buffer);
  screenPrint(2,1,buffer);
  screenPrint(2,6,"Volts");
  screenPrint(3,1,"Mode:");
  pinsSetup();
  servo1.attach(PIN_SERVO);
  if (!nrf24L01Setup()) 
  {
    alarm(4, 2);
  }

  mode = MODE_STOP;
  screenPrint(3,6,"STOP"); //Initilize so that the car is on stop mode once a signal is read
}

void loop() 
{
  if (getNrf24L01Data()) 
  {
    clearNrfFlag();

    if (mode == MODE_STOP) 
    {
      if ( nrfDataRead[5] == 0 ) //Once buttons is pressed down switch from stop to auto mode
      {
        mode = MODE_AUTO;
        automode = AUTO_MODE_STOP;
        screenPrint(3,6,"AUTO");
        start_time = millis();
        //Serial.println(start_time);
      }  
      else if ( nrfDataRead[7] == 0)
      {
        mode = MODE_TELEOP;
        screenPrint(3, 6, "TELEOP");
      }
    }

    if (mode == MODE_AUTO)
    {
        current_time = millis();
        delta_time = current_time - start_time; //Time since mode started, used as a check to change to manual mode after 15 seconds

        if (delta_time > 15000)
        {
          mode = MODE_TELEOP;  
          screenPrint(3,6,"TELEOP");
        } 
        else 
        {
          autonomous(delta_time);
        }
    }

    if (mode == MODE_TELEOP ) //Manual mode
    {
      updateCarActionByNrfRemote();
      
      float servoSpeed = nrfDataRead[0] / 1023.0;

      if (nrfDataRead[5] == 0)
      {
        servo1.write(int(91+(servoSpeed*36)));
      } 
      else if (nrfDataRead[7] == 0)
      {
        servo1.write(int(91-(servoSpeed*32)));
      }
      else if (nrfDataRead[6] == 0)
      {
        mode = MODE_STOP;
        screenPrint(3,6,"STOP");
      }
      else
      {
        servo1.write(91);
      }
    }
      lastNrfUpdateTime = millis();
  }
  
  if (millis() - lastNrfUpdateTime > NRF_UPDATE_TIMEOUT) {
    lastNrfUpdateTime = millis();
    resetNrfDataBuf();
    updateCarActionByNrfRemote();
  }
}

int do_auto(int automode, int current_time,  int start_time, int duration) {

  int stop_time = start_time + duration;

  if (current_time > start_time  &&   current_time < stop_time)

  switch (automode) {       //Definitions for automode; names are self defining, forward = moves forward etc
  case AUTO_MODE_STOP:
    motorRun(0, 0);
    servo1.write(90);
      break;
  case AUTO_MODE_FORWARD:
    motorRun(150, 150);
      break;
  case AUTO_MODE_BACKWARD:
    motorRun(-150, -150);
      break;
  case AUTO_MODE_RIGHTTURN:
      motorRun(200, -200);
      break;
  case AUTO_MODE_LEFTTURN:
      motorRun(-185, 185);
      break;
  case AUTO_MODE_SERVO_POS:
      motorRun(0, 0);
      servo1.write(180);
      break;
  case AUTO_MODE_SERVO_NEG:
      motorRun(0, 0);
      servo1.write(0);
      break;
  }  

  return stop_time;
}

void autonomous(int time_now) { //Action for autonomous mode
  int start_next;
  //                            mode     current_time  start_time  duration
  start_next = do_auto(AUTO_MODE_FORWARD,   time_now,        0,    1500); // 1500
  start_next = do_auto(AUTO_MODE_STOP,      time_now,  start_next, 1000); // 2500
  start_next = do_auto(AUTO_MODE_BACKWARD,  time_now,  start_next, 1500); // 4000
  start_next = do_auto(AUTO_MODE_STOP,      time_now,  start_next, 1000); // 5000
  start_next = do_auto(AUTO_MODE_RIGHTTURN, time_now,  start_next, 1500); // 6500
  start_next = do_auto(AUTO_MODE_STOP,      time_now,  start_next, 1000); // 7500
  start_next = do_auto(AUTO_MODE_LEFTTURN,  time_now,  start_next, 1500); // 9000
  start_next = do_auto(AUTO_MODE_STOP,      time_now,  start_next, 1000); // 10000
  start_next = do_auto(AUTO_MODE_SERVO_POS, time_now,  start_next, 1500); // 11500
  start_next = do_auto(AUTO_MODE_STOP,      time_now,  start_next, 1000); // 12500
  start_next = do_auto(AUTO_MODE_SERVO_NEG, time_now,  start_next, 1500); // 14000
  start_next = do_auto(AUTO_MODE_STOP,      time_now,  start_next, 500);  // 14500
}

void setupScreen() { //screen initialize
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

