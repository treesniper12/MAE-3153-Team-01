/**********************************************************************
  Filename    : RF24_Remote_Car.ino
  Product     : Freenove 4WD Car for UNO
  Description : A RF24 Remote Car.
  Auther      : www.freenove.com
  Modification: 2020/11/27
**********************************************************************/
#include "Freenove_4WD_Car_for_Arduino.h"
#include "RF24_Remote.h"

#include <Servo.h>

#define NRF_UPDATE_TIMEOUT    1000

Servo servo1;
u32 lastNrfUpdateTime = 0;

void setup() {
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
    
  if (nrfDataRead[5] == 0)
  {
    servo1.write(180);
  } 
  else if (nrfDataRead[6] == 0)
  {
    servo1.write(0);
  }
  else
  {
    servo1.write(90);
  }

    lastNrfUpdateTime = millis();
  }
  if (millis() - lastNrfUpdateTime > NRF_UPDATE_TIMEOUT) {
    lastNrfUpdateTime = millis();
    resetNrfDataBuf();
    updateCarActionByNrfRemote();
  }
}
