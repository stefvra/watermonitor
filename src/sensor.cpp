#include "sensor.h"


DistanceSensor::DistanceSensor() {
    sensor = new VL53L0X();
}


DistanceSensor::~DistanceSensor() {
    delete sensor;
}


void DistanceSensor::init() {

  sensor->setTimeout(500);
  if (!sensor->init())
  {
  Serial.println("Failed to detect and initialize sensor!");
  while (1) {}
  }

  // setup for long range mode
  // lower the return signal rate limit (default is 0.25 MCPS)
  sensor->setSignalRateLimit(0.1);
  // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor->setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor->setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);


}



int DistanceSensor::measure() {

  int D;
  D = sensor->readRangeSingleMillimeters();
  if (sensor->timeoutOccurred()){
  Serial.println("Distance sensor TIMEOUT");
  } else {
  Serial.print("distance: ");
  Serial.print(D);
  Serial.println(" mm");
  }
  return D;
}





VoltageSensor::VoltageSensor(int _pin) {
  pin = _pin;
}

VoltageSensor::VoltageSensor() {}



void VoltageSensor::init() {
  pinMode(pin, INPUT);
}



int VoltageSensor::measure() {

  int D;
  D = analogRead(pin);
  return D;
}